# TSEL

**逐元素 Tile–Tile** · **VEC** · 0.59 活动目录

## 简介

按 predicate 逐元素选择两个数值源的原始位。接口按 pto-spec
`d0884b821c6df76a37b82cdd92a36392df9c610a` 的 `PTO-TSEL-CONTRACT-001`
实现；独立输出不需要预先初始化。

```cpp
TSEL(dst, predicate, src_true, src_false);
TSEL(dst, predicate, src_true); // 等价于 TSEL(dst, predicate, src_true, dst)
```

predicate 为 1 时选择 `src_true`，为 0 时选择 `src_false`。三参数兼容形式
以调用前的 `dst` 为 false 源，必须先初始化。源与输出可以别名，选择观察
写入前的源值；相同 true/false 源也合法。

## 数学语义

逐坐标选择 `p ? src_true : src_false` 的原始位。三参数重载使用调用前的目标值作为 false 源。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v P, is_tile_data_v T,
          is_tile_data_v F>
void TSEL(D &dst, const P &mask, const T &when_true,
                            const F &when_false);

template <is_tile_data_v D, is_tile_data_v P = D, is_tile_data_v T = D>
void TSEL(D &dst, const P &mask, const T &when_true);

template <is_tile_data_v D, is_tile_data_v T, is_tile_data_v F>
void TSEL(D &dst, uint64_t mask, const T &when_true,
                            const F &when_false);

template <is_tile_data_v D, is_tile_data_v T, is_tile_data_v F>
void TSEL(D &dst, pto::predicate::GPRMask mask,
                            const T &when_true, const F &when_false);

template <is_tile_data_v D, is_tile_data_v T>
void TSEL(D &dst, uint64_t mask, const T &when_true);

template <is_tile_data_v D, is_tile_data_v T>
void TSEL(D &dst, pto::predicate::GPRMask mask,
                            const T &when_true);
```

## 约束与默认值

### Predicate 形式

- RowMajor：`TCMP`/`TCMPS` 产生的 packed predicate Tile。
- CUBE_M16/M32：`TCMP`/`TCMPS` 产生的 PredicateCell Tile，或
  `predicate::Predicate<Basis>`。PredicateCell 是逻辑布尔载体，不是普通
  Numeric U8 Tile；其比较 basis 不限制所选数值的类型或位宽。布局与逻辑
  有效形状必须匹配数值源，物理列数可以因 U8 存储而不同。
- CUBE_M16/M32 GPR：16/32/64-bit 操作使用 `uint64_t`；所有 8-bit 操作
  使用 `predicate::GPRMask{low, high}`，完整提供两个 64-bit mask word。
  CUBE_M16 不支持 64-bit 数值。

```cpp
// 类型化 PredicateCell；Basis 与所选数据的类型可以不同。
auto pred = pto::predicate::compare_scalar<pto::CmpMode::GE>(basis, threshold);
TSEL(dst, pred, src_true, src_false);
pto::predicate::select(dst, pred, src_true, src_false); // 同一实现

// GPR：16/32/64-bit 操作。
TSEL(dst, uint64_t(mask_bits), src_true, src_false);
// GPR：8-bit 操作，例如 U8、S8、E4M3、E5M2。
TSEL(dst8, pto::predicate::GPRMask{low_bits, high_bits}, true8, false8);
```

GPR mask 的覆盖范围由操作类型和布局决定：

| 布局 | 有效行上限 | 8-bit 有效列上限（两个 word） | 16-bit | 32-bit | 64-bit |
| --- | --- | --- | --- | --- | --- |
| CUBE_M16 | 16 | 8 | 4 | 2 | 不支持 |
| CUBE_M32 | 32 | 4 | 2 | 2 | 2 |

每个 word 内第 `column` 个字段使用 `row + column * 行位数` 位；CUBE_M16 每字段占 16 位，
CUBE_M32 每字段占 32 位。8-bit 类型先消费 low word 的字段，再消费 high
word 的字段。

### 数值类型与形状

操作类型取 `dst::DType`，支持 FP64、FP32、TF32、HF32、FP16、BF16、
E4M3、E5M2、S64、S32、S16、S8、U64、U32、U16、U8。两个数值源分别校验，
可与操作类型不同，但必须是等元素位宽、非 4-bit 的载体。CUBE 数值源
使用上述类型域；RowMajor 还允许 HiF8、E3M2、E2M3、E8M0 原始载体。
选择不执行转换、舍入、饱和或 NaN 规范化，也不更新浮点状态。

数值操作数必须为非 boxed 的 Local RowMajor/CUBE_M16/CUBE_M32 Tile，
物理形状、布局和有效形状一致。支持四种静态/动态有效维度组合；动态输出
的有效形状从源记录，独立输出不会读取未初始化的旧形状。类型化 Predicate
目前仍由静态有效维度的 producer 构造。

所有有效源元素与 predicate 必须已定义。PredicateCell 有效字节须为规范的
0/1，普通 U8 数据不能冒充 predicate。非法描述符、未定义源和非法 predicate
的执行时拒绝由目标实现负责；C++ 对静态类型和形状使用编译期检查，动态形状
使用断言。padding 使用 Null，其值和定义性不可作为有效结果读取。

## 示例

```cpp
#include <common/pto_tileop.hpp>
using namespace pto;
using TileT = Tile<Location::Vec, float, 64, 32>;
using GM = global_tensor<float, RowMajor<64, 32>>;

void select_values(GM &out, GM &left_mem, GM &right_mem) {
  TileT left, right, dst, pred;
  TLOAD(left, left_mem);
  TLOAD(right, right_mem);
  TCMP<CmpMode::LT>(pred, left, right);
  TSEL(dst, pred, left, right);
  TSTORE(out, dst);
}
```

## Bundle 对照

Tile predicate / PredicateCell：

```asm
BSTART.TEPL TSEL, DataType
B.DATR CUBE_M16/CUBE_M32, Null  ; CUBE 形式；RowMajor 省略
B.DIM ValidCol ->LB0
B.DIM ValidRow ->LB1
B.DIM PhysicalCol ->LB2
B.IOT Predicate, SrcTrue, mask=1111
B.IOT SrcFalse, mask=1111, last, ->Dst<TSize>
```

GPR mask：

```asm
BSTART.TEPL TSEL, DataType
B.DATR CUBE_M16/CUBE_M32, Null
B.DIM ValidCol ->LB0
B.DIM ValidRow ->LB1
B.DIM PhysicalCol ->LB2
B.IOT SrcTrue, SrcFalse, mask=1111, last, ->Dst<TSize>
B.IOR [MaskLow], []            ; 16/32/64-bit
; B.IOR [MaskLow, MaskHigh], [] ; 8-bit
```

当前包装器对所有 PE 发射 `mask=1111`。`PE_MASK=0000` 和通用 ExecutionMask
控制不通过这些重载暴露。

活动目录编码为 `TEPL`，mode `0`，function `26`，selector `0x01A`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/elementwise-tile-tile/logical/TSEL.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/elementwise-tile-tile/logical/TSEL.asl)

[返回操作索引](../README.md)
