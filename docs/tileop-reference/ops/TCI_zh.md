# TCI

**初始化与不规则访问** · **SFU** · 0.59 活动目录

## 简介

`TCI` 从绑定的起始值构造一行指定类型的序列，并按逻辑列递增或递减。
`TCI_2D` 为 `CUBE_M16/CUBE_M32` 生成二维序列。

## 数学语义

以给定起始值生成索引 Tile。`TCI_2D` 提供受限二维步长。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape, typename T, int descending = 0>
void TCI(tile_shape &dst, T s);

template <is_tile_data_v tile_shape, typename T, int row_step, int col_step>
void TCI_2D(tile_shape &dst, T s);
```

### 支持的数据类型

支持S64、S32、S16、U64、U32、U16类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `s` | 标量或标量 Tile 操作数；具体载体由重载决定。 |

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

### 数据规模与形状限制

- 输出 Tile 必须为 RowMajor 布局，`ValidRow` 必须为 **1**。
- `ValidCol` 必须非零，物理列数 `Col` 必须至少为 `ValidCol`；输出容量必须足以容纳声明的形状。
- 若显式提供方向控制，方向值只能为 `0`（递增）或 `1`（递减）；起始值按所选元素宽度解释，高位按规范忽略。C++ 接口必须传入起始值；省略模板方向参数时递增。
- 该接口没有额外的固定字节容量上限；实际可用规模仍受 Tile 的物理容量和 shape 合法性约束。

`TCI_2D` 仅接受 Matrix-location 的 `CUBE_M16/CUBE_M32`，支持 S64、S32、S16、U64、U32、U16。
`row_step` 和 `col_step` 必须是编译期常量 `-1/0/1`；逻辑元素 `(r,c)` 的值为
`start + r * row_step + c * col_step`，并按 CUBE CELL 物理映射写入。M16 不支持 S64/U64；M16 的
`ValidRow` 不得超过 16，M32 的 `ValidRow` 必须为正数。

按行号递增时设置 `(row_step, col_step) = (1, 0)`，按列号递增时设置 `(0, 1)`。

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
  using GM = global_tensor<int32_t, RowMajor<1, 64>>;
  using TileT = Tile<Location::Vec, int32_t, 1, 64, BLayout::RowMajor, 1, 64>;
  int32_t data[64] = {};
  GM dst_global(data);
  TileT dst;
  TCI<TileT, int32_t, 0>(dst, 0);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.TEPL TCI, S64|S32|S16|U64|U32|U16
B.DATR      all-zero (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional, default 1; when present must equal 1)
B.DIM       rCol, 0, ->LB2  ; (optional, default ValidCol)
B.IOR       Start, Direction (optional; omission selects 0 and ascending)
B.IOT       mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

二维 CUBE bundle 使用：

```asm
BSTART.SFU TCI, S64|S32|S16|U64|U32|U16
B.DATR      CUBE_M16|CUBE_M32, DTYPE_NONE, Zero
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1
B.DIM       rPhysicalCol, 0, ->LB2
B.IOR       [StartGPR, Step2DGPR], []
B.IOT       mask=1111, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `3`，function `6`，selector `0x066`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/irregular-and-complex/initialization/TCI.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/irregular-and-complex/initialization/TCI.asl)

[返回操作索引](../README.md)
