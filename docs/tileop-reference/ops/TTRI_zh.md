# TTRI

**初始化与不规则访问** · **SFU** · 0.59 活动目录

## 简介

`TTRI` generates an exact typed lower or upper triangular matrix in a new Local Tile。

## 数学语义

按操作参数生成三角形 Tile。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
void TTRI(tile_shape &dst);
```

### 支持的数据类型

支持FP32、FP16、S32、S16、U32、U16类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

### 数据规模与形状限制

- 输出 Tile 必须为 RowMajor 布局，`ValidRow` 和 `ValidCol` 都必须非零。
- 物理列数 `Col` 必须至少为 `ValidCol`；输出容量必须足以容纳声明的形状。
- 显式对角线位移必须在 **-65535..65535**（含端点）范围内；方向控制只能表示 lower 或 upper。未提供时默认对角线为 0 且使用 lower 方向。
- 该接口没有额外的固定字节容量上限；实际可用规模仍受 Tile 的物理容量和 shape 合法性约束。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 物理 padding 始终使用 `Null`。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using GM = global_tensor<int32_t, RowMajor<32, 32>>;
  using TileT = Tile<Location::Vec, int32_t, 32, 32, BLayout::RowMajor>;
  int32_t data[32 * 32] = {};
  GM dst_global(data);
  TileT dst;
  TTRI(dst);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.SFU TTRI, FP32|FP16|S32|S16|U32|U16
B.DATR      all-zero (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional, default 1)
B.DIM       rCol, 0, ->LB2  ; (optional, default ValidCol)
B.IOR       Diagonal, Orientation (optional; omission selects 0 and lower)
B.IOT       mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `3`，function `7`，selector `0x067`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/irregular-and-complex/initialization/TTRI.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/irregular-and-complex/initialization/TTRI.asl)

[返回操作索引](../README.md)
