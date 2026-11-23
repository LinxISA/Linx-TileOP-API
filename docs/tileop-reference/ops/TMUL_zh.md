# TMUL

**逐元素 Tile–Tile** · **VEC** · 0.59 活动目录

## 简介

`TMUL` 将两个 Local Tile 的对应元素相乘。

## 数学语义

对每个有效坐标，令 `x = src0[r,c]`、`y = src1[r,c]`，则 `dst[r,c] = x * y`。

## C++ 接口

```cpp
template <is_tile_data_v Dst, is_tile_data_v Src0, is_tile_data_v Src1>
void TMUL(Dst &dst, Src0 &src0, Src1 &src1,
          unsigned UserValidCol = 0, unsigned UserValidRow = 0,
          unsigned UserPhysicalCol = 0);
```

固定有效维度优先在 Tile 类型中声明。普通重载可用 `UserValidCol`、`UserValidRow`、`UserPhysicalCol` 覆盖有效列、有效行和物理列。`0` 表示使用原有描述。显式物理列须覆盖有效列，且不得超过 `Tile::Cols`。

### Associated destination：`TMUL_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TMUL_ASS(D &assembled_dst, A &src0, B &src1);
```

`assembled_dst` 必须是 `range::assemble` carrier；三个 Tile 的 dtype 必须相同，参数顺序为 destination、两个 source。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |

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
  float left_data[64 * 32] = {}, right_data[64 * 32] = {}, result_data[64 * 32] = {};
  GM left_global(left_data), right_global(right_data), result_global(result_data);
  TileT left, right, result;
  TLOAD(left, left_global);
  TLOAD(right, right_global);
  TMUL(result, left, right);
  TSTORE(result_global, result);
}
```

## Bundle 对照

```asm
BSTART.VEC TMUL, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcLeft, SrcRight, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `0`，function `2`，selector `0x002`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/arithmetic/TMUL.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/arithmetic/TMUL.asl)

[返回操作索引](../README.md)
