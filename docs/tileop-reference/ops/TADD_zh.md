# TADD

**逐元素 Tile–Tile** · **VEC** · 0.59 活动目录

## 简介

`TADD` 是由 VEC 执行的选择器编码 Tile 操作：它将有序的左、右 Local 源中对应的有效元素相加；其当前指令 contract 规定了确切的 bundle 形式和发布边界。

## 数学语义

对每个有效坐标，令 `x = src0[r,c]`、`y = src1[r,c]`，则 `dst[r,c] = x + y`。

## C++ 接口

```cpp
template <is_tile_data_v tile_shape>
  requires(!is_assemble_v<tile_shape>)
void TADD(tile_shape &dst, tile_shape &src0, tile_shape &src1,
          unsigned UserValidCol = 0, unsigned UserValidRow = 0,
          unsigned UserPhysicalCol = 0);

template <typename Parent, unsigned ParentSizeCode, bool INIT, bool LAST,
          unsigned OffsetUnits, unsigned RegSrc, unsigned WriterSizeCode,
          is_tile_data_v A, is_tile_data_v B>
  requires(is_local_tile_v<Parent> && !is_assemble_v<A> &&
           !is_assemble_v<B>)
void TADD(range::Assemble<Parent, ParentSizeCode, INIT, LAST, OffsetUnits,
                          RegSrc, WriterSizeCode> &dst,
          A &src0, B &src1, unsigned UserValidCol = 0,
          unsigned UserValidRow = 0);
```

固定有效维度优先在 Tile 类型中声明。普通重载可用 `UserValidCol`、`UserValidRow`、`UserPhysicalCol` 覆盖有效列、有效行和物理列。`0` 表示使用原有描述。显式物理列须覆盖有效列，且不得超过 `Tile::Cols`。

### Associated destination：`TADD_ASS`

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TADD_ASS(D &assembled_dst, A &src0, B &src1);
```

`assembled_dst` 必须是 `range::assemble_middle` 或 `range::assemble_last` carrier；三个 Tile 的 dtype 必须相同，参数顺序为 destination、两个 source。INIT slot 使用下面的 plain `TADD` producer。INIT 重载的两个可选维度实参保留默认 `0`。

两个 binary source 也可以是合法的 `range::subview` carrier。此时 source
`B.IOT` 后分别发射对应的 `B.SUBVIEW`，assembled destination 仍按原有规则
发射 `B.ASSEMBLE`：

```cpp
auto lhs = range::subview<4, 4>(lhs_parent);
auto rhs = range::subview<4, 8>(rhs_parent);
auto last = range::assemble_last(parent);
TADD_ASS(last, lhs, rhs);
```

### 支持的数据类型

常用浮点路径为 `FP64`、`FP32`、`FP16`、`BF16`。整数操作类型为 `S64/S32/S16/S8`、`U64/U32/U16/U8`。ASL 的类型检查还接纳 `TF32`、`HF32`、`E4M3`、`E5M2` 编码，但其浮点元素语义没有为这些类型定义结果。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |

## 约束与默认值

### Assembly destination

plain `TADD` 可以把 `range::assemble(parent)` 作为 INIT destination，并发射分配型 `B.IOT` 后的 `B.ASSEMBLE INIT`。两个 source 必须是普通 Local Tile，并且彼此及 destination parent 的 dtype、location、layout、物理 shape、valid shape、容量和存储描述必须完全一致。MIDDLE/LAST slot 必须使用 `TADD_ASS`；`TADD_ASS(range::assemble(parent), ...)` 会在编译期拒绝。

```cpp
auto init = range::assemble(parent);
TADD(init, a, b);
auto middle = range::assemble_middle(parent);
TADD_ASS(middle, a, b);
auto last = range::assemble_last(parent);
TADD_ASS(last, a, b);
```

- 普通重载的三个参数使用同一个 Local Tile 类型。源必须已定义，物理形状、有效形状和布局须相容。
- 支持 RowMajor、CUBE_M16 和 CUBE_M32；CUBE_M16 不接受 64-bit 元素。Shared、CUBE_N8 和混合布局不适用。
- 普通重载以 `mask=1111` 发射，不暴露动态参与掩码；结果占用新分配的 Local Tile 代际。
- 结果定义在有效区域内。未显式选择定义性 padding 时，不读取 padding。

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
- 显式 padding 编码 `00`、`01`、`10`、`11` 分别选择 `Zero`、`Max`、`Min`、`Null`。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using GM = global_tensor<float, RowMajor<64, 32>>;
  using TileT = Tile<Location::Vec, float, 64, 32>;

  float left_data[64 * 32] = {};
  float right_data[64 * 32] = {};
  float result_data[64 * 32] = {};
  GM left_global(left_data);
  GM right_global(right_data);
  GM result_global(result_data);
  TileT left, right, result;

  TLOAD(left, left_global);
  TLOAD(right, right_global);
  TADD(result, left, right);
  TSTORE(result_global, result);
}
```

## Bundle 对照

```asm
BSTART.VEC TADD, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcLeft, SrcRight, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `0`，function `0`，selector `0x000`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/arithmetic/TADD.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/arithmetic/TADD.asl)

[返回操作索引](../README.md)
