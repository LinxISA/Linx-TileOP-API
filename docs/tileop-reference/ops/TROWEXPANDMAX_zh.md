# TROWEXPANDMAX

**归约与广播** · **SFU** · 0.59 活动目录

## 简介

`TROWEXPANDMAX` 将每个有效行的元素与该行的行广播值取最大值。

## 数学语义

对输出有效坐标 `(r,c)`，令 `x = src0[r,c]`、`y = v[r,0]`，则 `dst[r,c] = max(x, y)`。`v` 是广播操作数 `src1`。

## C++ 接口

```cpp
template <
    is_tile_data_v tile_shape_out,
    is_tile_data_v tile_shape_in0,
    is_tile_data_v tile_shape_in1>
void TROWEXPANDMAX(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1);
```

### Associated destination：`TROWEXPANDMAX_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TROWEXPANDMAX_ASS(D &assembled_dst, A &src0, B &src1);
```

destination 必须是 `range::assemble` carrier；参数顺序为 destination、两个 source，dtype 和 expand shape 规则与普通接口相同。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |

## 约束与默认值

`src0` 和 `dst` 的有效 shape 为 `R x C`，`src1` 为每行一个值的 `R x 1`；
操作将 `src1` 的值按行广播到 `src0`。三个 Tile 的 dtype 必须相同。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | `src0`/`dst` 为 `R x C`，广播源 `src1` 为 `R x 1`；输出 valid shape 为 `R x C`。 |
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
  using MatrixGM = global_tensor<float, RowMajor<32, 32>>;
  using VectorGM = global_tensor<float, RowMajor<32, 1>>;
  using MatrixTile = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 32>;
  using RowTile = Tile<Location::Vec, float, 32, 1, BLayout::RowMajor, 32, 1>;
  float matrix_data[32 * 32] = {}, row_data[32] = {}, dst_data[32 * 32] = {};
  MatrixGM matrix_global(matrix_data), dst_global(dst_data); VectorGM row_global(row_data);
  MatrixTile src, dst; RowTile row;
  TLOAD(src, matrix_global); TLOAD(row, row_global);
  TROWEXPANDMAX(dst, src, row);
  TSTORE(dst_global, dst);
}
```

`BroadcastByteOffset` 选出的 operation-typed CELL slot 必须小于源的有效列数；
静态有效区在编译期检查，动态有效区由目标端合法性检查。

## Bundle 对照

```asm
BSTART.SFU TROWEXPANDMAX, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcTile, BroadcastTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `2`，function `9`，selector `0x049`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/reduce-and-expand/row-expansion/TROWEXPANDMAX.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/reduce-and-expand/row-expansion/TROWEXPANDMAX.asl)

[返回操作索引](../README.md)
