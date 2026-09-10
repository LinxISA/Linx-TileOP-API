# TCVT

`TCVT(dst, src)` 将 `src` 的每个有效元素转换为由 `dst` 描述的目标元素类型。C++ Tile 类型决定源数据类型、目标数据类型、布局、有效 shape、物理存储 shape 以及目标 Tile 容量。

## C++ 接口

```cpp
template <is_tile_data_v DstTile, is_tile_data_v SrcTile>
void TCVT(DstTile &dst, SrcTile &src);
```

### Destination assembly：`TCVT_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v S>
void TCVT_ASS(D &assembled_dst, S &src);
```

destination 必须是 `range::assemble` carrier；与普通 `TCVT` 一样，source 和
destination 的物理 shape 必须匹配，目标 dtype 由 assembled destination 的 Tile
类型决定。参数顺序为 destination、source。

开发者需要提供具有明确类型的源 Tile 和目标 Tile。无需手动传入 ISA 编码值、Tile 寄存器编号或 TSize code。

## 普通布局

对于 `RowMajor`、`ColMajor` 等普通布局，源 Tile 和目标 Tile 必须满足以下条件：

- **容量推导的物理行相同**（PTO ISA `PTO-TILE-TCVT` legality："Source and
  destination have equal Row, Col, ValidRow, and ValidCol. Their capacities
  and packing independently match their own DataTypes."，其中 Row/Col 是
  `DerivedTileRows = capacity × 8 / (物理Col × 元素位宽)` 的容量推导值，不是
  C++ Tile 类型声明的逻辑 `Rows`/`Cols`）；
- `ValidRow` 和 `ValidCol` 相同；
- 有效区域均包含在对应的物理 Tile 中；
- 源/目标 dtype 组合以及 Tile location 合法；
- 目标容量足以容纳转换结果。除适用的 Tile 容量和 shape 规则外，普通布局没有额外的固定字节数限制。

**窄化 dtype（如 bf16/fp32 → e8m0）注意**：B.IOT 目的 TSize 最小 128B，1 字节
窄类型的 `[Rows,1]` 列向量会被 `round_capacity` 抬到 128B 档，容量推导行数随之
翻倍（如 `[64,1]` e8m0 推导为 128 行），与 2 字节源（64 行）不再相等——这是 ISA
层面的非法编码（issue #42）。合法写法是把**两侧**的物理列声明为更大的 2 的幂
（如 `[64,2]`，`ValidCol` 保持 1）：

```cpp
using SrcBf = Tile<Location::Vec, __bf16,    64, 2, BLayout::RowMajor, 64, 1>;
using DstE8 = Tile<Location::Vec, __fp8_e8m0, 64, 2, BLayout::RowMajor, 64, 1>;
// bf16 256B → DerivedTileRows = 256*8/(2*16) = 64
// e8m0 128B → DerivedTileRows = 128*8/(2*8)  = 64  ✓ 相等，ISA 合法
TCVT(dst_e8, src_bf);
```

示例：

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using Src = Tile<Location::Vec, float, 2, 1024, BLayout::RowMajor>;
using Dst = Tile<Location::Vec, __half, 2, 1024, BLayout::RowMajor>;

void convert(Dst &dst, Src &src) {
  TCVT(dst, src);
}
```

## CUBE_M16 和 CUBE_M32 转换

PTO-SPEC ADR-0110 明确了 `CUBE_M16` 和 `CUBE_M32` 的转换规则。这些布局与普通 TCVT 有一个重要区别：改变 dtype 可能改变每个 128-byte CELL 中存储的元素数量。因此，源和目标的物理列数及 TSize 分别根据各自的 dtype 独立推导。

开发者必须保持以下条件不变：

- 相同的 CUBE 布局（`CUBE_M16` 到 `CUBE_M16`，或 `CUBE_M32` 到 `CUBE_M32`）；
- 相同的逻辑有效行数和有效列数；
- Matrix Tile location；
- ISA 规定范围内的 Local Tile 容量，即 `128 B` 至 `64 KiB`。

源和目标的物理 `Rows`、物理 `Cols`、CELL 数量、所需字节数以及 `TilesizeCode` **不要求**相同。`CubeTileM16` 和 `CubeTileM32` 会根据元素类型和声明的 shape 分别计算这些属性。

示例：FP16 源 Tile 使用 `512 B`，而 FP32 目标 Tile 需要 `1 KiB`。二者表示相同的有效 `16 x 9` 矩阵。

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using Src = CubeTileM16<__half, 16, 12, 16, 9>;
using Dst = CubeTileM16<float, 16, 10, 16, 9>;

static_assert(Src::ValidRow == Dst::ValidRow);
static_assert(Src::ValidCol == Dst::ValidCol);
static_assert(Src::TilesizeCode == __tilesize_512B);
static_assert(Dst::TilesizeCode == __tilesize_1KB);

void convert_cube(Dst &dst, Src &src) {
  TCVT(dst, src);
}
```

`CUBE_N8` 转换不在 ADR-0110 的范围内，当前 API 会拒绝此类转换。

## ISA bundle 映射

对于普通 Tile，TileOP 会生成全部三个逻辑维度绑定：

```asm
BSTART.TEPL TCVT, SrcDataType
B.DATR DstDataType, RNONE
B.DIM rValidCol, 0, ->lb0
B.DIM rValidRow, 0, ->lb1
B.DIM zero, PhysicalCol, ->lb2
B.IOT SrcTile, mask=1111, last, ->DstTile<DstTSize>
```

对于 `CUBE_M16` 和 `CUBE_M32`，ADR-0110 定义如下：

- `LB0 = source ValidCol`；
- `LB1 = source ValidRow`；
- 省略 `LB2`；
- `B.IOT` 中目标 TSize 来自目标 Tile 类型。

```asm
BSTART.TEPL TCVT, SrcDataType
B.DATR DstDataType, RNONE
B.DIM rValidCol, 0, ->lb0
B.DIM rValidRow, 0, ->lb1
B.IOT SrcTile, mask=1111, last, ->DstTile<DstTSize>
```

当前实现使用扩展内联汇编。由于带类型的 C++ 操作数已经提供了该 bundle 所需的全部字段，因此不需要额外的 intrinsic：包括源和目标 Tile 寄存器、dtype 选择器、有效维度、布局检查以及目标 TSize。

## 编译期诊断

当 TileOP 能够证明以下条件成立时，会拒绝调用：

- CUBE 转换在 `CUBE_M16` 和 `CUBE_M32` 之间切换；
- 源和目标的有效 shape 不同；
- CUBE 操作数不在 Matrix Tile location；
- 源或目标 Local TSize 超出 `128 B..64 KiB` 范围；
- 目标使用 CUBE 布局，但源不是 `CUBE_M16` 或 `CUBE_M32`；
- 任一操作数使用不支持的 `CUBE_N8` 转换；
- 普通布局转换改变了物理 `Rows` 或 `Cols`。

其他 dtype 合法性、alias、数值行为、饱和、舍入、padding 以及异常行为遵循 PTO-SPEC 的 TCVT 合约。

## 规范

详见 PTO-SPEC `TCVT` 操作和 ADR-0110；该规则由 PTO-SPEC issue #167 / PR #176 引入。
