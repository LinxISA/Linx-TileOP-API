# TPREFETCH

**GM 搬运与索引访存** · **TLSU** · 0.59 活动目录

## 简介

`TPREFETCH` 为全部四个 PE 预取一个带类型且带步长的 GM 矩形，不产生 Tile 目标。

## 数学语义

按受支持的区域和参数请求 GM 预取。

## C++ 接口

```cpp
template <is_global_data_v gm_shape>
void TPREFETCH(const gm_shape &src, uint32_t valid_col, uint32_t valid_row);
```

### 支持的数据类型

支持的 Tile 数据类型由该操作的 C++ 模板约束和本页约束共同限定。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `src` | 输入 Tile 或源数据。 |
| `valid_col` | 运行时有效列数；不得超过 Tile 的物理列数。 |
| `valid_row` | 运行时有效行数；不得超过 Tile 的物理行数。 |

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

- 省略 `B.DATR` 时使用 `NORM` 布局。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using GM = global_tensor<float, RowMajor<8, 512>>;

void prefetch_rows(const float *input) {
  GM input_gm(input);
  // 请求 4 行、每行 256 个有效元素对应的 GM cache lines。
  TPREFETCH(input_gm, 256, 4);
}
```

## Bundle 对照

```asm
BSTART.TPREFETCH DataType
B.DATR      Layout (optional)
B.DIM       LB0/ValidCol, LB1/ValidRow, LB2/Col (optional)
B.IOR       base,row_stride (optional)
BSTOP
```

活动目录编码为 `TLSU`，function `3`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/memory-and-data-movement/regular/TPREFETCH.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/memory-and-data-movement/regular/TPREFETCH.asl)

[返回操作索引](../README.md)
