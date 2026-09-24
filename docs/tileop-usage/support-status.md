# PTO ISA 0.58.6 wrapper 支持状态

本表以 `contracts/pto-isa-0.58.6-tile-operations.json` 的 117 个
`accepted-direct-operation` 为操作清单。它描述 Linx TileOP C++ wrapper 的
公开实现状态，不替代 PTO-ISA/pto-spec 的 ASL/NDF 合法性、fault、completion、
rollback、definedness 或 memory-order 语义。

| 状态 | 当前含义 |
| --- | --- |
| **已实现** | 当前 `include/` 有可调用 wrapper；本地测试覆盖其至少一个有效路径。 |
| **仅部分 layout 支持** | wrapper 存在，但只覆盖 catalog 合法 layout/shape/属性组合的子集；其他组合必须以编译期约束拒绝，不能推断为缺陷或自动支持。 |
| **尚无 wrapper** | PTO catalog 有操作，但本仓库没有公开的 C++ wrapper；不能通过拼接汇编或复用相近 API 冒充支持。 |
| **仅历史参考** | 名称来自旧版本或已删除清单；保留页面/禁用 stub 只用于迁移，不能生成当前 PTO 0.58.6 指令。 |

## 当前覆盖

| PTO catalog 范围 | 状态 | 说明 |
| --- | --- | --- |
| TEPL elementwise、scalar、reduce/expand、irregular | 已实现 | 逐操作页面和 `jcore/template_asm.hpp` 为 wrapper 权威；dtype、valid region、padding 和 selector 仍受 ASL 约束。 |
| TLSU `TLOAD`/`TSTORE`/`TMOV`/`TPREFETCH`/`GMOV` | 已实现 | 传输、peer move 和 prefetch 有独立 C++ 入口；layout conversion 仅对页面列出的组合开放。 |
| TLSU `MGATHER`/`MSCATTER` 及 atomic/reduction variants | 仅部分 layout 支持 | wrapper 覆盖当前公开 selector；ColumnMajor selector、mask/索引 dtype 和不同 atomic 组合不得由普通 gather/scatter 重载推断。 |
| CUBE TMATMUL/TGEMV 及 bias/acc/MX variants | 仅部分 layout 支持 | CUBE_M16/CUBE_M32、Shared physical shape、scale layout 和 InternalAcc 由具体 wrapper/操作页限制。 |
| TEPL layout/rearrangement（`TPERMUTE`、`TSHUF`、`TPACK`、`TUNPACK`、`TGPR2T`） | 仅部分 layout 支持 | 只覆盖已实现的 CUBE/Local 路径和控制字段；未列出的组合保持拒绝。 |
| catalog 0.58.6 active operation 且尚无对应公开重载 | 尚无 wrapper | 由 catalog alignment check 阻止静默漏项；需要新增 wrapper、测试和操作页后才能改为已实现。 |
| `deleted_names`、`rejected_names`，以及 `TTRANS`/`TFILLPAD` 等历史页面 | 仅历史参考 | 不属于 active catalog；页面必须保留明确的 retired 标识。 |

## 版本边界

- 当前 API/spec 基线：PTO ISA `0.58.6`，publication `0.58.6.0`，ABI
  `pto-isa-0.58.6-mode-function-v1`。
- `docs/tileop-usage/migration/pto-0583-migration.md` 是历史迁移材料；其中的
  0.58.3 compiler、ELF identity 和旧 engine projection 不表示当前实现基线。
- 需要核对精确 fault ordering、completion、rollback、memory ordering 或编码位域时，
  必须回到 `PTO-ISA/pto-spec` 对应 ASL/NDF owner；Markdown wrapper 文档不构成完整
  规范镜像。