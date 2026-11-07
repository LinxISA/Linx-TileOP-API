# Scalar sync intrinsics 与 MSGB/HSCB 收发合同（提案）

日期：2026-11-07。状态：讨论草案，尚非 PTO ISA 已接受语义或硬件实现说明。

跟踪：[Issue #309](https://github.com/LinxISA/Linx-TileOP-API/issues/309)。ACK 的寄存器位域与消息身份见 [SSRSET ACK profile](msgb-ssr-ack.md)；具体的发送、接收和逐消息 handler 规则见 [MSGB 实现合同](msgb-implementation.md)；可执行的传输参考见 [msgb_transport.h](reference/msgb_transport.h)。

本文定义拟议的标量 `__sync_*` 接口、MSGB/HSCB 收发合同及 Tile 依赖协议，并记录其在 WSE Calendar／Superscalar NPU 中的使用场景。文中的消息名、状态字段和伪代码表达候选合同，不表示现有指令、寄存器编码或 SDK 已支持。

## 1. 已确认边界、设计建议与证据

| 分类 | 内容 |
| --- | --- |
| 用户确认 | 保留 WSE NoC／Calendar，重新设计 Superscalar NPU Core 接口；每核 L1 SRAM 超过 100 MB；MSGB 通过 SSRGET/SSRSET 访问系统寄存器，并连接 HSCB；希望通过 HSCB 解除核间 Tile 依赖 |
| 用户给出的协议意图 | 消费者订阅，生产者异步上报，SHA/SSA 维护计数及持久 READY，并分发通知；数据就绪与 Buffer 复用分开 |
| 本文建议 | 大容量地址型 L1 承接网络，L0 Shared/Local Tile 承接计算；采用通用消息封装及带版本的同步对象；每个同步对象由单一逻辑 owner 串行裁决 |
| 尚未确定 | L0 Shared 是否由 256 KiB 扩到 512 KiB；HSCB 其余服务报文格式、可靠性、队列深度；SHA/SSA 模块划分；BWE 的目标事件与等待粒度；指令／SSR 编码 |

公开证据：[华为《昇腾 950 NPU 架构白皮书》](https://public-download.obs.cn-east-2.myhuaweicloud.com/ascend/%E6%98%87%E8%85%BE950%20NPU%E6%9E%B6%E6%9E%84%E7%99%BD%E7%9A%AE%E4%B9%A6.pdf)，关键术语及 §4.4，将 HSCB 用于 STARS 与计算核之间的专用控制交互，并描述其广播能力及与数据 NoC 的区分。本次检索未找到足以确认 SHA/SSA 缩写含义、WAIT/DONE 线格式或 expc_cnt 状态机的公开一手规范。因此下文不将拟议协议归因于已经公开验证的 HSCB 标准。

NVIDIA 的 [DSM](https://docs.nvidia.com/cuda/cuda-programming-guide/02-basics/writing-cuda-kernels.html#distributed-shared-memory)提供 cluster 内远端 shared memory 访问与生命周期合同，可借鉴其所有权和同步边界；这不是 WSE 的容量或时延证据。[异步搬运文档](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/async-copies.html)说明异步完成和数据可见性需要显式衔接；这里也不直接复制其编码和执行模型。

两项可用于检查边界的公开接口：CANN [Notify 同步](https://www.hiascend.com/doc_center/source/zh/canncommercial/900/programug/acldevg/runtime_doc_dev_0027.html)的等待会重置通知，而 Event 可由多个等待观察，说明一次性通知不能直接承载本文的持久 READY。NVIDIA [PTX mbarrier](https://docs.nvidia.com/cuda/parallel-thread-execution/#parallel-synchronization-and-communication-instructions-mbarrier)分别跟踪到达与异步事务完成，可作为区分贡献计数和 payload 完成的参考。这两者都不是 SHA/SSA 内部协议的证据。

## 2. 存储、数据与控制的分工

| 层次或通路 | 职责 | 对外完成含义 |
| --- | --- | --- |
| 每核大容量 L1 SRAM | 保存长期数据、通信窗口和中间结果 | 对指定访问路径可见，不自动表示 L0 已加载 |
| L0 Shared Tile | 多 PE 共享计算工作集、分块预取 | 指定 generation 的 descriptor/payload 已就绪并发布 |
| PE Local Tile | 私有操作数与累加结果 | 现有 Local Tile 数据依赖与生命周期 |
| NoC／Calendar | 批量数据传输、已支持的网络归约 | 区分源读取结束、目标提交和通信整体结束 |
| HSCB／MSGB | 依赖订阅、完成通知、资源控制及一般核间消息 | 消息被接纳／处理；是否蕴含数据可见性由消息类型合同决定 |
| 同步服务（暂称 SHA/SSA） | 维护同步对象、完成贡献及消费者义务 | DATA_READY、BUFFER_FREE 或错误状态 |

默认数据路径是 NoC → 接收方 L1 → TLOAD／相应搬运 → L0 Tile → 计算。网络直接写 L0 可作为后续优化，但必须另行证明 generation、发布和容量保持的正确性。

L0 Shared 先保留 256 KiB 比较基线，同时评估 512 KiB。扩大 Shared 总池与扩大单 Tile 最大尺寸是两个决策。当前 PTO 还具有各 PE 独立的 Local pool，不能把 Shared 容量当成 Core 的全部计算存储。容量选择需同时验证活跃 Tile 集合、流水深度、bank 冲突、L1→L0 持续带宽与物理实现代价。

## 3. 通用核间消息接口

复用 SSRGET/SSRSET 作为软件访问入口。MSGB 是传输与排队接口；同步服务是其上的一种消息使用者。一般任务消息不应修改同步计数。

建议逻辑消息头如下；字段可以压缩或由受信上下文隐含携带，本文不分配位宽或新增 SSR 地址。

```text
Message {
    protocol_version, service_class, opcode
    domain_id, invocation_id
    source_endpoint, destination_endpoint
    request_id, correlation_id
    payload_length, status
    payload
}
```

`service_class` 区分 DEPENDENCY、TASK、RESOURCE、COMPLETION、ERROR 等用途。传输层 ACK 只确认其定义的接收／处理点，不等价于 DATA_READY。消息重发保持 request_id；服务语义的去重身份由各 opcode 进一步定义。

SSR 入口需要补齐以下合同：

- 发送数据寄存器构成暂存区；显式提交动作一次捕获完整报文。队列满时返回背压／未接纳，不能发送半条报文。
- 提交时捕获精确 operand 和请求身份；推测路径不得对外发送，重试不得生成第二次服务副作用。
- 接收头部及数据在多次 SSRGET 期间保持同一消息快照；显式 POP／ACK 后才前移队列。具体副作用仍待定义。
- 多 PE 发送暂存区采用独立上下文或明确的软件所有权，不能允许不同 PE 的字段交错组成消息。
- 保证 DONE、READY、RELEASE、错误及必要回执能够取得进展；不能让等待中的执行线程成为唯一的收包或回执处理者。
- 传输可靠性尚未确认。实现可依靠可靠传输，或端点重试与去重；不能在队列满时静默丢弃协议消息。

## 4. 同步对象与身份

原文的 `block_id` 表示逻辑任务编号。本协议使用 `object_id` 表示数据对象，避免它与逻辑任务、PTO 指令 Block 混淆。

```text
SyncKey = {domain_id, invocation_id, object_id, generation, visibility_domain}

SyncEntry {
    key, config_identity, state
    expected_contributions, completed_contributions
    expected_consumers, released_consumers
    subscriptions
    buffer_binding, visibility_contract
    notification_pending, error
}
```

这是逻辑状态，集合不要求用固定 bitmap 实现。`visibility_domain` 指结果在哪些接收窗口／访问路径上已可用；实现可让 object_id 隐含这一信息。一个对象在多个 Core 有独立副本时，不能仅凭源端完成就向所有副本宣告可读。

基础方案由编译器／运行时为每代显式建立配置并等待接纳，然后才允许 WAIT/DONE。消费者可以晚于生产者到达，但配置应先于二者生效。不存在的 key 返回未建立或明确重试，不允许任意 WAIT 覆盖配置，也不允许未知 DONE 静默丢失。

同 key、同配置的 INIT 重试幂等；同 key 的冲突配置报告错误。新 generation 不可通过覆盖旧活动记录来创建。取消、换 invocation 或位宽回绕都需要避免迟到消息命中新实例。

正常生命周期为 `INITIALIZED/COLLECTING → READY → BUFFER_FREE → RETIRED`。消费者可以在 COLLECTING 或 READY 时订阅，不能在 BUFFER_FREE 后取得新的读取授权。FAILED/ABORTING 是失败分支：通知失败与停止授权先发生，资源安全回收仍等待相关访问排空。READY 的持久性持续到本版本停止授权，不承诺在存储已经复用后继续返回成功。

## 5. 从计数到精确完成贡献

`expc_cnt` 是必要完成贡献的数量，`cnt` 是已经接受的不同有效贡献数量，均不是消费者数量。一个生产者可以承担多个分段，多个核也可能聚合为一个经定义的贡献。因此去重键采用 contribution_id，而不只采用 producer_core_id。

基线配置固定集合 E：

```text
expc_cnt = |E|
DONE(key, contribution_id):
    检查身份、来源权限、贡献归属与完成合同
    如果 contribution_id 已接受：返回原处理结果，不重复计数
    否则将其加入 D
    cnt = |D|
    当 D = E 且 visibility_contract 满足时，锁存 DATA_READY
```

不属于 E 的贡献报告协议错误，不能通过任意 `cnt++` 使阈值提前满足。按此定义计数不会合法地越过阈值；`cnt >= expc_cnt` 只能作为建立去重与成员验证之后的实现判定。零贡献配置仅在其结果／空数据可见性条件确实满足时就绪，不能凭零计数制造有效数据。

计数单位还须与 Calendar 的 `exp_val` 区分：前者是语义贡献，后者是有效网络 payload 字节。字节数不能代替贡献身份、范围覆盖或归约完成；重复贡献即使在同步器被去重，也不能修复已被网络重复执行的非幂等归约。数据面自身需要防重放或等价保障。

## 6. T0–T6 正常流程

前置条件：同步对象配置已建立，目标存储及消费者义务已绑定。T0–T6 表示逻辑步骤，WAIT 与 DONE 可以并行、交换到达次序。

| 时刻 | 主体 | 操作 | 所需状态变化 |
| --- | --- | --- | --- |
| T0 | 消费者 | SUBSCRIBE／WAIT(key, consumer_slot) | 提交订阅，不强制整个 Core 阻塞 |
| T1 | 同步 owner | 原子检查并登记订阅 | 未就绪则保存订阅；已就绪且读义务仍有效则立即返回 READY；已关闭／错误则返回相应状态 |
| T2 | 生产者或完成代理 | DONE(key, contribution_id) | 仅在对应可见性合同已经满足后发送 |
| T3 | 同步 owner | 校验并去重贡献 | 更新精确贡献集合及计数 |
| T4 | 同步 owner | 判断完整贡献与可见性 | 一次性锁存 DATA_READY |
| T5 | 同步 owner | 向订阅端点分发 READY | 记录待投递通知；背压或重复投递不能丢状态或重复授权 |
| T6 | 消费端点 | 接受 READY，完成 acquire，再解除匹配依赖 | 允许后续数据加载；L0 计算仍等待实际加载与 Tile 发布 |

每个 key 的 SUBSCRIBE、DONE、RELEASE、CLOSE、ABORT 需要具有可串行化的状态转换。检查 READY 与插入订阅之间不能存在丢唤醒窗口。

READY 是持久状态，订阅通知是可重发的观察结果。已 RELEASE 的 consumer_slot 不能靠重复订阅获得新读权限；同一逻辑消费者再次使用数据，需要预先独立的消费义务或新的、被允许的租约。

## 7. DATA_READY 与 BUFFER_FREE

DATA_READY 解决新数据的读取许可；BUFFER_FREE 解决旧存储被覆盖的许可。订阅者集合仅表示谁要求通知，不能等同于所有将来可能读取数据的消费者。

基线使用静态消费者义务集合 C。每个元素是消费发生次序的身份，例如 `{logical_task, consumer_slot}`，不是仅有 core_id。集合由计划固定，消费者晚订阅也已经拥有保留义务。

条件分支若最终不使用某项义务，需要由受授权的 owner 确认其不会再读，再明确撤销该义务；不能因为未收到 WAIT 或经过一段时间就自动减掉消费者。

```text
RELEASE(key, consumer_slot):
    校验其读义务有效、实际读取及任何后续读取引用均已结束
    将 consumer_slot 幂等地加入 released_consumers

BUFFER_FREE 条件：
    本代已完成且禁止新增消费者义务
    released_consumers = expected_consumers
    不再存在会访问该存储的 DMA／网络／重放引用或写入
```

只有 WAIT 注册和 RELEASE 数量相等并不足够。例如 C0 订阅、读取并释放，C1 尚未订阅；若此时复用 Buffer，C1 随后会拿着旧 READY 读取新数据。固定集合 C 可以避免这一竞态。

未来若需要动态消费者，使用显式 REGISTER 获得读租约，并由唯一 owner 执行 SEAL/CLOSE。只有注册已关闭、所有已授权租约归还且存储访问已排空，才能 FREE。动态模式作为扩展，不混入静态基线。

远端源 L1 在传输引擎实际读完后，相关“搬运消费者”可释放源窗口；目的 L1 在搬入 L0 且后续不再依赖该 L1 版本时可释放。L0 的 last-use 则由 Tile 的所有本地实际消费者决定。这三个释放点可以不同，禁止在 DMA 仅提交时释放源。

## 8. 可见性与多接收者

默认以接收方 L1 窗口为 DATA_READY 的可见性边界。生产者发射完计算、源 L1 写入完成、NoC 已读取源、目的 L1 已提交，分别是不同事件。

```text
目的窗口 payload 提交并可见
    → 满足相应 contribution 的 DONE 条件
    → 同步 owner 锁存 READY
    → 消费端点完成约定的 acquire
    → TLOAD 读取 L1
    → L0 generation 就绪／发布
    → 后续 Tile 运算解除数据依赖
```

DONE 可由知道实际目标完成的接收引擎产生，也可由收到明确目标确认的生产者代理产生。本文不假设现有 HSCB 消息自带这类顺序，也不假设现有 FENCE.D 已覆盖 NoC 引擎。

对于 multicast，接收者 A 可见而 B 尚不可见时，不能向 B 返回与 A 相同含义的 READY。可为各目的窗口分别建 key；或让一个全局 key 等待所有被承诺副本完成后再统一 READY。前者暴露更多并行度，后者状态较集中；具体按编译计划选择并固定。

## 9. 通用消息类型与服务分层

| 服务 | 候选消息 | 语义 |
| --- | --- | --- |
| 对象管理 | INIT、QUERY、RETIRE | 建立、查询、回收同步实例 |
| 数据依赖 | SUBSCRIBE、DONE、DATA_READY | 订阅、提交完成贡献、通知可读 |
| 存储寿命 | RELEASE、BUFFER_FREE | 归还消费义务、允许覆盖旧存储 |
| 跨核集合 | ARRIVE、GROUP_READY | 按预定成员到齐；独立于 payload 完成 |
| 任务与资源 | TASK_NOTIFY、RESOURCE_RETURN | 一般任务通知、资源归还；不修改 Tile 完成计数 |
| 异常 | ABORT、ERROR | 传播失败、停止新工作并安排排空；不伪造成功 |

通用化应统一封装、投递、身份、队列与错误报告，同时保留每种 opcode 的状态含义。不能把所有报文都解释成“计数加一”。

`WAIT` 是用户侧等待语义；底层可以先 SUBSCRIBE，继续执行独立工作，到真正需要结果时才使用局部等待机制。普通消息也不应默认触发中断。Tile 依赖、任务唤醒、中断是三个不同的本地投递目标。

## 10. 与 PTO ISA 和乱序执行的衔接

当前 PTO 主线 `42067d0f2890b8c013a868bd1247ebc6437b3179` 的 [BSE/BWE/BWI/BWT handler](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/scalar/model/sys/semantics.asl#L275)只给出非阻塞调度请求交接，尚未定义本文的 mailbox、订阅和唤醒协议。[SYNC.CORE](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/arch/state/core-synchronization.asl#L4)只覆盖一个 Core 内的四个 PE，不直接替代跨核同步服务。

Linx 的 [MSGB 设计记录](https://github.com/LinxISA/linx-isa/blob/a30de2ad8ba79f9c39b022ad94622e86c2310afc/docs/isa/register/ssr/MSGB.md)提供 SSR 消息接口线索；当前 canonical v0.58 未闭合本文所需协议，不能把历史页面直接当作 PTO 新语义。

建议新增的是明确的系统接口合同，先不决定新增助记符：

1. 定义消息暂存／提交／状态／接收／确认的 SSR 行为与访问权限。
2. 定义事件目标、pending 状态、检查与等待的原子性以及 BWE 的可观察行为；BWI 仍与中断路径区分。
3. 把 READY 映射到本地依赖 token，并绑定 invocation、动态请求和目标 Tile generation。消息不直接引用远端不可理解的物理 ROB／寄存器编号。
4. 仅当一个操作的所有相关条件满足时解除依赖。L1_READY 不得直接冒充 L0 Tile ready，消息到达也不得自动使 Tile payload 有效。
5. 错路径请求不得产生不可撤销报到／发送。已接纳请求的重试保持身份；squash 只能取消可撤销部分，不能假装已发出的通知或数据从未发生。
6. ABORT 保留已发生效果并唤醒相关等待者报告错误；排空旧数据访问后才允许窗口和身份复用。READY 与 ABORT 竞争时由 owner 顺序裁决，已经授权的读取必须安全结束或被确认取消。

本地 token 等待应只约束对应执行上下文或依赖操作；是否允许更多独立指令继续发射属于具体微架构，但不能要求所有 PE 一起进入 BWE 才推进同步。保持现有单层执行架构，不建立嵌套指令体或隐式 body 队列。

## 11. 可扩展实现与进展条件

“同步器集中计数”建议解释为每个 key 由唯一逻辑 owner 裁决。物理上可以按区域、bank 或 key 分片；不要求整片 WSE 只有一个中心表。具体是否由 SHA、SSA 两级承担，需补充实际模块定义。

同步表覆盖活动窗口，不覆盖每个 L1 字节或所有静态 Tile。waiter 集合可以采用有限列表、分组或分层 fan-out。广播能力不意味着每次 READY 都要广播整片芯片。

容量估算应计入贡献去重、消费者义务、通知重试和回收记录。表满时在接纳前背压；已经接纳的对象必须保留完成与释放所需资源。需要为回执、错误和释放保留进展通路，并保证通知处理不依赖一个已经等待该通知的线程。

对象进入 BUFFER_FREE 仅表示其数据存储可复用；同步记录的 RETIRE 还需要防止旧消息、未完成响应和重复请求命中新记录。可以采用退休记录、受控排空或不复用的 incarnation 身份；具体方式及有限 epoch 回绕约束待定。

## 12. Calendar 前端与编译器的变化

前端 Group、集合结果、逻辑 block_id、CommEvent 与 deps 语义保持。编译器增加通信依赖计划：同步 key、贡献集合、消费者义务、目的可见性域、Buffer 释放条件及消息请求身份。

`TCalendar<Plan>` 仍是阶段封装。原 MTE3/MTE4 接口替换为目标数据搬运合同；BSP 组就绪可由独立 ARRIVE/GROUP_READY 服务承接，但前提是其成员和完成条件已定义。DATA_READY 不等于 BSP，BUFFER_FREE 也不等于最终集合结果完成。

AllToAllV 仍先交换 counts，再决定有效 payload 与接收计划。运行期生成的期望集合需要由权威配置动作建立，随后才开始相应 payload 阶段；WAIT 不得一边订阅一边改变期望数量。

完整 CommEvent 覆盖该集合要求的源读取、接收可见及本地计算结果。是否还覆盖供下一轮复用的 BUFFER_FREE 应由接口分别表达，不能让结果可读与存储可覆盖共用一个含糊的完成标志。

## 13. 验证清单与待决事项

以下是候选验证点，尚未执行协议仿真或硬件测试。

| 场景 | 应观察到的结果 |
| --- | --- |
| DONE 全部先于 WAIT | 晚订阅的有效消费者仍收到 READY |
| WAIT 与最后一个 DONE 同时到达 | 不丢通知、不产生两个消费义务 |
| 重复 DONE／READY／RELEASE | 计数、授权和释放均不重复生效 |
| 未声明贡献或配置冲突 | 报错且不提前 READY |
| 两个生产贡献、三个消费者 | 两份贡献触发 READY，三份读义务归还后才可能 FREE |
| C0 已释放、C1 尚未订阅 | 保留 C1 义务，禁止覆盖旧 Buffer |
| generation 跨轮复用、旧通知迟到 | 旧消息不改变新对象状态 |
| HSCB 先到而目的 payload 未可见 | 不允许消费，不能把消息到达当作数据完成 |
| multicast 仅一份副本完成 | 不提前授权其他副本消费者 |
| L1→L0 DMA 尚未读完 | 不释放相应 L1 源窗口 |
| 网络归约重发同一贡献 | 数据面不重复归约；只在同步层去重不足以通过 |
| 表满、通知队列满、消费者处于 BWE | 背压可恢复，完成与释放仍能推进 |
| Abort 与 READY、RELEASE 竞争 | 成功与失败不混淆，不提前复用，不污染下一 invocation |
| 多 PE 同时操作 MSGB | 报文不混合，响应只匹配正确请求 |

需要继续确定：SHA/SSA 的实际模块职责；HSCB 的投递及顺序保证；静态消费者基线是否覆盖目标负载；一个同步对象的可见性域；本地 token 与 BWE 的编码边界；对象退役及 epoch 回绕方式；活动表规模及通知吞吐。

发送采用 SSRSET MSGBD 后 BSE 原子提交；接收 ACK 明确采用 SSRSET MSGBACK 写入精确 RX token。

本提案不改变 PTO/Linx 当前发布规范。声明与参考实现保存在 docs/proposals 下，不由 make install 安装，也不从正式 TileOP umbrella header 导出。

## 14. C intrinsic 接口草案 v0.3

声明文件：[sync_intrinsics.h](sync_intrinsics.h)。这是纯 C 接口设计，可供 C11/C++17 做语法检查；没有编译器 lowering、设备实现或可链接运行库。`__sync_*` 为拟议的标量消息／同步 intrinsic 名称，`WSE_DEVICE` 由目标工具链定义。结构体布局是本地源级接口，不是 HSCB 线格式，也不承诺最终二进制 ABI。

风格参考 CUDA 的 [Device-Callable APIs and Intrinsics](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/device-callable-apis.html) 和 [Asynchronous Barriers](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/async-barriers.html)：使用具名 intrinsic、对象／token 分离、提交与 test/wait 分离，并在接口上明确完成范围。WSE 使用自己的 key 和生命周期，不复制 CUDA 的 phase 有效期或线程集合范围。

接口分层与参数顺序参考 **Linx-TileOP-API 默认分支 linx，提交 ad5a2310664041be1f2fad5be1dd8e9d682f23df**。命名采用本次讨论确定的规则：TileOP 保持大写，标量消息与同步 intrinsic 使用小写 `__sync_*`。核查的是远端提交，未采用本地较旧或未提交的文档作为接口事实。

| 仓库已有层次 | 实际名称或形式 | 本草案的对应规则 |
| --- | --- | --- |
| 公开 Tile 操作 | `TLOAD(dst, src)`、`TSTORE(dst, src)`、`TMOV(dst, src)`、`MGATHER(dst, src, offset)` | TileOP 保持大写；输出在前的参数顺序可借鉴 |
| 操作变体 | `TLOAD_ASS`、`TLOAD_SLOT`、`MGATHER_MASK` | TileOP 变体保留原拼写；标量扩展使用小写 async／acquire／release 后缀 |
| 公开查询辅助函数 | `get_thread_idx()`、`get_thread_id()` | 保留现有辅助函数风格，不重命名仓库接口 |
| 编译器 builtin | `__builtin_linx_get_thread_idx()` | builtin 与公开 API 分层；不声明尚不存在的消息 builtin |
| 实现层 | inline/template、扩展内联汇编、`*_Impl` | C API 不要求一对一对应新 opcode，lowering 仍待定义 |

证据：[公开 forwarding API](https://github.com/LinxISA/Linx-TileOP-API/blob/ad5a2310664041be1f2fad5be1dd8e9d682f23df/include/common/tileop_api.hpp)、[Linx 操作封装](https://github.com/LinxISA/Linx-TileOP-API/blob/ad5a2310664041be1f2fad5be1dd8e9d682f23df/include/jcore/template_asm.hpp#L2897)、[线程查询与 builtin](https://github.com/LinxISA/Linx-TileOP-API/blob/ad5a2310664041be1f2fad5be1dd8e9d682f23df/include/common/pto_tileop.hpp#L19)。在该提交的 include/ 中未检出 RecordEvent、WaitEvent 或 MSGB/BWE/SSRGET 包装 API，不能宣称已有兼容事件实现。

本版函数统一使用 `__sync_*` 前缀：对象操作为 `__sync_init_async`、`__sync_subscribe_async` 等；组、任务、资源及本地 token 操作分别为 `__sync_group_*`、`__sync_task_*`、`__sync_resource_*`、`__sync_request_*`、`__sync_event_*`。这些属于标量控制接口，采用小写；现有 TileOP 不改名。该前缀是新设计，尚非上游已有接口。

保留输出参数前置：一个输出时该输出最前；同时返回条件事件和请求 token 时，event 在前、request 次之，再跟输入。QUERY 的 snapshot 在前、request 次之。函数命名保持小写 `__sync_*`。v0.3 在应用收件信息中明确区分 application_message_id 与 transport_message_id，ACK 使用的 RX token 由低层管理。不保留旧函数名别名。

### 14.1 类型与统一约定

| 类型 | 含义 |
| --- | --- |
| `wse_sync_key_t` | domain、invocation、object、generation、visibility_domain 的完整对象身份 |
| `wse_endpoint_t` | 逻辑任务、PE、消息端口；部署层映射到物理端点 |
| `wse_window_t` | 平台预先绑定的 L1 窗口句柄，包含隐含的访问权限和存储版本；不是远端裸指针 |
| `wse_request_t` | 一个已接纳命令的处理结果 token；例如 DONE 被同步 owner 接受 |
| `wse_event_t` | 一个完成条件 token；例如 DATA_READY、BUFFER_FREE、某段 L1 的可见性或读取结束 |
| `wse_service_t` | 同步服务上下文，供 owner 的固件／服务接口使用 |

`wse_request_t` 与 `wse_event_t` 不可互换。每个 token 是本地、上下文绑定的不透明值，零值无效。事件来源与绑定对象由端点校验，不能用任意整数冒充完成证据。

命名使用 `_async` 表示非阻塞提交、`_test` 表示单次查询、`_wait` 表示等待、`_release/_acquire` 表示对应的顺序边界。`__sync_release_async` 中 release 还表示归还消费义务，其具体语义以该接口合同为准。

所有提交函数返回 `wse_status_t`：`WSE_OK` 只表示本地接纳；`WSE_WOULD_BLOCK` 表示资源暂不足且未接纳；非法、过期或权限错误可能立即拒绝，也可能在远端处理阶段返回。拒绝时输出 token 不变，没有该次远端副作用。已接纳请求由实现按原身份重试，调用者不能用重新提交来模拟等待。

输入元数据及嵌套数组在 request 到达终态前保持有效且不修改；异步输出 snapshot 的存储保留到 request 终态，成功完成后才可读取。输出 token 指针只在调用期间写入，不被异步借用。输入 payload 是设备可读地址，不能传入未经绑定的 host 指针。发送消息大小受目标能力限制，超限明确报错，不自动转换成大型 NoC 搬运。

提交接纳前预留 request/event 状态。若同时返回 event 和 request，远端拒绝或依赖失败也必须使对应 event 进入错误终态，不能永久悬挂。已经成功完成的事件不被后来的取消改写为另一个结果；已经授予的读义务通过排空和释放结束。

### 14.2 每类消息的 C 接口

下表覆盖 §9 的全部消息。完整参数类型在头文件中；本表中的签名省略统一的 `WSE_DEVICE wse_status_t` 前缀。

| 消息 | C intrinsic | 参数与完成合同 |
| --- | --- | --- |
| INIT | `__sync_init_async(&req, desc)` | `desc` 固定 key、region、贡献及消费者集合；req 成功表示 owner 已建立本代对象。重复相同配置幂等，冲突配置拒绝 |
| QUERY | `__sync_query_async(&snapshot, &req, key)` | req 成功后得到 owner 的一个一致快照。快照 READY 不授予读义务，也不替代 DATA_READY 的 acquire |
| RETIRE | `__sync_retire_async(&req, key)` | 仅 owner 可回收；仍有数据引用或协议义务则返回 BUSY，不覆盖活动记录 |
| SUBSCRIBE／WAIT | `__sync_subscribe_async(&ready, &req, key, consumer_slot)` | req 表示订阅处理结果；ready 表示该消费者的 DATA_READY。consumer_slot 必须属于既定消费者集合 |
| DONE | `__sync_done_release_async(&req, key, contribution_id, visible)` | visible 必须覆盖正确目标窗口及本次贡献的可见性。待完成时延后发 DONE；失败时不计数。req 成功只代表本贡献已接受，不代表所有贡献已齐 |
| DATA_READY | `__sync_ready_async(&req, service, notification)` | 同步服务侧发出；notification 引用已存在的订阅。服务验证 READY 和读义务后投递，不能由普通 kernel 任意宣告 |
| RELEASE | `__sync_release_async(&req, key, consumer_slot, reads_done)` | reads_done 覆盖该消费义务的实际读取完成。接纳后调用者不得新增该版本读取。req 成功表示归还已处理，不等价于全局 BUFFER_FREE |
| BUFFER_FREE | `__sync_free_async(&req, service, notification)` | 同步服务侧校验释放与排空条件后投递；生产者通过 watch_free 得到对应事件 |
| ARRIVE | `__sync_group_arrive_async(&group_ready, &req, group, member_id, prior_done)` | prior_done 为可空的事件指针，非空时覆盖本成员应先完成的工作；NULL 表示纯到齐且不承诺 payload 可见。req 成功表示报到接受，group_ready 表示全组条件满足 |
| GROUP_READY | `__sync_group_ready_async(&req, service, notification)` | 同步服务侧向已报到成员的事件槽投递，匹配精确 group generation；不承诺业务 payload 自动完成 |
| TASK_NOTIFY | `__sync_task_notify_async(&req, message)` | message 包含 destination、稳定 application_message_id、task_tag、payload 与长度；req 成功表示目的 mailbox 已接纳，不表示任务执行结束 |
| RESOURCE_RETURN | `__sync_resource_return_async(&req, resource, use_done)` | 按 resource_id＋lease_id 幂等归还一个完整资源授权；use_done 保护实际使用结束。基线不支持裸计数加 N 或部分归还 |
| ABORT | `__sync_abort_async(&quiescent, &req, key, reason)` | req 成功表示失败转换被接受；quiescent 成功表示旧访问已排空。两者分开，不能收到 abort ACK 就覆盖 Buffer |
| ERROR | `__sync_error_report_async(&req, error)` | error 包含 key、report_id、code、detail，交给对象 owner 处理。诊断上报本身不自动清除计数或回收 Buffer；不可恢复错误由 owner 发起失败转换 |

`DATA_READY`、`BUFFER_FREE`、`GROUP_READY` 的发送声明只在 `WSE_SYNC_SERVICE_API` 打开时可见，用于同步器固件／参考实现。该宏只管理声明可见性；实际权限及状态检查仍由 backend 执行，不能靠宏实现授权。

通知重试保持 notification_id，复用已记录结果，不因对象后来推进而创建新的读取授权。ARRIVE 的 prior_done 非空时按值捕获该事件并保留内部引用，该指针存储仅需在调用期间有效；这个可空参数不能推广到 DONE 的 visible 参数。

未增加普通 kernel 的“直接 SET_READY”入口。这样生产者提交贡献，消费者订阅结果，同步 owner 才决定条件是否满足，保留前文的角色边界。

### 14.3 配套操作：等待、查询与释放 token

```c
wse_status_t __sync_request_test(wse_request_t request);
wse_status_t __sync_request_wait(wse_request_t request);
wse_status_t __sync_event_test_acquire(wse_event_t event);
wse_status_t __sync_event_wait_acquire(wse_event_t event);
wse_status_t __sync_request_release(wse_request_t request);
wse_status_t __sync_event_release(wse_event_t event);
```

test 返回 PENDING 表示尚未完成且不建立 acquire；OK 表示成功，其他终态状态表示失败。wait 返回成功或失败，不能把失败伪装为就绪。等待只约束发出调用的执行上下文；独立消息引擎需要继续推进。具体是否降低为 BWE 或短轮询，由目标实现决定，但不能丢失先于 wait 到达的事件。

request wait 保证本命令结果及输出元数据可读；event acquire 只保证该事件规定的区域和范围，不是所有 Core 的全局 fence。重复 test/wait 不消费事件，也不延长或重新获得已经归还的读义务。

request/event release 仅释放本地 token，不发送 §7 的消费者 RELEASE。仅允许对终态 token 且没有本地观察者并发使用时执行；否则返回 BUSY。成功 release 后调用方 token 及其所有值拷贝失效，不得再使用。已接纳的后续命令可以持有事件的内部引用，局部 token 释放不得破坏这些引用。所有本地通知槽与远端退役义务需完成关联清理；GROUP 对象在成员通知／事件回收完成前不能 RETIRE。

两个额外配置入口：

```c
wse_status_t __sync_watch_free_async(
    wse_event_t *free_event,
    wse_request_t *request,
    const wse_sync_key_t *key);

wse_status_t __sync_group_init_async(
    wse_request_t *request,
    const wse_group_desc_t *desc);
```

watch_free 使用 SUBSCRIBE 服务订阅 BUFFER_FREE，不占消费者读义务。group_init 是 INIT 的 GROUP 对象形式；成员列表非空且唯一。DATA 与 GROUP 共用 key 命名空间但对象类型不同，不能混用 init、subscribe 或 arrive。group/query/retire 使用相同的通用对象管理入口，类型不匹配时明确拒绝。

### 14.4 数据引擎完成事件与通用收件箱

DONE 不接受布尔值 `is_ready=1`，而接受来自数据引擎的 event。L1 本地操作可以通过以下候选适配入口产生事件：

```c
wse_status_t __sync_l1_publish_async(
    wse_event_t *visible,
    wse_window_t window,
    uint64_t offset,
    uint64_t bytes);

wse_status_t __sync_l1_reads_complete_async(
    wse_event_t *reads_done,
    wse_window_t window,
    uint64_t offset,
    uint64_t bytes);
```

这两者是本地数据引擎适配，不发送 HSCB 消息；只返回 event，不另外返回 request。输入参数按值捕获，输出 token 存储只在调用期间使用；窗口绑定与实际存储保持到事件终止及相关读义务结束。接纳时捕获发起上下文先前在该范围的匹配访问，形成封闭依赖集。它们不能证明其他 PE 未纳入的写入、未知远端传输或后来新提交的访问已经完成。多 PE 生产需要先由相应操作聚合所有必要访问。跨 Core 的目的提交事件由 NoC 接收／确认路径提供；本地 publish 不能冒充它。

通用应用消息使用原子接收接口：

```c
wse_status_t __sync_message_try_recv(
    void *payload,
    wse_message_info_t *info,
    uint32_t capacity_bytes,
    uint32_t port);
```

成功时复制并弹出同一完整消息；空队列返回 PENDING；容量不足返回 BUFFER_TOO_SMALL 并写回所需长度，但不弹出消息。TASK 的 payload 是发送者提供的字节串；RESOURCE/ERROR 使用各服务定义的编解码格式，不能把 C 结构体原样当线格式。DATA_READY／BUFFER_FREE／GROUP_READY 投递到依赖事件引擎，不进入这个应用收件箱。

普通 TASK_NOTIFY 不隐含其 payload 中所携带地址的数据已完成。若消息指向一块异步产生的数据，应携带对应 sync key 并执行数据订阅；application_message_id 只提供应用消息身份，不能替代数据就绪。

### 14.5 正常路径调用示例

下面是接口调用顺序示意，`key`、窗口及身份由计划预绑定，数据引擎提供匹配的完成事件。每个提交调用都要检查返回值；仅在返回 OK 后使用输出 token。为突出协议关系，示意代码省略失败清理分支，不作为可运行样例交付。

```c
/* 配置 owner：先建立配置并确认成功，随后允许生产者/消费者活动。 */
__sync_init_async(&init_req, &desc);
__sync_request_wait(init_req);

/* 消费者：先订阅，可以继续处理不依赖此对象的工作。 */
__sync_subscribe_async(&ready, &sub_req, &key, consumer_slot);
__sync_request_wait(sub_req);             /* 订阅已经接受，不是数据已就绪 */

/* 生产者/接收完成代理：visible 来自对应目的窗口的数据引擎。 */
__sync_done_release_async(&done_req, &key, contribution_id, visible);
__sync_request_wait(done_req);            /* 本份贡献已接受 */

/* 消费者：结果就绪后才启动对应 L1→L0 读取。 */
__sync_event_wait_acquire(ready);
/* 提交实际 TLOAD/读取；reads_done 覆盖本次消费义务的所有读取。 */
__sync_release_async(&release_req, &key, consumer_slot, reads_done);
__sync_request_wait(release_req);         /* 本消费者归还，不是所有消费者归还 */

/* 窗口 owner：free_event 可在本轮早期通过 watch_free 订阅。 */
__sync_event_wait_acquire(free_event);    /* 此后才允许覆盖对应旧窗口 */
```

完整代码还需对每个 request/event 的终态做检查并释放本地 token。READY 成功后发生的后续 abort 仍按既有读义务排空；不能因等待曾经成功而忽略新的失败控制。

### 14.6 到 SSR／ISA 的候选映射

| C 层操作 | 候选 lowering | 仍需确定的合同 |
| --- | --- | --- |
| `*_async` 命令提交 | 写 MSGB 数据／描述符，执行原子提交动作 | SSR 地址、报文捕获点、队列满行为、精确请求身份 |
| request/event test | 读端点完成表或状态 SSR | 查询副作用、状态和版本一致性 |
| event wait | 原子检查／挂起，必要时使用 BWE | sticky completion、等待 token 选择、异常恢复 |
| 同步服务通知 | SHA/SSA 已满足条件后经 HSCB 分发 | owner 权限、重试、端点匹配和 fan-out |
| message try_recv | 锁定消息快照，读取后原子确认 | 多 PE 所有权、短缓冲拒绝、并发接收 |

这一版选择先稳定 C 语义，再决定 SSR/BSE/BWE 的具体扩展。接口中的 release/acquire 不能仅按名字机械替换为现有 FENCE.D；需明确数据引擎、可见性范围以及内存模型之间的连接。

### 14.7 本版检查范围

2026-11-07：声明头文件在 C11、C++17 下分别以应用接口和打开 `WSE_SYNC_SERVICE_API` 的服务接口通过 Clang `-Wall -Wextra -Werror -pedantic -fsyntax-only` 检查；14 类消息均有接口，附录引用名称与头文件声明一致。独立设计审查已覆盖事件失败传播、token 生命周期、纯到齐的空前置依赖及通知幂等性。上述结果仅验证声明语法和设计一致性，不证明设备执行、协议活性、硬件顺序或性能；正常路径示意代码不是可运行测试。

v0.2 统一为小写 `__sync_*` 后，重新通过上述四组语法检查，并核对 25 个声明、附录名称及异步接口输出位置。协议语义沿用上一版审查结果；未运行 Linx 目标代码生成或设备执行。

v0.3 将 ACK 固定为 SSRSET MSGBACK，新增 [ID／寄存器字段与状态机](msgb-ssr-ack.md) 和 [ACK 可执行参考](reference/msgb_ack.h)。C 接收信息现在分别携带 application_message_id 和 128 位 transport_message_id；64 位 RX token 仅用于本次快照的 ACK／RELEASE。`make check-sync-proposal` 同时运行声明检查、通用收发参考和 ACK 寄存器模型，覆盖 C11/C++17。模型的单队列槽是测试边界，不代表硬件容量。
