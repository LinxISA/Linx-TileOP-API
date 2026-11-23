# MGATHER_MASK

**GM 搬运与索引访存** · **TLSU** · 0.59 活动目录

## 简介

`MGATHER_MASK` 使用索引 Tile 中的 S32/U32/S64/U64 字节位移访问 GM：地址为 `Base + displacement`，不按元素宽度缩放，不使用 GM 行 stride。`B.IOR` 只绑定 GM base；普通重载的 RegSrc1、RegSrc2、RegDst 必须为零。可选 `IndexedByteRange` 重载遵循尚未合入 main 的 [#424](https://github.com/PTO-ISA/pto-spec/pull/424)，单独固定在 `1981bceffa82`；该重载使用 `[Base, Min, Max]`，参见[谓词与执行范围](../helpers_zh.md#谓词与执行范围)。

## 数学语义

每个参与 lane 使用 `address = Base + byte_offset`。索引按自身类型解释为有符号或无符号字节偏移。以掩码控制参与读取的索引 lane。

## C++ 接口

```cpp
template <
    typename tile_shape_out,
    typename tile_shape_offset,
    typename tile_shape_mask,
    typename gm_shape,
    TmaPadValue Pad = TmaPadValue::Null>
inline void MGATHER_MASK(
    tile_shape_out &dst,
    const gm_shape &src,
    const tile_shape_offset &offset,
    const tile_shape_mask &mask);
template <
    typename tile_shape_out,
    typename tile_shape_offset,
    typename tile_shape_mask,
    typename gm_shape,
    TmaPadValue Pad = TmaPadValue::Null>
inline void MGATHER_MASK(
    tile_shape_out &dst,
    const gm_shape &src,
    const tile_shape_offset &offset,
    const tile_shape_mask &mask,
    IndexedByteRange range);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、S64、S32、S16、S8、U64、U32、U16、U8类型。

| 操作数角色 | 类型要求 |
| --- | --- |
| 数据 Tile | 支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、S64、S32、S16、S8、U64、U32、U16、U8类型。 |
| 索引 / 地址位移 Tile | S32/U32/S64/U64；相对 GM Base 的字节位移。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `offset` | 相对 GM Base 的字节位移 Tile；不按元素宽度缩放。 |
| `mask` | 逐元素掩码 Tile；仅掩码允许的位置参与该重载定义的读写。 |
| `range` | 接口声明的输入参数；其类型、取值范围和作用由该重载及本页约束定义。 |

### 重载选择

这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。

范围重载传递有符号 64 位字节边界：普通 gather/scatter 在参数末尾追加 `IndexedByteRange range`，atomic/reduction 在 `validCol` 后、可选 `validRow` 前传入。

## 约束与默认值

内存地址、byte displacement、mask 和 PE 参与集合必须符合 TLSU contract；地址单位和 fault 行为见本页的异常和边界行为说明。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Values = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;
using ElementIndices = Tile<Location::Vec, uint32_t, 8, 32, BLayout::RowMajor>;
using Mask = Tile<Location::Vec, uint8_t, 8, 32, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<8, 1024>>;
using GMOut = global_tensor<float, RowMajor<8, 32>>;

void gather_masked(float *out, const float *base, const uint32_t *offsets,
                   const uint8_t *enabled) {
  GM base_gm(base);
  GMOut out_gm(out);
  Values dst;
  ElementIndices offset;
  Mask mask;
  global_tensor<uint32_t, RowMajor<8, 32>> offsets_gm(offsets);
  global_tensor<uint8_t, RowMajor<8, 32>> enabled_gm(enabled);
  TLOAD(offset, offsets_gm);
  TLOAD(mask, enabled_gm);
  MGATHER_MASK<Values, ElementIndices, Mask, GM, TmaPadValue::Zero>(
      dst, base_gm, offset, mask);
  TSTORE(out_gm, dst);
}
```

## Bundle 对照

```asm
BSTART.MGATHER.MASK DataType
B.DATR      PadValue, Layout (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       IndexTile, MaskTile, mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       BaseGPR, zero, zero, ->zero
BSTOP
```

活动目录编码为 `TLSU`，function `6`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/memory-and-data-movement/irregular/MGATHER_MASK.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/memory-and-data-movement/irregular/MGATHER_MASK.asl)

[返回操作索引](../README.md)
