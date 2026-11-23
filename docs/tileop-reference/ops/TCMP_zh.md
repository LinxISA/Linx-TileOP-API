# TCMP

**逐元素 Tile–Tile** · **VEC** · 0.59 活动目录

## 简介

`TCMP` 是由 VEC 执行的比较操作。NORM 布局产生 packed predicate；
CUBE_M16/M32 布局产生以源操作类型为 basis 的 PredicateCell。接口显式保留
源 CUBE 布局，不会回落为 NORM。PredicateCell 不是普通 Numeric U8；后续
`TSEL` 按逻辑坐标消费谓词，只要求布局和有效形状匹配；比较 basis
不限制所选数值的类型或位宽。

## 数学语义

逐坐标计算 `CmpMode(src0[r,c], src1[r,c])`，按目标布局编码谓词。

## C++ 接口

```cpp
template <CmpMode Mode, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1 = tile_shape_in0>
void TCMP(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1);
te
### 跨类型载体（PTO-ISA 0.58.6，pto-spec #260）

两个源可使用**等元素位宽、非打包 4-bit** 的异类型载体（如 FP32 操作数用 U32 tile 承载）：源 backing 类型与操作类型分离，各自独立校验；精确同类型保持合法。打包容器（FP4X2/S4X2/U4X2）不参与重解释。`tile_shape_in1` 缺省为 `tile_shape_in0`（同类型旧行为不变）。
mplate <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCMP(tile_shape_out &dst, tile_shape_in &src0, tile_shape_in &src1);
```

### Destination assembly：`TCMP_ASS`

```cpp
template <CmpMode Mode, is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TCMP_ASS(D &assembled_dst, A &src0, B &src1);
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TCMP_ASS(D &assembled_dst, A &src0, B &src1); // Mode = EQ
```

`assembled_dst` 必须是 `range::assemble(dst)`（或同一 assembly session 的
`range::assemble_middle`/`range::assemble_last`）返回值；参数顺序为 destination
在前、输入在后。其余比较约束与 `TCMP` 相同。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |

### 重载选择

这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

`src0`、`src1` 和 predicate `dst` 必须具有相同的物理 `Rows x Cols` 与
`BLayout`，并且 valid shape 也必须一致。`validCol < Cols` 只表示尾部物理列
不参与有效计算，不允许用不同物理列数的 Tile 作为 compare 链的另一输入。

窄列 `[32,1]` / `[64,1]` 的双动态 valid-region 类型可用 `Tile(rows, 1)`
构造；列 valid-region 模板参数固定为 `1` 的类型使用 `Tile(rows)`。两种构造
只设置有效区域，不改变类型声明的物理 `Rows/Cols`；对应 `TCMP` 的 `LB2`
均来自物理 `Cols=1`。`[2,16]` packed 类型不能通过 `validCol=1` 变成
`[32,1]`，与 one-column source 混用会在编译期被拒绝。
source 的 valid-region 模板参数仍须一致；动态有效区域的实际值一致且未越界
由调用方保证。此约束不添加运行时转换或 trap。

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
  using InputGM = global_tensor<float, RowMajor<64, 32>>;
  using ResultGM = global_tensor<float, RowMajor<64, 32>>;
  using InputTile = Tile<Location::Vec, float, 64, 32>;
  // RowMajor packed predicate carrier; TSEL also accepts a distinct carrier type.
  using PredicateTile = Tile<Location::Vec, float, 64, 32>;
  using ResultTile = Tile<Location::Vec, float, 64, 32>;

  float left_data[64 * 32] = {};
  float right_data[64 * 32] = {};
  float result_data[64 * 32] = {};
  InputGM left_global(left_data);
  InputGM right_global(right_data);
  ResultGM result_global(result_data);
  InputTile left;
  InputTile right;
  PredicateTile predicate;
  ResultTile result;

  TLOAD(left, left_global);
  TLOAD(right, right_global);
  TCMP<CmpMode::EQ>(predicate, left, right);
  TSEL(result, predicate, left, right);
  TSTORE(result_global, result);
}
```

示例选择相等位置的 `left`，其他位置选择独立 false 源 `right`。

## Bundle 对照

```asm
BSTART.VEC TCMP, DataType
B.DATR      CMode, PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcLeft, SrcRight, mask=PE_MASK, last, ->Predicate<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `0`，function `13`，selector `0x00D`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/logical/TCMP.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/logical/TCMP.asl)

[返回操作索引](../README.md)
