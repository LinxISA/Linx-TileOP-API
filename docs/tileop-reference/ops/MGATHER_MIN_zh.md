# MGATHER_MIN

**GM 搬运与索引访存** · **TLSU** · 0.59 活动目录

## 简介

`MGATHER_MIN` 使用索引 Tile 中的 S32/U32/S64/U64 字节位移访问 GM：地址为 `Base + displacement`，不按元素宽度缩放，不使用 GM 行 stride。`B.IOR` 只绑定 GM base；普通重载的 RegSrc1、RegSrc2、RegDst 必须为零。可选 `IndexedByteRange` 重载遵循尚未合入 main 的 [#424](https://github.com/PTO-ISA/pto-spec/pull/424)，单独固定在 `1981bceffa82`；该重载使用 `[Base, Min, Max]`，参见[谓词与执行范围](../helpers_zh.md#谓词与执行范围)。

## 数学语义

每个参与 lane 使用 `address = Base + byte_offset`。索引按自身类型解释为有符号或无符号字节偏移。在 indexed GM 地址执行原子取较小值更新并取得旧值。

## C++ 接口

```cpp
template <is_tile_data_v DstTile, is_tile_data_v IndexTile, is_tile_data_v ValueTile>
void MGATHER_MIN(
    DstTile &observedOld,
    uint64_t base,
    IndexTile &elementIndices,
    ValueTile &value,
    uint32_t validCol,
    uint32_t validRow = 1);
template <is_tile_data_v DstTile, is_tile_data_v IndexTile, is_tile_data_v ValueTile>
void MGATHER_MIN(
    DstTile &observedOld,
    uint64_t base,
    IndexTile &elementIndices,
    ValueTile &value,
    uint32_t validCol,
    IndexedByteRange range,
    uint32_t validRow = 1);
```

### 支持的数据类型

支持S64、S32、U64、U32类型。

| 操作数角色 | 类型要求 |
| --- | --- |
| 数据 Tile | 支持S64、S32、U64、U32类型。 |
| 索引 / 地址位移 Tile | S32/U32/S64/U64；相对 GM Base 的字节位移。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `observedOld` | 保存每个位置观察到的旧值的输出 Tile。 |
| `base` | GM 基地址。 |
| `elementIndices` | 相对 GM Base 的字节位移 Tile；不按元素宽度缩放。 |
| `value` | 接口声明的输入参数；其类型、取值范围和作用由该重载及本页约束定义。 |
| `validCol` | 有效区域的列数。 |
| `validRow` | 有效区域的行数，省略时使用接口/规范默认值。 |
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

以下是 C++ 声明中可直接省略的默认实参：

| 参数 | 默认值 |
| --- | --- |
| `validRow` | `1` |

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Transfer = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using ElementIndices = Tile<Location::Vec, int32_t, 8, 256, BLayout::RowMajor>;

void atomic_rmw(Transfer &observed_old, ElementIndices &element_indices,
                Transfer &value) {
  // 每个 index 是相对于 base 的相对 GM Base 的字节位移；返回值是运算前读到的值。
  MGATHER_MIN(observed_old, 0x1000ull, element_indices, value, 256, 2);
}
```

## Bundle 对照

```asm
BSTART.MGATHER.MIN DataType
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional, default 1)
B.DIM       rCol, 0, ->LB2
B.IOT       IndexTile, ValueTile, mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       BaseGPR, zero, zero, ->zero
BSTOP
```

活动目录编码为 `TLSU`，function `11`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/memory-and-data-movement/irregular/MGATHER_MIN.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/memory-and-data-movement/irregular/MGATHER_MIN.asl)

[返回操作索引](../README.md)
