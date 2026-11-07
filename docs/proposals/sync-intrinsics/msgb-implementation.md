# MSGB 收发与 `__sync_*` 处理实现合同

关联：[Issue #309](https://github.com/LinxISA/Linx-TileOP-API/issues/309)、[接口与协议设计](README.md)、[C 声明](sync_intrinsics.h)。

状态：拟议协议与可执行传输参考，不是当前 PTO/Linx 发布 profile 的硬件支持声明。这里将收发操作定义到可实现、可测试的状态转换；实体寄存器字段与目标汇编绑定仍需 owning ISA/platform 接受。

## 1. 已确定的收发选择

发送端通过 `SSRSET` 写 MSGBD 暂存数据，最后由 **BSE 原子捕获并提交**。单独写数据寄存器不会向 HSCB 发布消息。接收端通过 `SSRGET` 读取稳定队首快照，读取完成后 **显式 ACK/POP**；短缓冲、读取失败或失败的 ACK 不弹出消息。

Linx 历史设计列出 MSGBCR `0x0830` 和 MSGBD1..10 `0x0831..0x083a`，并展示先写数据、再调用 BSE 的序列。这些地址是后端绑定的设计来源，不是本 PR 为当前 profile 新分配的编码。参考模型采用十个 64-bit staging word；不把这个暂存容量直接等同于高层应用 payload 容量。

## 2. 软件与硬件层次

| 层次 | 本提案定义 | 尚未实现的目标绑定 |
| --- | --- | --- |
| C 接口 | `__sync_*` 参数、状态码、request/event 生命周期 | 编译器 builtin／SDK lowering |
| 协议服务 | key/身份检查、owner 状态转换、reply/event 分发规则 | 完整端点运行时、SHA/SSA 实体实现 |
| 传输参考 | staging、BSE 接纳、RX claim/read/ACK、故障与背压 | 物理 SSR、队列与 HSCB 路由 |
| 数据可见性 | DONE 所依赖的目的可见事件与 acquire 边界 | NoC／L1 引擎完成接口、内存模型绑定 |

`reference/msgb_transport.h` 实现第三行的可移植 C 参考算法。它不实现整套 `__sync_*`，也不提供吞吐或硬件正确性证据。原始 packet 的 control 与 words 由目标 codec 构造；参考实现不把 C struct 内存原样当成网络消息。

## 3. 发送状态机

```text
IDLE
  → 独占暂存区并预留发送资源
  → 写完所有 MSGBD（未用 word 清零）
  → BSE 原子捕获 control、words、消息身份
  → ACCEPTED 或明确 REJECTED
  → 释放暂存区
```

详细顺序：

1. 校验消息长度、control 的目标合法性、调用权限，以及 request/event 端点资源。失败发生在发布前。
2. 获取独占发送暂存区。可以是每 PE 私有 bank，也可以是共享 bank 的独占 claim；其他提交者不能插入字段。
3. 将该报文全部数据写入 MSGBD。参考实现写满十个 word，尾部清零，避免短消息携带上一次暂存数据。逻辑 word_count 单独传给 BSE callback，由目标 codec 校验其与 control 长度合同一致；三字消息接收长度仍为三，不将清零尾部作为有效数据交付。
4. 确保这些 SSR 写入在提交处可见，再执行 BSE。BSE 是唯一发送接纳点；要么捕获完整消息并接纳一次，要么拒绝且没有远端副作用。
5. 无论拒绝或成功，结束当前 staging claim。失败写入可以留在暂存区，因为下一次发送重新覆盖全区，但不能执行 BSE 发布半条消息。
6. 接纳成功后，传输层在需要时按相同 request/message identity 重发。应用层不能因尚未收到回复而创建另一条逻辑请求。

普通指令重放必须保持“这次 BSE 是否已经接纳”的不可撤销记录。若平台可能出现发送结果不确定，必须通过同身份结果查询／重发去重恢复；不能把不确定结果报告成“未发送”并分配新身份。当前可执行参考只接受能明确区分 accepted/rejected 的后端 callback。

TX/RX claim 的任何非成功返回均保证没有新增所有权且不改变队列。若后端在取 claim 的中途出错，必须在返回前撤销这次 claim；wrapper 不负责清理一个从未成功交付的未知 claim。

多个 SSRSET 分别退休不应被解释为多次发送。只有 BSE 接纳产生协议副作用，错路径、squash 或更早故障不能将暂存数据提前发布。

## 4. 接收状态机

```text
RX_AVAILABLE
  → claim 队首身份和元数据
  → 校验长度／目的域／消息类型
  → SSRGET 读取稳定数据到私有临时区
  → 接收目标已经接纳数据或事件
  → 对精确队首身份 ACK/POP
  → 释放 claim
```

接收 claim 需要包含端点、队首 incarnation 和消息序号。对过期身份的 ACK 必须拒绝，不能弹出后来进入同一槽的消息。在 claim 持有期间，元数据和全部 MSGBD 读取属于同一报文；别的 PE 不得抢先弹出或切换快照。

普通应用收件箱的 `try_recv` 先在私有临时区读取完整消息，再执行条件 POP，成功后才向调用者发布输出。调用者保证输出地址有效、可写且调用期间不被释放。参考实现覆盖 callback 故障，不模拟 CPU 页故障或在 POP 之后失效的非法用户指针。

容量不足时报告所需 word／byte 数并保留队首；读取失败、元数据非法或 POP 被拒绝时，也保留队首和调用者原输出。解除 claim 只释放读取所有权，不等于 POP。允许修复条件后重试同一消息。

依赖通知不是普通应用消息。端点必须先幂等地锁存 DATA_READY/BUFFER_FREE/GROUP_READY 或错误结果，确保本地 token 已取得结果，再 ACK；如果事件表暂时无法接纳，保持背压。ACK 失败后的再次派发按通知身份幂等处理。不能先丢弃 RX 消息，再尝试分配可能失败的本地事件。

非法报文可以由管理路径明确隔离、记录错误后消费；参考 `try_recv` 不悄悄丢弃它。否则一个格式错误可能被误认成合法完成，或在没有诊断的情况下丢失请求。

## 5. 报文编码与响应

概念封装包含 service、opcode、domain/invocation、source/destination、request identity 和 payload。这不表示所有完整 key、参与者列表及描述符都能塞入十个数据寄存器。

本 PR 不分配线上字段位宽。目标实现需要在以下方案中明确选择并验证：部署期把完整 key/成员表绑定成短句柄；或定义有生命周期与权限检查的描述符访问协议。不能把源核指针当作目标可解引用的地址。当前参考没有分片组装或远端描述符拉取，超出 staging 上限明确拒绝。

每个命令有 transport/result envelope：`RESULT(request_id, status, optional_output)`。这属于命令应答，不增加应用服务 opcode。接纳 ACK、owner 处理 RESULT、DATA_READY 及 BUFFER_FREE 是不同结果。RESULT 与事件通知可以走不同顺序；本地 token 在发送接纳前已经预留，因此先于订阅确认到达的 READY 也可安全锁存。

ACK 的范围必须分别命名：

| 确认层次 | 说明 | 不保证什么 |
| --- | --- | --- |
| 发送接纳／传输确认 | 完整消息已被该阶段接受 | 目标服务已经处理或数据已可读 |
| RX ACK/POP | 精确消息已被接收端复制或安全锁存，可以释放该队列项 | 业务任务完成；不会自动替代 RESULT |
| 服务 RESULT | 对应 request 已得到成功／失败处理结果 | 除非服务明确规定，不代表 DATA_READY 或 BUFFER_FREE |

普通 `__sync_message_try_recv` 内部完成快照读取与 RX ACK/POP，调用方无需再 ACK 第二次。同步通知 handler 在本地 token 安全锁存后 ACK。具体 ACK 可通过专用状态／确认 SSR 的写操作绑定，不必先引入新 CPU opcode；物理地址和写入字段仍是待接受的目标合同。

公开架构对照：[RISC-V RPMI 消息协议](https://docs.riscv.org/reference/rpmi/message-protocol.html)使用独立 ACKNOWLEDGEMENT 返回请求处理结果，通知另有类型；[传输规范](https://docs.riscv.org/reference/rpmi/transport.html)区分请求和应答队列。它是平台消息协议，不是新 ACK 指令。[RISC-V IMSIC](https://docs.riscv.org/reference/aia/IMSIC.html)通过 CSR 操作认领 MSI，并明确指出分离的“读当前项、写清除”可能清掉后来出现的另一项。IMSIC 是中断机制，不是 payload mailbox；这里仅借鉴精确身份与原子认领边界，不能复制其读写副作用到 MSGB。

发送者身份来自受信端点绑定，不接受仅凭 payload 声称的身份。请求及通知去重状态在相关实例退役前保留。恢复状态与在途消息的生命周期必须协调，不能只清一个计数器就复用 identity。

## 6. 每类消息如何处理

以下为 owner／端点 handler 的实现规则；原子性作用于一个精确 key，由单一逻辑 owner 串行裁决，可以跨 key 并行。

| 消息 | 入站验证与状态转换 | 成功结果与错误处理 |
| --- | --- | --- |
| INIT | 校验 owner、key、window 版本、成员唯一性；原子安装不可变配置，预留完成／释放资源 | 同配置重试返回既有结果；冲突配置拒绝；创建成功后才接受该 key 的 WAIT/DONE |
| QUERY | 校验域与读取权限；捕获该 key 的一致快照 | 返回 snapshot；不创建读义务，不授予数据可读权限 |
| RETIRE | 校验 owner；确认旧数据访问、通知及结果义务可以退役 | 否则 BUSY；退役留下必要 stale/dedup 防护，不能马上让旧消息命中新对象 |
| SUBSCRIBE | 校验 consumer_slot 属于既定义务且尚未释放；原子执行 READY 检查与订阅登记 | 已 READY 则安排同身份通知；未就绪则存订阅；FREE/RETIRED 不授权新读。watch_free 使用相同订阅服务的独立条件 |
| DONE | 校验贡献身份、生产者授权和目的可见性证据；去重后记录该贡献 | 必要贡献全部满足时锁存 READY，安排通知；未知或失败贡献不加计数 |
| DATA_READY | owner 发出前检查数据条件与读义务；目的端匹配 invocation/key/本地 token | 记录成功和约定 acquire 关系，再确认接收；不直接把 L0 descriptor 标为有效 |
| RELEASE | 校验 consumer_slot、读取结束及无未来读义务；幂等归还该 slot | 所有义务归还且旧存储访问排空后转 FREE；重复释放不重复减计数 |
| BUFFER_FREE | owner 验证精确 generation 的 FREE 条件；目的端匹配 watch token | 允许复用该窗口的旧版本，不替代同步记录 RETIRE |
| ARRIVE | 校验 GROUP 类型、成员身份与 prior_done；每成员每代只接纳一次 | 完整成员到齐后转 GROUP_READY；纯 rendezvous 的空 prior_done 不携带 payload 保证 |
| GROUP_READY | owner 验证所有必要到达；目的端匹配成员事件及 group generation | 唤醒对应上下文／依赖，不冒充跨核数据传输完成 |
| TASK_NOTIFY | 校验目的端口、长度、message_id，预留应用 mailbox 容量后接纳 | 入箱 ACK 不等于任务执行成功；重复 message_id 不重复投递；数据地址依赖另走同步对象 |
| RESOURCE_RETURN | 校验 resource_id、精确 lease_id、归还端点和 use_done | 原子归还一个完整 grant；同 lease 重试幂等，不接受任意计数增量 |
| ABORT | owner 原子停止新授权，锁存错误，失败尚未完成的依赖；安排访问排空 | RESULT 表示接受取消；quiescent 事件表示排空结束。已不可撤销的数据效果保留 |
| ERROR | 校验 report_id、来源及作用对象，记录并交由 owner 决策 | 诊断 ACK 不表示恢复成功；不可恢复错误进入失败／abort 流程，不能伪造 READY |

FAILED 或 STALE 的异步结果必须使相应尚未完成的 event 进入错误终态，不能留下永久 PENDING。已经成功完成的 event 不倒退；已授予读权限由释放／排空协议收尾。

## 7. BWE、BWI 与解依赖

本地 event 表由 key、invocation 和动态 wait token 选择。READY 到达先锁存 sticky completion；等待操作原子执行 check-and-arm，避免在“检查未完成”和“进入睡眠”之间丢通知。

`BWE` 是候选等待入口。它如何选择 token、是否让出当前上下文，以及恢复后如何返回错误，仍需 ISA/profile 定义。`BWI` 用于中断路径，常规 Tile_READY 不必产生中断。独立的接收和同步处理逻辑在 PE 等待时继续运行。

某个 L1 数据对象 READY 只解除相应搬运的前置依赖；后续 Tile 运算仍等待实际 L1→L0 搬运与 Tile generation 发布。网络消息不携带可直接操作的远端物理 ROB 或寄存器编号。

## 8. 数据顺序与资源保持

发送消息的 BSE 接纳不能自动充当 NoC 数据 fence。DONE 前的 event 必须证明目标 window 在所声明可见性域已经提交；接收 READY 的 acquire 只覆盖这份合同。跨 PE 的先前工作需要明确聚合，不得用一个 PE 的局部完成冒充其他 PE 的进展。

消息队列、owner 表和事件表需要对已接纳工作保留完成与错误通路。同步器满不能阻断其自身已经承诺处理的 DONE/RELEASE；控制通路无进展时不能让所有 PE 仅靠 BWE 等待解决。

生产者先完成与消费者晚订阅均合法，但初始化先于二者。消费者义务不从当前等待者数推导。FREE 后不再授予新读，旧通知到达只匹配旧的已结束身份，不能产生新授权。

## 9. 目标实现仍需提供的绑定

| 项目 | 所需定义 |
| --- | --- |
| MSGBCR | control 字段、有效类型、长度／目的编码、零值与非法值 |
| TX | staging bank 所有权、队列 credit、BSE 接纳结果、重放时的请求身份 |
| RX | 可用状态、长度、claim/snapshot、精确消息身份、ACK/POP、解锁但不弹出 |
| 传输 | 路由、安全域、可靠性、重复／乱序、reset 后的旧报文处理 |
| 内存顺序 | 数据引擎完成到 DONE、READY 到后续加载的顺序边界 |
| 同步等待 | BWE token 参数、pending 状态、异常恢复及错误结果 |
| 编译器 | 合法 SYS envelope、SSR side effects、BSE 不可撤销提交点、优化器约束 |

目标 lowering 在这些合同接受后实现，并补充精确汇编、独立功能模型、异常／重放及硬件端到端证据。当前 PR 不将历史寄存器页面或主机测试当作这些工作的替代。

## 10. 本 PR 的可执行检查

运行 `python3 test/test_sync_intrinsics_proposal.py -v` 或 `make check-sync-proposal`。

检查包括 C11/C++17 下应用与服务声明，以及 portable fake backend 上的真实 C 收发调用。覆盖队列满、staging 写失败、BSE 拒绝、尾部清零、稳定接收 claim、短缓冲、读取失败、ACK 失败及精确弹出。

这些用例验证传输参考的状态转换。owner handler 表、完整 HSCB、请求重放恢复和数据面可见性仍需要后续实现与验证。
