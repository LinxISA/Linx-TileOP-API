# TEXPDIF

`TEXPDIF` 是 PTO ISA 0.58.7 的 SFU 直接操作，对两个同形状 Local Tile
逐元素计算 `dst = exp(src0 - src1)`。它不是 broadcast expansion，也不能用
`TSUB` 后接 `TEXP` 替代。

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

## Bundle

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

## Backend 状态

CPU simulator 实现普通数值路径并保留 source-before-publication 的 alias 行为。
JCORE emits the direct selector. AArch64/SME 当前明确诊断为 unsupported；不会
静默降级为两个独立操作。