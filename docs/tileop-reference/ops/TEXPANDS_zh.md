# TEXPANDS

**Tile 标量与立即数** · **VEC** · 0.59 活动目录

## 简介

`TEXPANDS` 将低位元素宽度的原始标量编码复制到每个有效目标坐标。

## 数学语义

对每个有效坐标写入 `dst[r,c] = s`。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
void TEXPANDS(tile_shape &dst, typename tile_shape::DType s,
              unsigned UserValidCol = 0, unsigned UserValidRow = 0,
              unsigned UserPhysicalCol = 0);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `s` | 标量或标量 Tile 操作数；具体载体由重载决定。 |

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

### 数据规模与布局限制

- 使用 `CUBE_M16` 布局时，目标 Tile 的 `ValidRow` 不得超过 **16**。
- 使用 `CUBE_M32` 布局时，目标 Tile 的 `ValidRow` 不得超过 **32**。
- CUBE 布局下目标 Tile 的物理 shape 和容量必须能够表示声明的 valid shape；物理几何由所选布局和容量规则推导。

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
  using GM = global_tensor<float, RowMajor<32, 32>>;
  using TileT = Tile<Location::Vec, float, 32, 32>;
  float data[32 * 32] = {};
  GM dst_global(data);
  TileT dst;
  TEXPANDS(dst, 1.0f);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.VEC TEXPANDS, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       ScalarGPR, zero, zero, ->zero (optional)
BSTOP
```

活动目录编码为 `TEPL`，mode `1`，function `27`，selector `0x03B`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/tile-scalar-and-immediate/initialization/TEXPANDS.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/tile-scalar-and-immediate/initialization/TEXPANDS.asl)

[返回操作索引](../README.md)
