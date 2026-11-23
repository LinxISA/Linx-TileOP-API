# TCOLEXPAND

**归约与广播** · **SFU** · 0.59 活动目录

## 简介

`TCOLEXPAND` 将列广播源逐位复制到每个有效目标行。

## 数学语义

对输出有效坐标 `(r,c)`，将广播源 `v` 的 `v[0,c]` 写入 `dst[r,c]`。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLEXPAND(tile_shape_out &dst, tile_shape_in &src);
```

### Destination assembly：`TCOLEXPAND_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v S>
void TCOLEXPAND_ASS(D &assembled_dst, S &src);
```

输入为 `1 x C`，destination 为 `R x C`；destination 必须使用
`range::assemble` 系列 carrier。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |

## 约束与默认值

输入是每列一个元素的 `1 x C` Tile，操作把该行值广播到输出的每一列，
因此输出逻辑 valid shape 必须为 `R x C`。输入和输出 dtype 必须相同。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 输入 valid shape 为 `1 x C`，输出 valid shape 为 `R x C`。 |
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
  using InputGM = global_tensor<float, RowMajor<1, 32>>;
  using OutputGM = global_tensor<float, RowMajor<32, 32>>;
  using InputTile = Tile<Location::Vec, float, 1, 32, BLayout::RowMajor>;
  using OutputTile = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
  float src_data[32] = {}, dst_data[32 * 32] = {};
  InputGM src_global(src_data);
  OutputGM dst_global(dst_data);
  InputTile src;
  OutputTile dst;
  TLOAD(src, src_global);
  TCOLEXPAND(dst, src);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.SFU TCOLEXPAND, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       BroadcastTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `2`，function `20`，selector `0x054`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/reduce-and-expand/column-expansion/TCOLEXPAND.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/reduce-and-expand/column-expansion/TCOLEXPAND.asl)

[返回操作索引](../README.md)
