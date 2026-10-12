# PE runtime 与 Core 同步

本页说明 TileOP API 与 PTO-ISA 规范之间的 PE 启动合同。规范语义以
[PTO-ISA/pto-spec commit `3c1657c43d212d0cd8a2c3c017da4ad42bec0112`](https://github.com/PTO-ISA/pto-spec/tree/3c1657c43d212d0cd8a2c3c017da4ad42bec0112)
为准；本页不把 Linux CPU、CPU simulator 或 pthread 数量当作 PTO PE context 数量。

## PE 身份

`get_thread_idx()` 和 `get_thread_id()` 返回当前 PE 的 `THREAD_ID`。按照当前
PTO-ISA 规范，`THREAD_ID` 是只读 SSR，地址为 `0x0026`，合法值为 `0..3`：

```cpp
#include <common/pto_tileop.hpp>

uint32_t pe_id = get_thread_idx();
// pe_id 必须为 0、1、2 或 3
```

两个函数是 API 别名，不会创建 context，也不会启动其他 PE。

## `SYNC_CORE()`

`SYNC_CORE()` 对应 `SYNC.CORE`，用于当前 Core 中 PE0、PE1、PE2、PE3 的固定集合
rendezvous：

```cpp
// 只有在四个 PE 都执行到同一个静态同步点时才允许调用
SYNC_CORE();
```

调用合同如下：

- 四个 PE 必须以相同的动态顺序到达同一个 `SYNC_CORE()`；
- `PE_MASK` 不会改变 `SYNC.CORE` 的参与者集合；
- 不得在分歧路径、只启动部分 PE 或单 context 进程中调用；
- API 会在发出硬件指令前检查 runtime context 数和 `THREAD_ID`；
- context 数不是 4，或 `THREAD_ID` 不在 `0..3` 时，调用会进入 trap，不会执行
  `SYNC.CORE`。

因此，single-context 模式不会因为误执行 `SYNC.CORE` 而永久等待。

## 启动合同

普通 musl `_start`、`crt1`/`rcrt1` 和 TileOP 测试 `_start.s` 都是 single-context
入口，默认报告一个 context：

```text
__pto_runtime_pe_context_count() == 1
```

它们不会自动创建四个 PE，也不会设置四个 PE 的独立栈或 `THREAD_ID`。需要使用
`SYNC_CORE()` 的专用 launcher 必须在调用 kernel 前完成以下工作：

1. 创建四个相互独立的 PE context；
2. 为每个 context 设置独立的启动状态和栈；
3. 分别建立 `THREAD_ID = 0、1、2、3`；
4. 提供强符号 `__pto_runtime_pe_context_count()`，返回 `4`；
5. 让四个 context 从同一个 kernel 入口开始执行。

只有 launcher 明确满足上述合同后，`SYNC_CORE()` 才会发出硬件同步指令。仓库外
的 QEMU 或硬件 launcher 需要单独验证这些条件；Linux SMP、pthread、CPU hart 或
四次普通函数调用都不能替代四个 PTO PE context。

## 编译期 feature 检查

安装后的 `common/pto_tileop_api_revision.hpp` 提供：

```cpp
#if !PTO_TILEOP_API_HAS_SYNC_CORE
#error "SYNC_CORE is required"
#endif
```

该宏表示 wrapper 已提供 `SYNC_CORE()` API 和启动合同保护；它不表示运行时已经
启动了四个 PE。context 数量和 PE ID 仍由实际 launcher 在运行时建立并验证。
