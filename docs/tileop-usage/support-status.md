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
| TEPL elementwise、scalar、reduce/expand、irregular（除 `TLEA`） | 已实现 | 逐操作页面和 `jcore/template_asm.hpp` 为 wrapper 权威；dtype、valid region、padding 和 selector 仍受 ASL 约束。 |
| `TLEA` | JCORE inline-asm wrapper 已实现（RowMajor 与 CUBE_M32）；CPU simulator 未实现；AArch64/SME 未实现；LLVM MC / compiler lowering 分别判断 | PTO catalog 将 TLEA 列为 `accepted-direct-operation`（`BSTART.TEPL`，**VEC** 引擎，selector `0x2E`=46，function 14/mode 1）；操作数为 S32/U32/S64/U64 element index、per-PE element-width 标量与 S64/U64 byte-offset 目标。`include/jcore/template_asm.hpp` 的 wrapper 以编译期约束限定 dtype/layout/logical-shape（RowMajor/CUBE_M32，Shared/CUBE_N8/CUBE_M16 拒绝）；signed→S64/unsigned→U64 的严格配对由 ASL `lea-operands.asl` 要求，当前 wrapper 仅校验目标为 S64/U64，配对以指令 legality 为准。element-width 的运行时合法性（8/16/32/64）由指令 legality 检查，不是编译期静态拒绝。wrapper 存在不代表 CPU simulator、AArch64 或 LLVM 端到端已具备 TLEA 支持。 |
| `TEXPDIF` | CPU simulator、JCORE inline-asm 已实现；LLVM MC catalog 已实现；LLVM compiler lowering 尚未完成；AArch64/SME 不支持 | PTO selector 29。已有 wrapper 不等于 LLVM compiler 已具备从 IR/builtin 到 TEXPDIF 的端到端 lowering。 |
| TLSU `TLOAD`/`TSTORE`/`TMOV`/`TPREFETCH`/`GMOV` | 已实现 | 传输、peer move 和 prefetch 有独立 C++ 入口；layout conversion 仅对页面列出的组合开放。 |
| TLSU `MGATHER`/`MSCATTER` 及 atomic/reduction variants | 仅部分 layout 支持 | wrapper 覆盖当前公开 selector；ColumnMajor selector、mask/索引 dtype 和不同 atomic 组合不得由普通 gather/scatter 重载推断。 |
| CUBE TMATMUL/TGEMV 及 bias/acc/MX variants | 仅部分 layout 支持 | CUBE_M16/CUBE_M32、Shared physical shape、scale layout 和 InternalAcc 由具体 wrapper/操作页限制。 |
| TEPL layout/rearrangement（`TPERMUTE`、`TSHUF`、`TPACK`、`TUNPACK`、`TGPR2T`） | 仅部分 layout 支持 | 只覆盖已实现的 CUBE/Local 路径和控制字段；未列出的组合保持拒绝。 |
| `MGATHER`/`MSCATTER` family（byte-displacement 索引） | 已适配 | 自 PTO 0.58.7 起 GM row-stride 操作数被移除、index tile 解释为显式 byte displacement，该语义在 0.59.0.0 发布 catalog 中保持不变；wrapper 与 contract test 使用 base-only `B.IOR`。 |
| `TROWEXPAND*`/`TCOLEXPAND*` roles | 已适配 | Source roles 使用 row-broadcast/column-broadcast operation view；wrapper 约束、注释及操作文档已同步。 |
| `deleted_names`、`rejected_names`，以及 `TTRANS`/`TFILLPAD` 等历史页面 | 仅历史参考 | 不属于 active catalog；页面必须保留明确的 retired 标识。 |

## 版本边界

- 当前 API/spec 基线：已发布的 PTO ISA `v0.59.0.0` release（commit `b016532`），ABI
  `pto-isa-0.59.0-mode-function-v1`、encoding projection SHA-256
  `91509080…`、ELF release 字段 `0.59.0`（见 `contracts/pto-isa-0.59.0-tile-operations.json`、
  `contracts/linxisa-v0.58-engine-ops.json` 的 `pto_source` 与 provenance 测试）。
- 发布 manifest（`spec/release-manifest.json`）自身记为
  `specification_status: draft`，并把 implementation conformance 限定为
  `b-fpatr-and-tcvt-e8m0-asl-proven-other-numeric-domains-unproven`；本页据此
  不宣称该规范已最终定稿或全部数值域已证明。119 个操作的 Tile catalog 与 PTO 0.58.7
  development golden 逐字节一致（SHA-256 `533db9b8…`），因此 `TLEA` 等条目在两者之间保持一致，
  只是 encoding identity 移到了发布版本。
- 语义 authority 仍是 LinxISA v0.58 profile（无 0.59 profile）与 PTO ASL/NDF：
  engine-ops 投影保留真实 LinxISA v0.58 来源，并在 `pto_source.development_lock`
  记录其曾锁定的 PTO development golden，不把旧来源伪标为 v0.59。
- backend 能力必须分别判断：CPU simulator/JCORE wrapper、LLVM MC
  assembler/parser、LLVM compiler lowering 以及 AArch64/SME 并不是同一个状态。
  `TLEA` 目前只有 JCORE inline-asm wrapper（RowMajor 与 CUBE_M32），没有 CPU simulator 语义。
- `TEXPDIF` 当前已有 CPU simulator 实现、JCORE inline-asm wrapper 和 LLVM
  MC catalog 条目；LLVM compiler lowering 尚未完成，因此当前不能宣称 LLVM
  端到端支持。
- `docs/tileop-usage/migration/pto-0583-migration.md` 是历史迁移材料；其中的
  0.58.3 compiler、ELF identity 和旧 engine projection 不表示当前实现基线。
- 需要核对精确 fault ordering、completion、rollback、memory ordering 或编码位域时，
  必须回到 `PTO-ISA/pto-spec` 对应 ASL/NDF owner；Markdown wrapper 文档不构成完整
  规范镜像。
