# TFMA

**逐元素 Tile–Tile** · **VEC** · 0.59 活动目录

## 简介

`TFMA` 对三个 Local Tile 源执行对应类型的逐元素融合乘加。

## 数学语义

对每个有效坐标执行 `dst[r,c] = fma(src0[r,c], src1[r,c], src2[r,c])`。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
void TFMA(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2);
```

### Destination assembly：`TFMA_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TFMA_ASS(D &assembled_dst, A &src0, B &src1, A &src2);
```

`assembled_dst` 必须是 `range::assemble(dst)` 或同一 session 的后续 assembly
carrier；三个 source 的 dtype 必须满足普通 `TFMA` 的约束，且第三个 source 与
第一个 source 使用相同的 Tile 类型。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |
| `src2` | 第三个输入 Tile。 |

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
  float a_data[64 * 32] = {}, b_data[64 * 32] = {}, c_data[64 * 32] = {}, result_data[64 * 32] = {};
  GM a_global(a_data), b_global(b_data), c_global(c_data), result_global(result_data);
  TileT a, b, c, result;
  TLOAD(a, a_global);
  TLOAD(b, b_global);
  TLOAD(c, c_global);
  TFMA(result, a, b, c);
  TSTORE(result_global, result);
}
```

## Bundle 对照

```asm
BSTART.VEC TFMA, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcLeft, SrcRight, mask=PE_MASK
B.IOT       SrcAddend, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `0`，function `28`，selector `0x01C`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/arithmetic/TFMA.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/arithmetic/TFMA.asl)

[返回操作索引](../README.md)
