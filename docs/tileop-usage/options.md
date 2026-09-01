# `fixp::Options` 使用指南

本文说明矩阵类 TileOP 的后处理 options。当前 C++ 实现位于
[`include/common/pto_tile.hpp`](../../include/common/pto_tile.hpp)，其架构属性对应 PTO
规范的 [`B.FPATR`](https://github.com/PTO-ISA/pto-spec/blob/main/docs/block/attributes/B.FPATR.md)。
本文中的 API、字段和合法组合以当前仓库为准；具体矩阵操作是否支持某个属性，还必须同时
满足对应的 `TMATMUL*`、`TGEMV*` 操作页约束。

## 1. 快速开始

普通计算直接使用基础重载；需要后处理时，在对应的 `TMATMUL*` 或 `TGEMV*` 调用中传入
一个 options 对象：

```cpp
TMATMUL(d, a, b);                                  // 默认属性
TMATMUL(d, a, b, fixp::f16());                      // FP32 -> FP16
TMATMUL(d, a, b, fixp::f16().relu());               // 转换并应用 ReLU
TMATMUL(d, a, b, fixp::s8(scalar_descriptor));      // scalar quant
TMATMUL(d, a, b, fixp::s8(quant_tile));             // vector quant
TMATMUL(d, a, b, fixp::keep_acc()
                    .row_max(row_out)
                    .group_max<16>(group_out)
                    .max_abs());                   // reductions
```

按需求选择 builder：

| 需求 | builder |
| --- | --- |
| 保留 accumulator 类型 | `fixp::keep_acc()` |
| 转换为 FP16 / BF16 | `fixp::f16()` / `fixp::bf16()` |
| 无参数转换 | `fixp::convert<Mode>()` |
| 需要 scalar descriptor 的模式 | `fixp::scalar<Mode>(descriptor)` |
| 需要 vector parameter Tile 的模式 | `fixp::vector<Mode>(tile)` |
| S8 scalar / vector quant 快捷方式 | `fixp::s8(descriptor)` / `fixp::s8(tile)` |

builder 返回新的 options，可以继续链式配置：

```cpp
auto options = fixp::s8(descriptor)
                   .lrelu(fp19_slope)
                   .row_max(row_in, row_out)
                   .group_max<16>(group_out)
                   .max_abs();
```

## 2. options 做什么

带 options 的矩阵调用将矩阵乘的结果与固定点/数值后处理配置放在同一个 CUBE bundle 中：

```cpp
TMATMUL(d, a, b, options);
TMATMUL_ACC(d, c, a, b, options);
TMATMUL_BIAS(d, a, b, bias, options);
TMATMUL_MX(d, a, scale_a, b, scale_b, options);
```

`B.FPATR` 是一个 **32-bit block header command**，它只记录当前 bundle 的后处理描述符，
不会单独执行 tile body 操作。它必须在 CUBE Matrix header 中恰好出现一次，并且位于
scalar/tile binding 和 body 之前；完整 header、维度、operand schema 和 body 通过检查后，
矩阵操作才会消费这些字段。

不需要后处理时，使用不带 options 的基础重载。当前 C++ 基础重载不是“不生成
`B.FPATR`”，而是使用默认的 `FixpAttr{}`（所有业务字段为零）生成默认属性。显式
options 重载则允许修改这些字段。

这与手写 bundle 时省略 `B.FPATR` 不同：CUBE Matrix bundle 仍要求恰好一个 `B.FPATR`；
编码为默认值的 `B.FPATR` 才表示明确关闭转换、激活和 reduction。

## 3. `FixpAttr` 与 `Options` 类型

`FixpAttr` 保存编译期确定的 B.FPATR 字段；`Options` 还保存由该属性选择的运行时
descriptor 和可选 Tile 指针。options 的类型会把 quant/PReLU/RowMax 等 operand 类型
编码进模板，因此不完整或冲突的组合通常在编译期报错。

```cpp
namespace pto {
struct FixpAttr;
enum class FixpPreQuantMode : uint8_t;
enum class FixpReluMode : uint8_t;
namespace fixp {
template <FixpAttr Attr_, typename QuantTile = NoOperand,
          typename ReluTile = NoOperand, typename RowMaxIn = NoOperand,
          typename RowMaxOut = NoOperand, typename GroupMaxOut = NoOperand,
          typename CScaleTile = NoOperand>
struct Options;
}
}
```

常用入口如下：

| builder | 作用 |
| --- | --- |
| `fixp::keep_acc()` | `PreQuant=None`，保留 FP32/S32 accumulator 类型 |
| `fixp::f16()` | `F322F16`，输出 FP16 |
| `fixp::bf16()` | `F322BF16`，输出 BF16 |
| `fixp::convert<Mode>()` | 无参数转换；Mode 只能是 `None`、`F322F16` 或 `F322BF16` |
| `fixp::scalar<Mode>(descriptor)` | 需要 64-bit scalar quant descriptor 的模式 |
| `fixp::vector<Mode>(parameter_tile)` | 需要 Local vector parameter Tile 的模式 |
| `fixp::s8(descriptor)` | `scalar<QF322S8Pre>` 快捷方式 |
| `fixp::s8(parameter_tile)` | `vector<VQF322S8Pre>` 快捷方式 |

builder 返回新 options，可以继续链式配置。也可以显式写
`fixp::Options<FixpAttr::keep_acc()>{}`，但常用 helper 更易读。

## 4. B.FPATR 字段

业务字段的表达顺序为：

```text
PreQuantMode, ReluMode, GroupNCode,
RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn,
TransA, TransB, CScaleEn
```

| 字段 | C++ 表达 | 语义 |
| --- | --- | --- |
| `PreQuantMode` | `FixpPreQuantMode` | 选择 accumulator 的预量化、反量化、移位或输出转换 |
| `ReluMode` | `FixpReluMode` | None、ReLU、scalar LReLU 或 vector PReLU |
| `GroupNCode` | `.group_max<GroupN>()` | 选择每组列数；0 表示关闭 GroupMax |
| `RowMaxEn` | `.row_max(...)` | 启用 RowMax reduction 及其输出 |
| `GroupMaxEn` | `.group_max<...>(...)` | 启用 GroupMax 输出 |
| `RowMaxInit` | `row_max(row_in, row_out)` | 从 RowMaxIn 累加已有 RowMax；单参数为 fresh reduction |
| `MaxAbsEn` | `.max_abs()` | 对已启用的 RowMax/GroupMax 计算 maximum absolute value |
| `TransA` | `.transpose_a()` | 对 A 选择逻辑转置；只允许规范支持的 Shared A |
| `TransB` | `.transpose_b()` | 对 B 选择逻辑转置；只允许规范支持的 Shared B |
| `CScaleEn` | `.cscale(scale_tile)` | 启用每行 accumulator C scale；只适用于 FP32 `TMATMUL_ACC`/`TMATMUL_MX_ACC` |

当前 header 的 `FixpAttr::encoding()` 位布局如下：

| 字段 | 位范围 | 位宽 |
| --- | ---: | ---: |
| `PreQuantMode` | `[31:26]` | 6 |
| `ReluMode` | `[25:23]` | 3 |
| `GroupNCode` | `[22:19]` | 4 |
| `RowMaxEn` | `[18]` | 1 |
| `GroupMaxEn` | `[17]` | 1 |
| `RowMaxInit` | `[16]` | 1 |
| `MaxAbsEn` | `[15]` | 1 |
| reserved | `[14:10]` | 5 |
| `CScaleEn` | `[9]` | 1 |
| `TransB` | `[8]` | 1 |
| `TransA` | `[7]` | 1 |
| fixed/discriminator bits | `[6:0]` | 7 |

bit 10 是 reserved，必须为零。低位固定编码由当前实现的 `0x2023` 提供；`Func`、
`ElementWiseEn`、`Opc1`、`Opcode` 和 `W` 等 discriminator 必须使用规范规定的固定值。
固定位不匹配时不会解码为 `B.FPATR`。

可用 `encoding()` 验证 builder 结果：

```cpp
constexpr auto transposed = fixp::keep_acc().transpose_a().transpose_b();
static_assert(transposed.encoding() == 0x000021a3u);
constexpr auto cscaled = fixp::keep_acc().cscale_enable();
static_assert(cscaled.encoding() == 0x00002223u);
```

## 5. PreQuantMode 取值与参数类别

参数类别以 header 中的 `is_scalar_fixp_pre_quant()` 和 `is_vector_fixp_pre_quant()` 为准，
不要仅根据枚举名称推断。通常名称中的 `V` 表示 vector parameter，未带 `V` 的参数化模式
使用 scalar descriptor；`None`、`F322F16` 和 `F322BF16` 不需要参数。

| 编码 | 枚举值 | 参数类别 |
| ---: | --- | --- |
| 0 | `None` | 无参数，保留 accumulator |
| 1 | `F322F16` | 无参数，转 FP16 |
| 2/3 | `VREQS8Pre` / `REQS8Pre` | vector / scalar |
| 4/5 | `VDEQF16` / `DEQF16` | vector / scalar |
| 12/13 | `VSHIFTS322S16` / `SHIFTS322S16` | vector / scalar |
| 16 | `F322BF16` | 无参数，转 BF16 |
| 17/18 | `QF322S4Pre` / `VQF322S4Pre` | scalar / vector |
| 19/20 | `QF322S16Pre` / `VQF322S16Pre` | scalar / vector |
| 23/28 | `VQF322S8Pre` / `VQF322HIF8Pre` | vector |
| 24–27 | `QF322S8Pre`, `QF322HIF8Pre`, `QF322FP8Pre`, `QF322F32Pre` | scalar |
| 32/33 | `QF322F16Pre` / `VQF322F16Pre` | scalar / vector |
| 34/35 | `QF322BF16Pre` / `QS322BF16Pre` | scalar |
| 36 | `VQF322BF16Pre` | vector |
| 37/38 | `VQF322FP8Pre` / `VQF322F32Pre` | vector |
| 39 | `VQS322BF16Pre` | vector |

合法编码集合是 `0..5`、`12..13`、`16..20`、`23..28` 和 `32..39`；其他六位编码保留。
非零 mode 只能用于它指定的 S32 或 FP32 accumulator class；mode 0 才可以接受 FP32、
S32 或 U32，并保持原类型。目标 Tile dtype 也必须与所选 mode 匹配。当前实现的主要输出
映射为：`None` 保留 FP32/S32/U32，FP32 转换 mode 输出 FP16/BF16，S8 mode 输出 S8，
S16 mode 输出 S16，S4 mode 输出 S4 packed，FP8/HiF8 mode 输出对应 FP8/HiF8，
F16/BF16/F32 mode 输出对应浮点类型。最终判断以操作重载的静态检查为准。

### 5.1 Scalar descriptor

```cpp
uint64_t desc = make_s8_quant(scale, offset);
TMATMUL(d_s8, a, b,
        fixp::scalar<FixpPreQuantMode::QF322S8Pre>(desc));
```

scalar descriptor 通过 `B.IOR SrcReg0` 传递。64-bit descriptor 位布局为：FP19 scale
`[31:13]`；S4 offset 为 S5 two's-complement `[41:37]`；S8 offset 为 S9 two's-complement
`[45:37]`；S16 offset 为 S17 two's-complement `[53:37]`；未使用位必须为 0。

```cpp
constexpr uint64_t make_s8_quant(uint32_t fp19_scale, int16_t offset) {
  return (static_cast<uint64_t>(fp19_scale & 0x7ffff) << 13) |
         ((static_cast<uint64_t>(offset) & 0x1ff) << 37);
}
```

上面的函数只构造 S8 descriptor。S8 offset 使用 9-bit two's-complement；S4 和 S16 descriptor
分别使用规范规定的 5-bit 和 17-bit offset 字段。构造其他 mode 的 descriptor 时不要复用
`make_s8_quant` 的字段位置或掩码；未使用位必须为零。

### 5.2 Vector parameter Tile

```cpp
using Quant = Tile<Location::Vec, unsigned long, 2, 32,
                   BLayout::RowMajor, 1, 32>;
Quant quant;
TMATMUL(d, a, b,
        fixp::vector<FixpPreQuantMode::VQF322F16Pre>(quant));
```

vector quant Tile 是普通 Local Tile，valid shape 必须为 `1 x N`。如果有效区域小于物理 Tile
容量，仍应使用合法的物理 shape，并用 `ValidRow=1, ValidCol=N` 表示有效区域。每个 64-bit element
使用与 scalar descriptor 相同的位布局。

## 6. 激活函数

`ReluMode` 的编码为：`None=0`、`Relu=1`、`LRelu=2`、`PRelu=3`；其他 3-bit 值保留。

ReLU 不需要额外 operand：

```cpp
TMATMUL(d, a, b, fixp::f16().relu());
```

LReLU 使用 scalar FP19 slope，通过 `B.IOR SrcReg1` 传递：

```cpp
uint64_t slope = encoded_slope & 0x7ffff; // 高位必须为 0
TMATMUL(d, a, b, fixp::s8(desc).lrelu(slope));
```

PReLU 使用长度为 N 的 Local Tile。每个 element 的低 19 bit 保存 FP19 slope，高位必须
为 0，valid shape 为 `1 x N`；有 quant Tile 时它在 quant Tile 之后进入 Local source stream。

```cpp
using PRelu = Tile<Location::Vec, unsigned long, 2, 32,
                   BLayout::RowMajor, 1, 32>;
PRelu prelu;
TMATMUL(d, a, b, fixp::f16().prelu(prelu));
```

## 7. RowMax、GroupMax 和 MaxAbs

RowMax 在 ReLU、quant 和 convert **之前**基于 FullAcc 计算，输入/输出 dtype 必须是 FP32
或 S32 AccType，valid shape 必须是 `M x 1`，且输入输出 dtype/valid shape 一致。

```cpp
using Row = Tile<Location::Vec, __fp32, 32, 8,
                 BLayout::RowMajor, 32, 1>;
Row row_out;
TMATMUL(d, a, b, fixp::keep_acc().row_max(row_out)); // RowMaxInit=0

Row row_in;
TMATMUL(d, a, b, fixp::keep_acc().row_max(row_in, row_out)); // RowMaxInit=1
```

Local auxiliary Tile 的物理 active size 必须处于当前实现允许的 `128 B..256 KiB` 范围内，
但具体操作还可能有更严格的限制。valid shape 变小不会降低物理容量要求；可以增大物理
shape，并用 `ValidCol=1` 表示有效区域。

GroupN 必须是 `8, 16, 32, 48, 64, 80, 96, 112, 128` 之一，对应 `GroupNCode=1..9`；
编码 0 表示关闭 GroupMax。

```cpp
using Group = Tile<Location::Vec, __fp32, 32, 8,
                   BLayout::RowMajor, 32, 4>;
Group group_out;
TMATMUL(d, a, b, fixp::keep_acc().group_max<8>(group_out));
```

`GroupMaxOut` 的 valid shape 必须为 `M x ceil(N / GroupN)`，dtype 为 FP32/S32 AccType，
物理 active-size 同样必须处于当前实现允许的 `128 B..256 KiB` 范围内。例如 `N=32,
GroupN=8` 时有效列数为 4。
`.max_abs()` 必须在 RowMax 或 GroupMax 已启用后调用，并同时作用于所有已启用的 max reduction：

```cpp
TMATMUL(d, a, b, fixp::keep_acc()
          .row_max(row_in, row_out).group_max<8>(group_out).max_abs());
```

输出顺序固定为 `D, RowMaxOut, GroupMaxOut`；多个 destination-only `B.IOT` 中只有最后一条
带 `last`。所有输出作为一个完整 output group 原子发布，非法 bundle 不会产生部分输出。

## 8. 转置、CScale 与 Shared operand

`TransA`/`TransB` 是独立的一位控制，但只有对应 primary operand 为 Shared 时合法，不能
把它当作任意 Local Tile layout 转换：

```cpp
TMATMUL(d, shared_a, shared_b,
        fixp::keep_acc().transpose_a().transpose_b());
```

`CScale` 用 Local scale Tile 启用，只被 FP32 accumulator 的 `TMATMUL_ACC` 和
`TMATMUL_MX_ACC` 接受。它必须是 U8、`CUBE_M32` layout，valid shape 为 `M x 1`：

```cpp
using CScaleTile = Tile<Location::Vec, uint8_t, 32, 32,
                         BLayout::CubeM32, 32, 1>;
// c_scale must have type CScaleTile (or another type satisfying the same contract).
TMATMUL_ACC(d, c, a, b, fixp::keep_acc().cscale(c_scale));
```

Shared Right 的 options 与普通 Right 完全相同，只替换 operand 类型。Shared B 不进入普通
Local `B.IOT` source stream，而由 `C.B.IOS S#right` 携带；其余输入输出仍是 Local Tile。
Shared form 使用四 PE cooperative、mask `1111`，Shared Right 当前要求静态 valid N。

### 操作支持速查

以下是当前 API 的使用方向；每个操作页仍可能根据输入 dtype、layout 和 accumulator 类型
施加更严格的限制。

| 属性 | `TMATMUL` | `TMATMUL_ACC` | `TMATMUL_BIAS` | `TMATMUL_MX*` | `TGEMV*` |
| --- | ---: | ---: | ---: | ---: | ---: |
| 类型转换 / quant / activation | ✓ | ✓ | ✓ | ✓ | ✓ |
| RowMax / GroupMax / MaxAbs | ✓ | ✓ | ✓ | ✓ | ✓ |
| 转置 | Shared A/B 约束 | Shared A/B 约束 | Shared A/B 约束 | Shared A/B 约束 | — |
| CScale | — | FP32 C | — | 仅 ACC 变体，FP32 C | — |

## 9. 完整合法性约束

- `RowMaxInit` 必须同时启用 `RowMaxEn`，并且必须提供 `RowMaxIn`；
- `GroupNCode != 0` 当且仅当 `GroupMaxEn=true`，且必须提供 `GroupMaxOut`；
- `MaxAbsEn` 必须至少伴随 RowMax 或 GroupMax；
- scalar mode 必须提供 scalar descriptor，vector mode 必须提供 parameter Tile；
- LReLU 必须提供 FP19 scalar descriptor，PReLU 必须提供 PReLU Tile；
- auxiliary Tile 必须是 Local Tile，不能重复绑定同一类 operand 或非法 alias；
- `A/B/D` 的 location、CUBE layout、dtype、M/N/K、valid shape 和物理容量必须满足所选
  `TMATMUL*` 重载，维度满足 `M×K` 乘 `K×N`；
- Local auxiliary Tile 的 active-size/SizeCode 必须在 `128 B..256 KiB` 范围内，不会因为
  valid shape 变小而降低容量要求；
- CScale 必须为 Local U8 `CUBE_M32` Tile，valid shape 为 `M x 1`，且 accumulator 为 FP32；
- `B.DATR` 负责 destination conversion controls。B.FPATR 存在时，None/fixed floating/
  fixed shift 的 RMode/Sat 组合必须符合规范；
- 完整 bundle 的 field、B.DATR、operand count、alias、shape 和 allocation preflight 在
  消费 source 或产生 destination effect 前完成。

非法枚举或固定位通常无法解码并产生 illegal-instruction；缺失/重复 B.FPATR、非 CUBE
Matrix 使用或完整组合不合法，会在 operand consumption、allocation 和输出之前被拒绝。

## 10. Bundle 与 operand 顺序

C++ 调用者无需手写汇编；生成代码可按以下结构核对：

```asm
BSTART.CUBE TMATMUL, AType
B.DATR      BType, RMode, Sat
B.FPATR     PreQuant, Relu, GroupNCode,
            RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn,
            TransA, TransB, CScaleEn
B.DIM       M
B.DIM       N
B.DIM       K
B.IOT       Local mathematical sources and post-process sources
B.IOT       D, RowMaxOut, GroupMaxOut
B.IOR       scalar quant descriptor, scalar LReLU slope
BSTOP
```

普通 Local source 顺序为 `A, B, RowMaxIn（若启用）`、vector quant Tile（若启用）、PReLU
Tile（若启用）。GPR 顺序为 `SrcReg0=scalar quant descriptor`、`SrcReg1=scalar LReLU FP19`；
destination 顺序为 `D, RowMaxOut（若启用）, GroupMaxOut（若启用）`。`TMATMUL_ACC` 会在
这些后处理 source 之前加入 accumulator C；`TMATMUL_MX*` 还会加入对应 A/B scale。

## 11. 使用示例

下面的示例包含所有声明，可作为 host syntax check 的最小代表。示例使用 `using namespace
pto;`，因此 `FixpPreQuantMode` 和 `Tile` 不需要重复写 `pto::`：

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using D = CubeAccumulatorM32<float, 32, 32>;
using Ds8 = CubeAccumulatorM32<int8_t, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;
using Param = Tile<Location::Vec, unsigned long, 2, 32,
                   BLayout::RowMajor, 1, 32>;
using Row = Tile<Location::Vec, float, 32, 32,
                 BLayout::RowMajor, 32, 1>;
using Group = Tile<Location::Vec, float, 32, 32,
                   BLayout::RowMajor, 32, 2>;
using CScale = Tile<Location::Vec, uint8_t, 32, 32,
                    BLayout::CubeM32, 32, 1>;

void options_example(D &d, Ds8 &d8, D &c, A &a, B &b,
                     Param &quant, Param &prelu, Row &row_in,
                     Row &row_out, Group &group_out, CScale &c_scale) {
  constexpr uint64_t descriptor =
      (static_cast<uint64_t>(7) << 13) |
      (static_cast<uint64_t>(1) << 37);
  TMATMUL(d8, a, b, fixp::s8(descriptor).lrelu(0x123));
  TMATMUL(d8, a, b, fixp::s8(quant).prelu(prelu));
  TMATMUL(d, a, b, fixp::keep_acc()
                       .row_max(row_in, row_out)
                       .group_max<16>(group_out)
                       .max_abs());
  TMATMUL_ACC(d, c, a, b, fixp::keep_acc().cscale(c_scale));
}
```

## 12. 常见错误与排查顺序

1. 先改用不带 options 的基础重载，确认 A/B/D 的 location、layout、dtype、shape 和容量。
2. 再只启用一种属性，并检查它要求的 Tile/descriptor 是否存在且 valid shape 正确。
3. 区分 scalar 与 vector mode；不要用 scalar descriptor 替代 vector Tile，反之亦然。
4. 检查 `RowMaxInit`、`GroupN`、`MaxAbs` 的依赖关系及 destination 顺序。
5. 生成 `.s`，确认 `B.FPATR`、`B.IOR`、`B.IOT`、Shared IOS 和 `last` 的顺序。

相关操作的具体约束请继续阅读：

- [TMATMUL](cube/matrix-matrix/TMATMUL.md)、[TMATMUL_ACC](cube/matrix-matrix/TMATMUL_ACC.md)、
  [TMATMUL_BIAS](cube/matrix-matrix/TMATMUL_BIAS.md)
- [TMATMUL_MX](cube/matrix-matrix/TMATMUL_MX.md)、[TMATMUL_MX_ACC](cube/matrix-matrix/TMATMUL_MX_ACC.md)、
  [TMATMUL_MX_BIAS](cube/matrix-matrix/TMATMUL_MX_BIAS.md)
- [TGEMV](cube/matrix-vector/TGEMV.md)、[TGEMV_ACC](cube/matrix-vector/TGEMV_ACC.md)、
  [TGEMV_BIAS](cube/matrix-vector/TGEMV_BIAS.md)
- [TGEMV_MX](cube/matrix-vector/TGEMV_MX.md)、[TGEMV_MX_ACC](cube/matrix-vector/TGEMV_MX_ACC.md)、
  [TGEMV_MX_BIAS](cube/matrix-vector/TGEMV_MX_BIAS.md)