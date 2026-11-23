# TMULS

**Tile 标量与立即数** · **VEC** · 0.59 活动目录

## 简介

`TMULS` 将每个有效 Local Tile 元素乘以 private-GPR 标量。

## 数学语义

对每个有效坐标，令 `x = src[r,c]`、`y = s`，则 `dst[r,c] = x * y`。标量在当前 PE 的一次 Tile 运算中复用。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
void TMULS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s);
```

### Associated destination：`TMULS_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v S>
void TMULS_ASS(D &assembled_dst, S &src, typename S::DType scalar);
```

destination 必须是 `range::assemble` carrier；scalar、source 和 destination dtype 必须相同，参数顺序为 destination、source、scalar。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `s` | 标量或标量 Tile 操作数；具体载体由重载决定。 |

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
  using GM = global_tensor<float, RowMajor<64, 32>>;
  using TileT = Tile<Location::Vec, float, 64, 32>;
  float src_data[64 * 32] = {}, dst_data[64 * 32] = {};
  GM src_global(src_data), dst_global(dst_data);
  TileT src, dst;
  TileT::DType scalar = 2.0f;
  TLOAD(src, src_global);
  TMULS(dst, src, scalar);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.VEC TMULS, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcTile, mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       ScalarGPR, zero, zero, ->zero (optional)
BSTOP
```

活动目录编码为 `TEPL`，mode `1`，function `2`，selector `0x022`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/tile-scalar-and-immediate/arithmetic/TMULS.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/tile-scalar-and-immediate/arithmetic/TMULS.asl)

[返回操作索引](../README.md)
