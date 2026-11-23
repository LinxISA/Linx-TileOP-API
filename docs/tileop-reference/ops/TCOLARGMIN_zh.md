# TCOLARGMIN

**归约与广播** · **SFU** · 0.59 活动目录

## 简介

`TCOLARGMIN` 按行号递增顺序扫描每列，返回该列最小值对应的最小行索引。

## 数学语义

沿有效行按索引递增顺序比较 `src[k,c]`，将最小元素所在的 `k` 写入 `dst[0,c]`。结果类型为 `U32`，相等时保留较小的索引。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLARGMIN(tile_shape_out &dst, tile_shape_in &src);
```

### 支持的数据类型

输出索引使用 `U32`。

支持FP32、FP16、BF16、S32、S16、S8、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |

## 约束与默认值

该操作沿每列的行轴归约。输入有效 shape 为 `R x C`，输出逻辑 valid
shape 为 `1 x C`，输出元素是 U32 语义的行索引；输出 Tile dtype 必须为
`uint32_t`。

源 Tile 的 allocated capacity 仅需满足通用 Tile allocation 规则；归约操作本身不额外施加 2048-byte 上限。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 每列输出一个行索引，输出 valid shape 为 `1 x C`。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- 省略 `B.DATR` 时，padding 值使用 `Null`。
- `LB0`/`LB1`/`LB2` 描述**源 Tile** 几何（源的 `ValidCol`/`ValidRow`/物理列数），且必须与源 descriptor 完全一致；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 列归约的 `LB1` 必须等于**源**的有效行数（即被归约的全部行数），不是目的的单行；目的是规则推导的 `1 x N`，不参与 B.DIM 编码。
- 显式 padding 编码 `00`、`01`、`10`、`11` 分别选择 `Zero`、`Max`、`Min`、`Null`。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void column_indices(uint32_t *output, const float *input) {
  using InputGM = pto::global_tensor<float, pto::RowMajor<32, 32>>;
  using OutputGM = pto::global_tensor<uint32_t, pto::RowMajor<1, 32>>;
  using Input = pto::Tile<pto::Location::Vec, float, 32, 32>;
  using Output = pto::Tile<pto::Location::Vec, uint32_t, 32, 32,
                           pto::BLayout::RowMajor, 1, 32>;
  InputGM source(input);
  OutputGM destination(output);
  Input values;
  Output indices;
  TLOAD(values, source);
  TCOLARGMIN(indices, values);
  TSTORE(destination, indices);
}
```

## Bundle 对照

```asm
BSTART.SFU TCOLARGMIN, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `2`，function `29`，selector `0x05D`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/reduce-and-expand/column-reduction/TCOLARGMIN.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/reduce-and-expand/column-reduction/TCOLARGMIN.asl)

[返回操作索引](../README.md)
