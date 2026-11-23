# TSELS

**Tile 标量与立即数** · **VEC** · 0.59 活动目录

## 简介

`TSELS` 按照 packed predicate Tile，从 Tile true source 和每 PE
false-scalar 中选择结果。其参数顺序是
`(dst, predicate, false_scalar, src_true)`。

## 数学语义

逐坐标在有效 Tile 元素与同一个标量之间进行谓词选择。标量和 Tile 的角色按所选重载绑定。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
void TSELS(tile_shape &dst, tile_shape &predicate,
           typename tile_shape::DType false_scalar,
           tile_shape &src_true);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `predicate` | `TCMP`/`TCMPS` 产生的 packed predicate。 |
| `false_scalar` | predicate 为假时使用的每 PE scalar。 |
| `src_true` | predicate 为真时使用的 Tile。 |

对每个有效元素，predicate 为真时执行 `dst = src_true`，否则使用
`false_scalar`。

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

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using GM = global_tensor<float, RowMajor<64, 32>>;
  using TileT = Tile<Location::Vec, float, 64, 32>;
  float left_data[64 * 32] = {}, right_data[64 * 32] = {}, dst_data[64 * 32] = {};
  GM left_global(left_data), right_global(right_data), dst_global(dst_data);
  TileT left, right, dst, predicate;
  TileT::DType scalar = 1.0f;
  TLOAD(left, left_global);
  TLOAD(right, right_global);
  TCMP<CmpMode::LT>(predicate, left, right);
  TSELS(dst, predicate, scalar, left);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.VEC TSELS, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       Predicate, SrcTrue, mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       ScalarFalseGPR, zero, zero, ->zero (optional)
BSTOP
```

活动目录编码为 `TEPL`，mode `1`，function `26`，selector `0x03A`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/tile-scalar-and-immediate/logical/TSELS.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/tile-scalar-and-immediate/logical/TSELS.asl)

[返回操作索引](../README.md)
