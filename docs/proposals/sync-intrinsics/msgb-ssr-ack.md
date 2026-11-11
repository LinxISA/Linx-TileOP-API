# MSGB Message ID、SSRSET ACK 与收发状态机（v0.3 提案）

更新：低层实现现固定 PTO PR #426 `d537afa23ea0783b55f994ac52e2154f44b60b0c` 的未合并扩展，见 [backend/runtime 对接说明](backend/README.md)。下文保留 #310 的原始设计背景；涉及“SSR 地址／control 未分配”的段落已由该扩展取代。MSGBCR 为只读，BSE 源操作数携带 control。目标支持、数据可见性与发布状态仍需各自验证。
跟踪：[Issue #309](https://github.com/LinxISA/Linx-TileOP-API/issues/309)、[PR #310](https://github.com/LinxISA/Linx-TileOP-API/pull/310)。本页拥有本提案的消息身份、ACK 字段与状态转换；其他设计页引用本页，不再将 ACK 机制列为未决。

采用 XLEN=64。发送为 SSRSET 暂存数据后 BSE 原子提交；接收为 claim、SSRGET 稳定快照、`SSRSET MSGBACK, rx_token` 确认消费。下文寄存器名、位域及行为是此次拟议 profile 的具体设计；数值 SSR 地址仍待 owning ISA 的正式分配，不把候选名字宣称为当前已分配寄存器，也不在 TileOP API 仓库修改 PTO/Linx 编码。

## 1. 三种身份不可混用

| 身份 | 宽度与来源 | 用途 |
| --- | --- | --- |
| Service request/message ID | 64 位，由 API／运行时分配，按发送者与 invocation 隔离 | 关联业务请求及 RESULT；C 字段明确命名为 application_message_id |
| Transport Message ID | 128 位，发送端硬件／端点在 BSE 接纳时分配 | 区分核间消息，传输重试和接收去重 |
| RX token | 64 位，接收端每次成功 claim 分配 | 精确标识本次快照及 owner，作为 MSGBACK 的写入值 |

### 1.1 Transport Message ID

| 组成 | 位域 | 含义 |
| --- | --- | --- |
| MSGID_HI | [63:32] SOURCE_ENDPOINT | 32 位受信端点编号；与逻辑 block_id 分开，由部署绑定 |
| MSGID_HI | [31:0] SOURCE_EPOCH | 32 位非零发送端 incarnation；重置／重绑定不能恢复为旧值 |
| MSGID_LO | [63:0] TX_SEQUENCE | 64 位非零接纳序号，在 source epoch 内严格递增 |

完整去重键是 `{transport_domain, SOURCE_ENDPOINT, SOURCE_EPOCH, TX_SEQUENCE}`；transport_domain 来自受信队列绑定，不接受 payload 伪造。MSGID 是硬件附加的传输元数据，不占 MSGBD 的十个业务 word。线上字节序、路由头及 control codec 另由 transport profile 定义。

TX_SEQUENCE 仅在 BSE 成功接纳时分配。暂存失败、credit 不足或 BSE 拒绝不分配新 ID；同一已接纳 BSE 的硬件重放／传输重试保留原 MSGID。新一次应用提交可以有新 MSGID，但同一业务 contribution 的重复仍由 service key 去重。

序号与 epoch 均不回绕。序号耗尽时拒绝新接纳；在旧传输、回复、去重窗口和恢复引用安全退休后，由管理路径进入下一 epoch。SOURCE_EPOCH 耗尽则不能在同一身份空间内复用。链路复位不等于清除所有旧身份；无法保存状态时必须隔离旧链路流量并执行受控重建。

### 1.2 RX token

| 位域 | 名称 | 含义 |
| --- | --- | --- |
| [63:48] | RX_EPOCH | 16 位非零接收端 incarnation |
| [47:0] | CLAIM_SEQUENCE | 48 位非零 claim 序号；每次成功 claim 分配 |

RX token 在同一个接收端点身份空间内唯一，涵盖所有可以经该 SSR bank 访问的队列；不能为不同队列各自从 1 计数。位域任一为零的 token 非法。CLAIM_SEQUENCE 与 RX_EPOCH 均不回绕，耗尽返回 EXHAUSTED 并停止新 claim。

同一条消息释放 claim 后重新 claim，**必须获得新的 RX token**。这保证旧 ACK 不能跨越 cancel/reclaim 边界。消息本身的 Transport Message ID 不因重新 claim 改变。

端点换 RX_EPOCH 前停止新访问、排空或确认取消旧 claim／ACK 操作，并处理旧队列状态；参考模型要求无队首、无活动 claim 才可换 epoch。旧软件 token 随后得到 STALE_EPOCH，不能误命中新快照。RX_EPOCH 最大值后不允许 wrap 或直接重新初始化成 1。真正冷启动使用新的隔离域／端点 incarnation，只有确认所有旧引用和旧流量失效后才建立全新身份空间。

如果取消后序号耗尽，而队列仍有无法再次 claim 的消息，端点进入管理恢复：先对已接纳请求明确报告失败并完成受控取消，再清理旧队列以建立下一 epoch；不能把清理伪装成成功 ACK，也不能释放仍被数据面使用的 Buffer。参考模型不实现管理取消，明确保留 EXHAUSTED/BUSY 状态，不靠原地 init 偷越这个边界。

## 2. SSR 与 bank 作用域

SSR bank 绑定 `{domain, receiving_endpoint, PE, execution_context_incarnation}`；同一 bank 的控制写入按退休顺序执行，保持最多一个活动 RX claim。每次命令验证当前受信 owner；陷阱恢复不能将另一上下文的快照或 ACK 结果当成自己的。

不同 PE 可以拥有各自状态／结果 bank，但同一接收队首最多由一个 owner claim。跨上下文切换要保留绑定或先完成受控 release；不能仅切换一个软件指针就复用活动 bank。

| 寄存器 | 访问 | 字段／语义 |
| --- | --- | --- |
| MSGBD1..10 | SSRSET / SSRGET | 写访问 TX staging；读访问已 claim 的 RX snapshot，二者是分离的数据 bank |
| MSGBRXCLAIM | WO | [0] CLAIM：1 请求原子 claim，0 无操作；[63:1] 必须为零 |
| MSGBRXSTATE | RO | [7:0] 最近 CLAIM/RELEASE 结果；[8] OWNED；[9] ACKED；[15:12] WORD_COUNT；其余位读零 |
| MSGBRXTOKEN | RO | [63:0] 当前活动快照的 RX token；未持有时读零 |
| MSGBRXMSGID_HI/LO | RO | 已 claim 消息的 128 位传输 ID；两个寄存器由同一快照保持稳定 |
| MSGBRXCONTROL | RO | 已 claim 消息的 control 值，字段由目标 codec 定义 |
| MSGBRXRELEASE | WO | [63:0] 要释放的精确 RX token；只释放快照，不弹出未确认消息 |
| MSGBACK | WO | [63:0] 要确认的精确 RX token；一次 SSRSET 是原子的本地消费提交点 |
| MSGBACKSTAT | RO | [7:0] ACK_RESULT；[8] VALID；[63:9] 读零 |
| MSGBACKID | RO | [63:0] 与 MSGBACKSTAT 对应的最近一次 ACK 写入值 |
| MSGBTXID_HI/LO | RO | 最近一次已接纳 BSE 的传输 ID；失败的尝试不伪造新 ID |
| MSGBTXSTAT | RO | [7:0] 最近 BSE 结果；[8] VALID；其余读零。是否接纳以结果码为准，不能只看残留 TXID |

有效 WORD_COUNT 为 0..10，不含清零尾部。无活动快照时，RX 数据／ID／control 访问拒绝，不读取下一条消息。ACKED 但尚未 release 时保留完整不可变快照、token 与身份，允许为异常恢复重新 SSRGET 同一份快照；这些读取不切换到新队首。MSGBRXCLAIM 的保留位非法时返回 BAD_COMMAND，无 claim 或队列变化。

无快照、非 owner 或超出有效 word 的 SSRGET 是精确的失败读取：不更新目的寄存器、不置 read_mask、不弹出队首。参考模型分别返回 NO_CLAIM、NOT_OWNER、BAD_COMMAND，并保持输出参数不变；真实指令对应哪个已有异常编号，仍需 owning ISA 正式绑定，不以零 payload 代替错误。只读的状态／ACK 结果寄存器不具有 POP 或清零副作用，并按本上下文程序顺序观察先前已退休 ACK 的结果。

在新的、受信建立的 bank 中，ACKSTAT.VALID=0、ACK_RESULT=NONE、ACKID=0、last_success 无效，RXSTATE 的结果为 NONE，OWNED/ACKED/WORD_COUNT 为零。没有本 owner 快照时 WORD_COUNT 必须为零。受控换 RX_EPOCH 后，已存在上下文的历史结果寄存器可以保留供诊断，但旧 last_success 因 epoch 不匹配不再授予 DUPLICATE 成功；下一个命令覆盖其对应结果。陷阱保存／恢复不得回滚已发生的 POP、claim 序号或发送接纳记录。

本提案统一的 8 位结果码：

| 值 | 名称 | 含义 |
| --- | --- | --- |
| 0x00 | NONE | 尚无结果，ACKSTAT.VALID=0 |
| 0x01 | OK | 操作成功；对 MSGBACK 表示首次 ACKED 并弹出一条 |
| 0x02 | DUPLICATE | 同 owner 本 epoch 最近一次成功 ACK 的重复，无队列副作用 |
| 0x03 | EMPTY | 没有可 claim 的队首 |
| 0x04 | BUSY | claim 已被占用或本次发送未获接纳 |
| 0x05 | NO_CLAIM | 没有可用于这次操作的活动 claim |
| 0x06 | NOT_OWNER | 当前上下文不是 claim owner |
| 0x07 | TOKEN_MISMATCH | token 与当前 claim 不匹配 |
| 0x08 | STALE_EPOCH | token 属于旧／其他接收 epoch |
| 0x09 | BAD_TOKEN | RX_EPOCH 或 CLAIM_SEQUENCE 为零 |
| 0x0a | NOT_READ | 尚未成功读取全部有效 MSGBD word |
| 0x0b | EXHAUSTED | 身份空间耗尽，禁止回绕 |
| 0x0c | BAD_COMMAND | 保留位、长度或操作形态非法 |
| 0x0d | IO_ERROR | 提交前失败；没有本次 ACK／发送的不可撤销效果 |

不存在的结果码保留，不作成功解释。常规协议错误经状态寄存器返回；SSR 地址访问权限错误仍按 owning ISA 的系统寄存器访问异常处理，不能假定任何权限级均可访问。

## 3. SSRSET MSGBACK 的精确行为

输入是一个完整 RX token。写指令仅在允许产生不可撤销副作用的位置执行。退休时 ACKSTAT 与 ACKID 已构成一致的本地结果；下一次 ACK 写入前，调用方应读走这份结果。当前 owner 的错误状态写入不能覆盖另一 owner 的结果 bank。

处理顺序：

1. 验证 SSR 权限与当前 bank 绑定；非法访问按 ISA 异常处理，不修改队列或任何其他 owner 的结果。
2. 验证 token 非零字段以及当前 RX_EPOCH。格式错误返回 BAD_TOKEN，epoch 不匹配返回 STALE_EPOCH。
3. 如果 token 等于当前 owner 在本 epoch 保存的 `last_success_token`，返回 DUPLICATE，既不弹出队首，也不修改可能存在的新 claim。
4. 否则检查活动 claim、owner、精确 token，分别返回 NO_CLAIM、NOT_OWNER 或 TOKEN_MISMATCH。
5. 检查当前 claim 的 read_mask 是否覆盖全部 `WORD_COUNT` 有效 word；未覆盖则 NOT_READ。
6. 原子弹出该 claim 指向的精确队首，记录 `last_success_token`，进入 ACKED 状态，返回 OK。token/快照身份保持到显式 release。

每次合法的 ACK 寄存器写都会更新本 owner 的 ACKSTAT/ACKID；失败写入**不得清除**独立的 last_success_token。否则在一次失败后重放已成功 ACK 会失去幂等判定。只保存最近成功记录即可：更旧的 token 可以拒绝为 stale/no-claim/mismatch，但永远不能弹出新消息。

read_mask 只在相应 SSRGET 成功交付并提交后更新；读取异常不置位，重复读取同一 word 不替代其他 word。零 word 消息的完整读取条件自然成立。read_mask 证明寄存器已被读取，不能证明应用已经持久保存或处理数据；依赖 handler 仍需先安全锁存结果再 ACK。

ACK 是**接收队列消费确认**。它可以驱动传输 credit／消费回执，但不自动生成业务 RESULT，也不表示 DATA_READY、BUFFER_FREE 或消费者读义务归还。

## 4. 接收状态转换

| 当前状态 | 动作 | 下一状态／效果 |
| --- | --- | --- |
| IDLE，无队首 | CLAIM | EMPTY，状态不变 |
| IDLE，有队首 | CLAIM 成功 | CLAIMED，分配新 token，read_mask=0，锁住稳定快照 |
| 任意状态 | CLAIM 值为 0 | 无操作，结果、token、read_mask、计数器均保持 |
| CLAIMED/READ_COMPLETE/ACKED | 再次 CLAIM | BUSY，保留当前快照、token、read_mask 和 claim 计数器 |
| CLAIMED | SSRGET 有效数据 word | 设置对应 read bit；全部读取后为 READ_COMPLETE |
| CLAIMED，未读全 | ACK 当前 token | NOT_READ，保留消息与 claim |
| CLAIMED/READ_COMPLETE | RELEASE 当前 token | IDLE，消息不变、旧 token 失效；下次 claim 分配新 token |
| READ_COMPLETE | ACK 当前 token | ACKED，精确弹出一次并保存成功记录 |
| ACKED | SSRGET 快照／重复 ACK 同 token | 重读原快照，或 DUPLICATE；均不访问／弹出新队首 |
| ACKED | RELEASE 当前 token | IDLE；不再次弹出消息 |
| 任意状态 | 旧成功 token 的重复 ACK | 仅在 owner/epoch/最近成功记录匹配时 DUPLICATE；不改变当前 claim |
| 任意状态 | 错 token、旧 epoch、非 owner ACK/RELEASE | 拒绝，无队列或 claim 变化 |

RELEASE 同样携带精确 token，避免迟到的“取消当前 claim”误取消新 claim。其验证不要求所有数据已读，也不复用 ACK 的 last_success 幂等路径。没有活动 claim 时返回 NO_CLAIM；取消不算数据已消费。

RELEASE 的错误优先级依次为 token 格式、epoch、是否存在 claim、owner、token 匹配；成功后清除当前快照，保持队列和独立 last_success 不变。CLAIM 的优先级为保留位、零值无操作、已占用、队列为空、身份耗尽，只有全部满足才增加 claim 序号。

接收参考要求短缓冲在读取／ACK 前返回，读取失败不 ACK。若本地 ACK 已提交，但其调用者尚未观察到结果便发生异常，恢复使用同 token 和保留结果；不能重新 claim 当前队首，再对新消息执行旧 ACK。

## 5. 发送状态转换

| 当前状态 | 动作 | 下一状态／效果 |
| --- | --- | --- |
| IDLE | 取得发送 staging 所有权 | STAGING，绑定动态提交 occurrence |
| STAGING | SSRSET MSGBD | 仅修改该 owner 的 TX staging，不发包 |
| STAGING | BSE 校验失败／队列满 | REJECTED，不分配 MSGID、不发布；释放 staging 后可重试 |
| STAGING | BSE 成功 | ACCEPTED，原子捕获 control、长度、数据、MSGID；身份与结果成为不可撤销记录 |
| ACCEPTED | 传输重试或同 occurrence 重放 | 保留 MSGID，不重新提交第二条逻辑消息 |
| ACCEPTED | 远端接收／消费确认 | 按相应层次推进，不能冒充业务完成 |
| ACCEPTED | 结果丢失或异常恢复 | 查询／重放精确 occurrence 的记录；不按“未发送”分配新 ID |

硬件区分“同 occurrence 的重放”与“新的应用提交”。在记录仍可能被恢复路径使用时不得回收；记录容量不足在接纳前背压。TX 数据 bank 与 RX 快照 bank 分离，接收读取不能破坏尚未 BSE 的发送暂存。

接收去重使用完整 Transport Message ID；重复传输不能生成第二条队列消息或重新增加服务贡献。已消费 MSGID 的去重记录需要保留到该源的安全重放窗口关闭，不能在 RX POP 时立即忘记。去重窗口与链路重传实现仍由传输 profile 实现；本页不声称 ACK 参考模型实现了整条网络。

## 6. 推荐低层序列

下列为逻辑 SSR 操作，不分配数值地址或假定现有汇编器已支持新增名字：

```text
SSRSET MSGBRXCLAIM <- 1
SSRGET MSGBRXSTATE → 确认 OK / OWNED
SSRGET MSGBRXTOKEN → token
SSRGET MSGBRXMSGID_HI/LO → 传输消息身份
SSRGET MSGBD1..WORD_COUNT → 私有临时区
将依赖结果安全锁存，或准备向应用交付已复制消息
SSRSET MSGBACK <- token
SSRGET MSGBACKSTAT / MSGBACKID → 检查 OK 或精确 DUPLICATE
SSRSET MSGBRXRELEASE <- token
```

短缓冲或读取失败走 `MSGBRXRELEASE <- token`，不得写 ACK。`__sync_message_try_recv` 内部封装完整序列，普通调用方不需要再 ACK 一遍。同步服务端独立处理事件锁存与 ACK，不能把普通收件箱 POP 后才开始的可能失败派发当作可靠事件投递。

## 7. 验证边界

参考模型覆盖字段构造／解析、精确 token、未读全拒绝、重复 ACK、失败后重复成功 ACK、非 owner、cancel/reclaim、新消息到达、epoch 变化、序号耗尽及保留位检查。错误路径验证队列和 claim 不被修改。

数值 SSR 分配、实际 privilege decode、BSE 重放日志、HSCB 重传窗口和跨数据引擎可见性仍需 owning ISA／平台实现及验证。主机参考测试不代替这部分证据。
