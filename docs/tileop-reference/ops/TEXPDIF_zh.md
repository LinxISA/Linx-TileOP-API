# TEXPDIF

**逐元素 Tile–Tile** · **SFU** · 0.59 活动目录

## 简介

`TEXPDIF` 是 PTO ISA 0.59.0 清单中的 SFU 直接操作，对两个同形状 Local Tile
逐元素计算 `dst = exp(src0 - src1)`。它不是 broadcast expansion，也不能用
`TSUB` 后接 `TEXP` 替代。

## 数学语义

对每个有效坐标计算 `dst[r,c] = exp(src0[r,c] - src1[r,c])`。差与指数遵循该 selector 的数值合同。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TEXPDIF(D &dst, A &src0, B &src1);
```

源操作类型与 destination 类型的合法组合为 FP16→FP16、BF16→BF16、
FP32→FP32、FP16→FP32 和 BF16→FP32。两个源必须选择相同的操作类型；源
descriptor 可以通过 `reinterpret_tile` 使用独立的等宽、非 packed backing carrier。

支持 RowMajor、CUBE_M16 和 CUBE_M32 Local layout；Shared、CUBE_N8、mixed
layout、shape 或 valid-region 不匹配均在编译期拒绝。源和 destination 的物理容量
按各自 backing/destination dtype 计算。

## 约束与默认值

### Backend 状态

CPU simulator 实现普通数值路径并保留 source-before-publication 的 alias 行为。
JCORE 内联汇编直接发射 selector，LLVM LinxV5 MC
catalog/parser 也已登记该 selector。当前尚未实现从 LLVM IR 或 builtin
到 `TEXPDIF` 的 compiler lowering，因此不能把 MC catalog 支持误称为 LLVM
编译器端到端支持。AArch64/SME 当前明确诊断为 unsupported；不会静默降级
为两个独立操作。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void exp_difference(float *output, const float *left, const float *right) {
  using GM = pto::global_tensor<float, pto::RowMajor<32, 32>>;
  using Data = pto::Tile<pto::Location::Vec, float, 32, 32>;
  GM lhs(left), rhs(right), destination(output);
  Data first, second, result;
  TLOAD(first, lhs);
  TLOAD(second, rhs);
  TEXPDIF(result, first, second);
  TSTORE(destination, result);
}
```

## Bundle 对照

```asm
BSTART.TEPL 29, SrcOperationType
B.DATR      Layout, DstDataType, Null   ; mixed dtype/layout 时合并为一条
B.DIM       ValidCol, 0, ->LB0
B.DIM       ValidRow, 0, ->LB1
B.DIM       Col, 0, ->LB2
B.IOT       SrcTile0, SrcTile1, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

`B.IOT` 的 source 顺序固定为 `src0, src1`，且只有一个 terminating Local
binding；不会生成 `B.IOR`、`B.IOS` 或额外 binding。RowMajor 同类型形式保持
最小 encoding。实现的 selector 是 TEPL mode 0/function 29 (`0x01D`)。

活动目录编码为 `TEPL`，mode `0`，function `29`，selector `0x01D`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/transcendental/TEXPDIF.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/transcendental/TEXPDIF.asl)

[返回操作索引](../README.md)
