# TNOT

**逐元素 Tile–Tile** · **VEC** · 0.59 活动目录

## 简介

`TNOT` 对 Local 整数 Tile 执行按元素宽度的按位取反。

## 数学语义

对每个有效坐标，令 `x = src[r,c]`，则 `dst[r,c] = ~x`。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
void TNOT(tile_shape &dst, tile_shape &src);
```

### Associated destination：`TNOT_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v S>
void TNOT_ASS(D &assembled_dst, S &src);
```

`assembled_dst` 必须是 `range::assemble` carrier；source 与 destination dtype 必须相同，参数顺序为 destination、source。

### 支持的数据类型

操作类型为 `S64`、`S32`、`S16`、`S8`、`U64`、`U32`、`U16` 或 `U8`。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- 省略 `B.DATR` 时，padding 值使用 `Null`。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 显式 padding 编码 `00`、`01`、`10`、`11` 分别选择 `Zero`、`Max`、`Min`、`Null`。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using GM = global_tensor<uint32_t, RowMajor<64, 32>>;
  using TileT = Tile<Location::Vec, uint32_t, 64, 32>;
  uint32_t src_data[64 * 32] = {}, dst_data[64 * 32] = {};
  GM src_global(src_data), dst_global(dst_data);
  TileT src, dst;
  TLOAD(src, src_global);
  TNOT(dst, src);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.VEC TNOT, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       Src, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `0`，function `16`，selector `0x010`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/logical/TNOT.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/logical/TNOT.asl)

[返回操作索引](../README.md)
