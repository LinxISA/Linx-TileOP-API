# TCMPS

**Tile 标量与立即数** · **VEC** · 0.59 活动目录

## 简介

`TCMPS` 按照 CMode 将每个有效 Tile 元素与每 PE 标量比较，并将谓词结果编码为 0 或 1。

Tile 输出重载显式保留源 CUBE_M16/M32 布局及比较模式，结果为以源操作类型
为 basis 的 PredicateCell，而非普通 Numeric U8。NORM 输出保持 packed
predicate 的既有编码；GPR 输出重载的 carrier 规则不变。

### 实验性强类型生产者

开发分支新增 `predicate::compare_scalar<Mode>(source, scalar)`，返回私有
carrier 的 `Predicate<Basis>`，避免将 CUBE PredicateCell、普通 Numeric U8
和 packed GPR 混为同一类型。它是独立封装，不改变本页既有 `TCMPS` 重载。
该接口尚未发布。参见[谓词与执行范围](../helpers_zh.md#谓词与执行范围)。

## 数学语义

逐坐标计算 `CmpMode(src[r,c], scalar)`，按目标布局编码谓词。

## C++ 接口

```cpp
template <CmpMode Mode, is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCMPS(tile_shape_out &dst, tile_shape_in &src, typename tile_shape_in::DType s);
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCMPS(tile_shape_out &dst, tile_shape_in &src, typename tile_shape_in::DType s);
template <CmpMode Mode, bool High = false, is_tile_data_v tile_shape_in>
uint64_t TCMPS(tile_shape_in &src, typename tile_shape_in::DType s);
```

### Destination assembly：`TCMPS_ASS`

```cpp
template <CmpMode Mode, is_tile_data_v D, is_tile_data_v S>
void TCMPS_ASS(D &assembled_dst, S &src, typename S::DType scalar);
template <is_tile_data_v D, is_tile_data_v S>
void TCMPS_ASS(D &assembled_dst, S &src, typename S::DType scalar); // Mode = EQ
```

`assembled_dst` 必须由 `range::assemble`（或其 `middle`/`last` 形式）构造，
scalar 类型必须为 source 的 `DType`。参数顺序与普通 `TCMPS` 相反：source、
scalar、assembled destination。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `s` | 标量或标量 Tile 操作数；具体载体由重载决定。 |

GPR 输出重载仅接受 `VEC` 上的 `CUBE_M16`/`CUBE_M32` source，并返回打包到一个
`uint64_t` carrier 中的 predicate bits。`High=true` 只对 `U8` 选择高半 carrier；
其他类型只能选择低半 carrier。调用者必须保证 valid shape 可以放入规范定义的
一个 carrier，不能用该重载表达跨多个 GPR 的完整结果。

### 重载选择

这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。

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

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using InputGM = global_tensor<float, RowMajor<64, 32>>;
  using ResultGM = global_tensor<int32_t, RowMajor<64, 32>>;
  using InputTile = Tile<Location::Vec, float, 64, 32>;
  using ResultTile = Tile<Location::Vec, int32_t, 64, 32>;
  float src_data[64 * 32] = {};
  int32_t dst_data[64 * 32] = {};
  InputGM src_global(src_data);
  ResultGM dst_global(dst_data);
  InputTile src;
  ResultTile dst;
  InputTile::DType scalar = 0.0f;
  TLOAD(src, src_global);
  TCMPS<CmpMode::EQ>(dst, src, scalar);
  TSTORE(dst_global, dst);
}
```

## Bundle 对照

```asm
BSTART.VEC TCMPS, DataType
B.DATR      CMode, PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcTile, mask=PE_MASK, last, ->Predicate<TSize>
B.IOR       ScalarGPR, zero, zero, ->zero (optional)
BSTOP
```

GPR 输出形式将末两条替换为：

```asm
B.IOT       SrcTile, mask=1111, last
B.IOR       [ScalarGPR], PredicateGPR
```

活动目录编码为 `TEPL`，mode `1`，function `13`，selector `0x02D`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/tile-scalar-and-immediate/logical/TCMPS.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/tile-scalar-and-immediate/logical/TCMPS.asl)

[返回操作索引](../README.md)
