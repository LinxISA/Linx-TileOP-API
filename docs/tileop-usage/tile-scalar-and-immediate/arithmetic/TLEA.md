# TLEA

`TLEA` 将 Local 的逻辑元素索引扩展为 64 位，并按 `element_bits / 8` 显式缩放为字节偏移；
它只生成字节偏移，不加 BaseGPR、不访存、不产生内存事件、也不更新 numeric status。

> 规范权威：`PTO-ISA/pto-spec` 的
> `asl/tile/tile-scalar-and-immediate/arithmetic/TLEA.asl` 与
> `asl/tile/model/legality/lea-operands.asl`。本页是 wrapper 使用说明，不替代 ASL/NDF。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TLEA(tile_shape_out &dst, tile_shape_in &src, unsigned element_bits);
```

- 编码：TEPL Mode 1 / Function 14，selector `0x02E`（46），执行引擎 **VEC**。
- `src`：持久化 Local 索引 Tile，operation type 必须恰为 `S32`、`U32`、`S64` 或 `U64`，
  且选定 operation type 等于其 backing type。
- `dst`：新的 Local byte-offset Tile；有符号源对应 `S64`，无符号源对应 `U64`
  （裸 64 位整型不足以满足该配对）。
- `element_bits`：per-PE private-GPR 标量，表示元素宽度（bit 数，不是字节数）。
  其运行时合法值为 `8/16/32/64`，由指令 legality 检查；`0`（硬件零寄存器）是非法宽度。

### 支持的数据类型

支持索引 Tile 类型 `S32`、`U32`、`S64`、`U64`；目标 Tile 类型为对应的 `S64`/`U64`。

| 操作数角色 | 类型要求 |
| --- | --- |
| `src`（索引） | `S32`/`U32`/`S64`/`U64`，operation type 等于 backing type。 |
| `dst`（字节偏移） | 有符号源 → `S64`；无符号源 → `U64`。 |
| `element_bits`（标量） | 每 PE 的元素位宽，运行时取值 `8/16/32/64`。 |

## 使用要求

- `src` 与 `dst` 必须是 `Location::Vec`（持久化 Local）Tile；
- `src` 与 `dst` 的逻辑 valid shape 必须相同，物理 geometry/capacity 可以独立；
- 符号性配对（signed→S64，unsigned→U64）必须成立，不能只检查目标是任意 64 位整型；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

- 仅 `RowMajor` 与 `CUBE_M32` 是合法 layout；`Shared`、`CUBE_N8`、`CUBE_M16` 一律拒绝。
  本仓库 wrapper 目前只发射 `RowMajor` 形式，对 `CUBE_M32` 以编译期约束 fail closed
  （issue #371 double-CELL b64 映射尚待在 Linx 工具链上对照发布 ASL 验证）。
- 有符号索引 sign-extend 到 S64、无符号索引 zero-extend 到 U64，再乘以 `element_bits / 8`，
  保留低 64 位，不做饱和、不产生 overflow fault。
- `PE_MASK=0000` 在读取、分配和 fault 之前是严格 no-op。
- bundle 必须是一条终结的 Local `B.IOT`（一个索引源加重命名目标）加一条必需的
  `B.IOR`（在 `RegSrc0` 绑定 element width）；未使用的 selector/目标编码为零。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 仅对 `src`/`dst` 共同的逻辑 valid shape 定义结果；物理 padding 不是有效数据。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape；源与目标容量可独立。 |
| 输出 padding | 目标物理区域中有效区以外的填充遵循 `B.DATR` 的 pad-value 约定，不可依赖未声明状态。 |

## 异常和边界行为

非法索引类型、非法 element width、非法 layout、descriptor/schema 不匹配或逻辑 shape 不一致，
会在效果发生前被拒绝；目标容量不足走 `Fault_TileAllocation`。TLEA 不引入 overflow、
访存或 numeric-status fault。合法性/分配 preflight 和源快照先于目标的原子发布；直接
`S64`/`U64` 别名会读取旧的源 payload。

## 后端能力

- JCORE inline-asm wrapper：本仓库已实现（仅 `RowMajor`）。
- CPU simulator：未实现。
- AArch64/SME：未实现。
- LLVM MC / compiler lowering：分别判断，不因 wrapper 存在而推断端到端支持。

详见[统一支持状态表](../../support-status.md)。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.VEC TLEA, SrcDataType
B.DATR     PadValue, Layout (optional)
B.DIM      LB0=ValidCol
B.DIM      LB1=ValidRow (optional)
B.DIM      LB2=DstCol (optional)
B.IOT      IndexTile, mask=PE_MASK, last, ->ByteOffsetTile<TSize>
B.IOR      ElementBitsGPR, zero, zero, ->zero
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
// 源是 Local S32 元素索引；目标是对应的 Local S64 字节偏移（同一逻辑 valid shape）。
using IndexTile = Tile<Location::Vec, int32_t, 8, 32>;
using OffsetTile = Tile<Location::Vec, int64_t, 8, 32>;
IndexTile indices;
OffsetTile offsets;
// 每个张量元素 4 字节 (S32/FP32) 时，element_bits = 32。
TLEA(offsets, indices, 32u);
```

无符号索引改用 `uint32_t`/`uint64_t` 源并配 `uint64_t` 目标；`element_bits` 按实际元素位宽取 `8/16/32/64`。
