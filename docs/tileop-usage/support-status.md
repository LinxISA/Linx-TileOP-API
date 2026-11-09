# PTO ISA 0.59.0 wrapper 支持状态

本表以 `contracts/pto-isa-0.59.0-tile-operations.json` 的 119 个
`accepted-direct-operation` 为操作清单。它描述 Linx TileOP C++ wrapper 的
公开实现状态，不替代 PTO-ISA/pto-spec 的 ASL/NDF 合法性、fault、completion、
rollback、definedness 或 memory-order 语义。

| 状态 | 当前含义 |
| --- | --- |
| **已实现** | 当前 `include/` 有可调用 wrapper；本地测试覆盖其至少一个有效路径。 |
| **仅部分 layout 支持** | wrapper 存在，但只覆盖 catalog 合法 layout/shape/属性组合的子集；其他组合必须以编译期约束拒绝，不能推断为缺陷或自动支持。 |
| **尚无 wrapper** | PTO catalog 有操作，但本仓库没有公开的 C++ wrapper；不能通过拼接汇编或复用相近 API 冒充支持。 |
| **仅历史参考** | 名称来自旧版本或已删除清单；保留页面/禁用 stub 只用于迁移，不能生成当前 PTO 0.59.0 指令。 |

## 当前覆盖

| PTO catalog 范围 | 状态 | 说明 |
| --- | --- | --- |
| TEPL elementwise、scalar、reduce/expand、irregular | 已实现 | 逐操作页面和 `jcore/template_asm.hpp` 为 wrapper 权威；dtype、valid region、padding 和 selector 仍受 ASL 约束。 |
| `TEXPDIF` | CPU simulator、JCORE inline-asm 已实现；LLVM MC catalog 已实现；LLVM compiler lowering 尚未完成；AArch64/SME 不支持 | PTO 0.59.0 selector 29。已有 wrapper 不等于 LLVM compiler 已具备从 IR/builtin 到 TEXPDIF 的端到端 lowering。 |
| TLSU `TLOAD`/`TSTORE`/`TMOV`/`TPREFETCH`/`GMOV` | 已实现 | 传输、peer move 和 prefetch 有独立 C++ 入口；layout conversion 仅对页面列出的组合开放。 |
| `TMOV_SHARED_ND_TO_CUBE` | API 已实现；要求 LLVM 支持 TMOV Layout 21/22 | Shared ND 完整 parent 或显式连续整行范围转 Local M32/M16；详见 [TMOV](tlsu/load-store-move/TMOV.md)。 |
| TLSU `MGATHER`/`MSCATTER` 及 atomic/reduction variants | 仅部分 layout 支持 | wrapper 覆盖当前公开 selector；ColumnMajor selector、mask/索引 dtype 和不同 atomic 组合不得由普通 gather/scatter 重载推断。 |
| CUBE TMATMUL/TGEMV 及 bias/acc/MX variants | 仅部分 layout 支持 | CUBE_M16/CUBE_M32、Shared physical shape、scale layout 和 InternalAcc 由具体 wrapper/操作页限制。 |
| TEPL layout/rearrangement（`TPERMUTE`、`TSHUF`、`TPACK`、`TUNPACK`、`TGPR2T`） | 仅部分 layout 支持 | 只覆盖已实现的 CUBE/Local 路径和控制字段；未列出的组合保持拒绝。 |
| 0.59.0 `MGATHER`/`MSCATTER` family | 已适配 | 0.59.0 removes the GM row-stride operand and interprets index tiles as explicit byte displacements；wrapper 与 contract test 已改为 base-only `B.IOR`。 |
| 0.59.0 `TROWEXPAND*`/`TCOLEXPAND*` roles | 已适配 | Source roles 使用 row-broadcast/column-broadcast operation view；wrapper 约束、注释及操作文档已同步。 |
| `deleted_names`、`rejected_names`，以及 `TTRANS`/`TFILLPAD` 等历史页面 | 仅历史参考 | 不属于 active catalog；页面必须保留明确的 retired 标识。 |

## 版本边界

### main 合同与可选扩展边界

规范来源与 LLVM 依赖固定在 `contracts/pto-isa-0.59.0-sources.json`。
活动 catalog 固定到 PTO-SPEC main `42067d0f2890`。#424 尚未合入 main；按开发策略继续保留范围 API，
其合同独立固定到 PR 提交 `1981bceffa82`，不能把范围路径宣称为 main 合同的一部分。

| 接口 | 实现边界 | 编译与执行限制 |
| --- | --- | --- |
| `TLEA` | JCORE；Local RowMajor/CUBE_M32；S32/S64→S64、U32/U64→U64；位宽 8/16/32/64 | 无 GM 访问；不支持 Shared、CUBE_N8/M16 b64、packed dtype；CPU simulator/AArch64 尚无 wrapper |
| `TCI`/`TCI_2D` | JCORE；S16/U16/S32/U32/S64/U64；RowMajor 或已有 CUBE 专用入口 | M16 b64 仍拒绝；CUBE API 的 location/静态有效区限制保持生效 |
| `predicate::compare_scalar` | Local VEC M16/M32，静态有效区，FP32/S32/U32 | 私有 PredicateCell carrier；不接受任意 Numeric U8 |
| Predicate 逻辑、select、subview、concatenate | M16/M32；逻辑结果为 U8-basis PredicateCell；混合 Numeric 的结果为 Numeric U8；select payload 为 FP32/S32/U32 | CELL 对齐 view；拼接仅闭合的两个完整 writer；具体容量限制见 predicate-adapters |
| Predicate scatter/atomic_add | 仅 M32；FP32/S32/U32 values，S64/U64 byte offsets；相同静态有效区 | atomic_add 需要 `ew_mscatter_add_predicate` builtin；生产与消费在优化、内联区域内 |
| `TSTORE_ATOMIC_*` | Local RowMajor/M16/M32；8 种 reduction 各自限定 dtype | 不支持 CUBE_N8；Shared、mask、dtype 以实际重载约束为准；无新 TileOp/function |
| CUBE→Shared ND / Shared ND→CUBE | M16/M32；精确容量与物理列约束；显式连续整行 Shared ND subview | Shared parent 完整发布；不作数值转换；LLVM 必须支持相应 Layout |
| `LocalCubeAssembly` | 静态 M32 writer、顺序 INIT/MIDDLE/LAST、闭合后可读取 | 1..16 writers，具体容量限制见 b-subview-b-assemble；需要 LLVM Shared handle identity 修复 |
| `IndexedByteRange` | 全部 23 个 indexed TLSU wrapper；有符号 i64 字节边界 | PR #424 开发扩展；LLVM `b19e8c0e184c` 的 `R` 约束及 lowering；未知范围使用原重载 |

Predicate 跨调用 ABI/spill、GPR 自动适配、与 element-if 隐式 mask 的桥接尚未提供。
对象生成、汇编往返不等于模型数值或硬件验收。既有 multi-PE runtime、
participant mask 与 hosted adapter 保留；本次不宣称完成 #415 的外部启动 ABI 验收。
#415 的 worker 启动、每 PE 私有栈、PE0 共享初始化、共同 generation-zero SYNC、异常退出与重启
由外部 compiler-rt/platform 提供；当前头文件与 hosted adapter 不构成完整启动 ABI 验收。
详见 [PredicateCell adapters](predicate-adapters.md)、[范围参数](indexed-tlsu-range.md)。

### 已发布基线

- 当前 API/spec 基线：PTO ISA `0.59.0`，publication `0.59.0.0`，ABI
  `pto-isa-0.59.0-mode-function-v1`。
- backend 能力必须分别判断：CPU simulator/JCORE wrapper、LLVM MC
  assembler/parser、LLVM compiler lowering 以及 AArch64/SME 并不是同一个状态。
- `TEXPDIF` 当前已有 CPU simulator 实现、JCORE inline-asm wrapper 和 LLVM
  MC catalog 条目；LLVM compiler lowering 尚未完成，因此当前不能宣称 LLVM
  端到端支持。
- `docs/tileop-usage/migration/pto-0583-migration.md` 是历史迁移材料；其中的
  0.58.3 compiler、ELF identity 和旧 engine projection 不表示当前实现基线。
- 需要核对精确 fault ordering、completion、rollback、memory ordering 或编码位域时，
  必须回到 `PTO-ISA/pto-spec` 对应 ASL/NDF owner；Markdown wrapper 文档不构成完整
  规范镜像。

### 编译器组合的已知限制

LLVM `b19e8c0e184c` 已验证本次新增 API 与范围扩展的对象生成，但并非全部
既有多 PE 接口的兼容构建：该提交不提供 `__LINX_PTO_PE_DOMAIN__`，也不接受
既有 TileArray transport 使用的 `.reuse` 语法。修改前 `1153ea6477` 也能复现
同一组 3 项 native participant-mask 测试失败。多 PE 应用仍需同时具备这些能力的
LLVM 构建；本仓保留其实现与测试，不以本次范围扩展验证替代多 PE 验收。
