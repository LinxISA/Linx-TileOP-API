# TileOP 辅助接口

辅助接口负责描述数据、构造操作数或协调发布，通常与基础 TileOP 一起使用。

## 分片与组装

| 入口 | 用法 | 约束 |
|---|---|---|
| `range::subview` | `auto src = range::subview<OffsetUnits, SizeUnits>(parent);` | 借用现有 Local Tile。范围遵守该布局的对齐规则，并且不得超出 parent 的有效范围或生命周期。由接受该 carrier 的 TileOP 消费。 |
| `range::assemble`、`range::assemble_middle`、`range::assemble_last` | 构造 INIT、MIDDLE、LAST 三阶段 destination。 | 按 writer 次序发射；必须在 LAST 闭合后读取 parent。使用操作页明确允许的 producer。 |
| `TileArray`、`TASSEMBLY<Parent>` | 向 `TileArray` 的 slot 写入数据，随后用 `TASSEMBLY<Parent>(std::move(parts))` 得到 parent。 | 单次组装最多 16 个 writer。选择满足粒度、容量和布局要求的 fragment。 |
| `TPARTVIEW<ElementsPerPart>` | `auto parts = TPARTVIEW<32>(parent, valid_elements);` | 借用 parent；用 `size()`、`part(index)` 和 `valid_size(index)` 获取分片和各片的有效长度。 |
| `TPARTELEMENT` | `TPARTELEMENT(part)` | 返回逻辑元素引用。使用已支持的 logical32 或 logical64 part 类型。 |
| `TREDUCEPREFIXVIEW<ViewTile>` | 借用 CUBE 行归约结果的第一个 CELL。 | 要求 `ValidCol=1`，视图的 dtype、布局和行数匹配，物理容量为一个 CELL。参见 [TROWSUM](ops/TROWSUM_zh.md)。 |
| `LocalCubeAssembly` | 将多个静态 Local CUBE writer 组装成 parent。 | 按 INIT、MIDDLE、LAST 顺序发布；必须在闭合后消费。 |
| `reinterpret_tile` | 使用另一种等宽描述借用同一数据。 | 不改变数值或布局；需要数值转换时用 [TCVT](ops/TCVT_zh.md)。 |

## 谓词与执行范围

| 入口 | 用法 | 约束 |
|---|---|---|
| `predicate::compare_scalar` | `auto pred = pto::predicate::compare_scalar<pto::CmpMode::GE>(basis, threshold);` | Local VEC M16/M32、静态有效区域；FP32/S32/U32 basis。 |
| `predicate::select`、`TSEL` | 使用类型化 predicate 选择两个输入 Tile。 | predicate 与 payload 须有匹配的逻辑有效区；不能把普通 U8 Tile 当作布尔 carrier。见 [TSEL](ops/TSEL_zh.md)。 |
| `predicate::GPRMask` | 8-bit CUBE `TSEL` 的两字掩码载体。 | 两个 `uint64_t` 按 low、high 的顺序提供。 |
| `IndexedByteRange` | 传入 `pto::IndexedByteRange{min, max}`，为 indexed TLSU 添加可验证的字节偏移区间。 | 对应独立的 PTO-SPEC #424 扩展。需要支持该扩展的编译器。常规 indexed TLSU 重载仍只绑定 GM base。 |
| `ElementTile<T, Elements>` | 和 `TPARTVIEW`、`TPARTELEMENT` 配合提供逻辑元素接口。 | C++ 存储 profile、编译器对 `element for` 的 lowering、目标执行支持分别确认。 |

## Shared、运行时与块操作

| 入口 | 用途 |
|---|---|
| `SharedTile<T>` | 一个 Core 内的共享 Tile，寄存器由编译器分配。生产者与消费者在同一函数或内联调用链中传递该对象。 |
| 核内 PE 协作 | 按平台启动约定就绪，使用对应同步阶段确保 Shared generation 发布和复用。 |
| [TIMG2COL](ops/TIMG2COL_zh.md) | 使用独立的块操作加载卷积窗口。 |

0.59 的 `SharedTile` 不能通过普通 C++ 函数的非内联调用或内存保存来跨越生命周期边界。编译器分配 Shared 寄存器，算子代码负责 PE 参与方式及数据发布、消费的顺序。

核内 PE 协作不能直接作为 WSE 芯片的核间通信或同步接口使用。核间行为取决于 WSE 的 runtime 合同。

## 矩阵后处理选项

`fixp::Options` 为 CUBE 矩阵操作指定转换、量化、激活或辅助输出。基础重载使用默认属性。按各操作页的约束选用相应辅助函数，例如 `fixp::keep_acc()` 保留累加器类型，`fixp::f16()` 和 `fixp::bf16()` 分别将输出转换为 FP16 和 BF16。

[返回操作索引](README.md)
