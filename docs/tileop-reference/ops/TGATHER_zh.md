# TGATHER

**初始化与不规则访问** · **SFU** · 0.59 活动目录

## 简介

`TGATHER` 从各自独立选择的源行中收集值。

## 数学语义

在 Tile 内按索引采集元素。

## C++ 接口

```cpp
template <
    is_tile_data_v tile_shape_out,
    is_tile_data_v tile_shape_in,
    is_tile_data_v tile_shape_off>
void TGATHER(tile_shape_out &dst, tile_shape_in &src, tile_shape_off &off);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、S64、S32、S16、S8、U64、U32、U16、U8类型。

| 操作数角色 | 类型要求 |
| --- | --- |
| 数据 Tile | 支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、S64、S32、S16、S8、U64、U32、U16、U8类型。 |
| 索引 / 地址位移 Tile | 必须使用该操作 contract 允许的整数 dtype 与单位。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `off` | 编译期或运行期偏移参数；单位和取值范围由该重载定义。 |

## 约束与默认值

索引/地址 Tile 的 dtype、单位和取值范围必须有效；重复目标、越界、alias 与写入顺序的语义 由该操作的约束和边界行为定义，不能由普通逐元素操作的直觉推断。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Source = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Indices = Tile<Location::Vec, uint16_t, 16, 32, BLayout::RowMajor>;
using Result = Tile<Location::Vec, float, 16, 32, BLayout::RowMajor>;
using GMSource = global_tensor<float, RowMajor<32, 64>>;
using GMIndices = global_tensor<uint16_t, RowMajor<16, 32>>;
using GMResult = global_tensor<float, RowMajor<16, 32>>;

void tile_gather(float *out, const float *input, const uint16_t *indices) {
  GMSource source_gm(input); GMIndices indices_gm(indices); GMResult out_gm(out);
  Source source; Indices index; Result result;
  TLOAD(source, source_gm);
  TLOAD(index, indices_gm);
  TGATHER(result, source, index);
  TSTORE(out_gm, result);
}
```

## Bundle 对照

```asm
BSTART.SFU TGATHER, ValueDataType
B.DATR      Layout (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       ValueSrc, IndexSrc, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `3`，function `15`，selector `0x06F`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/irregular-and-complex/layout/TGATHER.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/irregular-and-complex/layout/TGATHER.asl)

[返回操作索引](../README.md)
