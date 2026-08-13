# Matrix post-processing compatibility surface

The active CUBE contract defines twelve named matrix operations and no separate
post-processing attribute command. The historical fixed-point wrapper names are
retained only to produce a compile-time diagnostic; they do not emit an
instruction bundle.

Requirements:

- `dst`、`a` 和所有辅助 Tile 都是普通 Local Tile。
- `right` 可以是普通 `Tile<Location::Right, ...>`，也可以是 `SharedTile<RightTile>`。
- 所有 `B.FPATR` 配置和可选 operand 都封装在唯一的 `options` 参数中。
- options 的类型在编译期确定模式；scalar descriptor 的值和 Tile 寄存器内容可在运行时确定。

## 基础类型

```cpp
using tile_a = Tile<Location::Left, __half, 32, 32>;
using tile_b = Tile<Location::Right, __half, 32, 32>;
using tile_fp32 = Tile<Location::Vec, __fp32, 32, 32>;
using tile_fp16 = Tile<Location::Vec, __half, 32, 32>;
using tile_bf16 = Tile<Location::Vec, __bf16, 32, 32>;
using tile_s8 = Tile<Location::Vec, int8_t, 32, 32>;
```

A/B/dst 的物理 Tile 必须满足 TileOP 的对齐和 512 B..32 KB active-size 约束。`dst` 的 valid shape 必须为 `M x N`。

## 无额外参数的转换

```cpp
TMATMUL_FIXP(dst_fp32, a, b, fixp::keep_acc());
TMATMUL_FIXP(dst_fp16, a, b, fixp::f16());
TMATMUL_FIXP(dst_bf16, a, b, fixp::bf16());
```

对应的 `PreQuantMode`：

| options | PreQuantMode | dst dtype |
| --- | ---: | --- |
| `fixp::keep_acc()` | `None` / 0 | FP32 或 S32 AccType |
| `fixp::f16()` | `F322F16` / 1 | FP16 |
| `fixp::bf16()` | `F322BF16` / 16 | BF16 |

普通 ReLU 通过链式调用启用，不需要额外 operand：

```cpp
TMATMUL_FIXP(dst_fp16, a, b, fixp::f16().relu());
```

## Scalar quant descriptor

需要 scalar quant parameter 的模式使用：

```cpp
uint64_t quant_desc = make_quant_descriptor(...);
TMATMUL_FIXP(dst_s8, a, b, fixp::s8(quant_desc));
```

`fixp::s8(uint64_t)` 是以下通用写法的快捷形式：

```cpp
TMATMUL_FIXP(
    dst_s8, a, b,
    fixp::scalar<FixpPreQuantMode::QF322S8Pre>(quant_desc));
```

通用 `fixp::scalar<Mode>(descriptor)` 支持全部 scalar-parameter mode：

- `REQS8Pre`
- `DEQF16`
- `SHIFTS322S16`
- `QF322S4Pre`
- `QF322S16Pre`
- `QF322S8Pre`
- `QF322HIF8Pre`
- `QF322FP8Pre`
- `QF322F32Pre`
- `QF322F16Pre`
- `QF322BF16Pre`
- `QS322BF16Pre`

scalar quant descriptor 通过 `B.IOR SrcReg0` 传递。64-bit descriptor 布局为：

- FP19 scale：`[31:13]`。
- S4 offset：S5 two's-complement，`[41:37]`。
- S8 offset：S9 two's-complement，`[45:37]`。
- S16 offset：S17 two's-complement，`[53:37]`。
- 未使用位必须为 0。

建议业务代码封装 descriptor builder，不要在 kernel 中重复手写位移。例如：

```cpp
constexpr uint64_t make_s8_quant(uint32_t fp19_scale, int16_t offset) {
  return (static_cast<uint64_t>(fp19_scale & 0x7ffff) << 13) |
         ((static_cast<uint64_t>(offset) & 0x1ff) << 37);
}
```

## LReLU

scalar LReLU 在任意尚未选择 ReLU 的 options 上调用 `.lrelu()`：

```cpp
uint64_t quant_desc = make_s8_quant(...);
uint64_t lrelu_fp19 = encoded_slope & 0x7ffff;

TMATMUL_FIXP(
    dst_s8, a, b,
    fixp::s8(quant_desc).lrelu(lrelu_fp19));
```

- `lrelu_fp19` 的低 19 bit 是 FP19 slope，高位必须为 0。
- scalar quant 使用 `B.IOR SrcReg0`。
- LReLU 使用 `B.IOR SrcReg1`。
- 只有 LReLU、没有 scalar quant 时会生成 `B.IOR [zero, lrelu],[]`。

## Vector quant parameter

vector quant parameter 使用 ordinary Local Tile：

```cpp
using quant_tile =
    Tile<Location::Vec, uint64_t, 2, 32,
         BLayout::RowMajor, 1, 32>;

quant_tile quant;
TMATMUL_FIXP(
    dst_fp16, a, b,
    fixp::vector<FixpPreQuantMode::VQF322F16Pre>(quant));
```

`fixp::s8(quant)` 是 `VQF322S8Pre` 的快捷形式：

```cpp
TMATMUL_FIXP(dst_s8, a, b, fixp::s8(quant));
```

通用 `fixp::vector<Mode>(tile)` 支持全部 vector-parameter mode：

- `VREQS8Pre`
- `VDEQF16`
- `VSHIFTS322S16`
- `VQF322S4Pre`
- `VQF322S16Pre`
- `VQF322S8Pre`
- `VQF322HIF8Pre`
- `VQF322F16Pre`
- `VQF322BF16Pre`
- `VQF322FP8Pre`
- `VQF322F32Pre`
- `VQS322BF16Pre`

vector quant Tile 的每个 64-bit element 使用与 scalar descriptor 相同的 bit layout。

参数 Tile 的 valid shape 必须为 `1 x N`。如果 `1 x N` 的逻辑数据不足 512 B，必须扩大物理 Rows/Cols 保证 Tile register 至少 512 B，同时用 `ValidRow=1, ValidCol=N` 保持有效区域。例如上例物理 shape 为 `2 x 32`，valid shape 为 `1 x 32`。

## PReLU

PReLU 参数是长度 N 的 FP19 Tile，通过 `.prelu(tile)` 追加：

```cpp
using fp19_tile =
    Tile<Location::Vec, uint64_t, 2, 32,
         BLayout::RowMajor, 1, 32>;

fp19_tile quant;
fp19_tile prelu;

TMATMUL_FIXP(
    dst_fp16, a, b,
    fixp::vector<FixpPreQuantMode::VQF322F16Pre>(quant)
        .prelu(prelu));
```

PReLU Tile 的 valid shape 必须为 `1 x N`，每个 element 的低 19 bit 保存 FP19 slope，高位为 0。它在 B.IOT source stream 中位于 quant parameter Tile 之后。

也可以配合无 quant 的转换：

```cpp
TMATMUL_FIXP(dst_fp16, a, b, fixp::f16().prelu(prelu));
```

## RowMax

RowMax 在 ReLU/quant/convert 之前基于 FullAcc 计算，dtype 必须是 FP32/S32 AccType。

### Fresh RowMax

```cpp
using row_max_tile =
    Tile<Location::Vec, __fp32, 32, 8,
         BLayout::RowMajor, 32, 1>;

row_max_tile row_max_out;
TMATMUL_FIXP(
    dst_fp32, a, b,
    fixp::keep_acc().row_max(row_max_out));
```

这会设置 `RowMaxEn=1, RowMaxInit=0`，没有 RowMaxIn。

### 累加已有 RowMax

```cpp
row_max_tile row_max_in;
row_max_tile row_max_out;

TMATMUL_FIXP(
    dst_fp32, a, b,
    fixp::keep_acc().row_max(row_max_in, row_max_out));
```

这会设置 `RowMaxEn=1, RowMaxInit=1`。source 顺序为 A、B、RowMaxIn，destination 顺序为 D、RowMaxOut。

RowMaxIn/Out 的 valid shape 必须为 `M x 1`，dtype 和 valid shape 必须一致。物理 Tile 仍必须至少 512 B，因此可以像示例一样扩大物理列数，使用 `ValidCol=1`。

## GroupMax

GroupMax 的 group width 是编译期参数：

```cpp
using group_max_tile =
    Tile<Location::Vec, __fp32, 32, 8,
         BLayout::RowMajor, 32, 4>;

group_max_tile group_max_out;
TMATMUL_FIXP(
    dst_fp32, a, b,
    fixp::keep_acc().group_max<8>(group_max_out));
```

支持的 `GroupN`：`8, 16, 32, 48, 64, 80, 96, 112, 128`。

GroupMaxOut：

- valid shape 必须为 `M x ceil(N / GroupN)`；
- dtype 必须是 FP32/S32 AccType；
- 物理 Tile 必须满足 512 B..32 KB active-size 约束。

例如 N=32、GroupN=8 时，valid columns 是 4。

## RowMax + GroupMax + MaxAbs

各功能可链式组合：

```cpp
TMATMUL_FIXP(
    dst_fp32, a, b,
    fixp::keep_acc()
        .row_max(row_max_in, row_max_out)
        .group_max<8>(group_max_out)
        .max_abs());
```

- `.max_abs()` 只能在已启用 RowMax 或 GroupMax 后调用。
- 它同时作用于所有启用的 max reduction。
- destination 紧凑顺序固定为 D、RowMaxOut、GroupMaxOut。
- TileOP 会生成多条 destination-only `B.IOT`，仅最后一条带 `last`。

## Shared Right

接口和 options 完全相同，只替换 Right operand 类型：

```cpp
using right_tile = Tile<Location::Right, __half, 32, 32>;
SharedTile<right_tile> shared_b;

TMATMUL_FIXP(dst_fp32, a, shared_b, fixp::keep_acc());
TMATMUL_FIXP(dst_s8, a, shared_b, fixp::s8(quant_desc));
```

Shared form 生成：

```asm
C.B.IOS S#right
B.IOT   A
```

- Shared B 不进入普通 B.IOT source stream。
- A、quant/PReLU/RowMax inputs 和所有 outputs 仍是 Local Tile。
- Shared form 固定四 PE cooperative，mask 为 `1111`。
- Shared Right 当前要求静态 valid N。

## 通用模式选择

不需要额外参数的模式：

```cpp
fixp::convert<FixpPreQuantMode::F322F16>()
```

需要 scalar descriptor 的模式：

```cpp
fixp::scalar<FixpPreQuantMode::QF322S16Pre>(descriptor)
```

需要 vector parameter Tile 的模式：

```cpp
fixp::vector<FixpPreQuantMode::VQF322S16Pre>(parameter_tile)
```

然后按需链式追加：

```cpp
.relu()
.lrelu(fp19_descriptor)
.prelu(fp19_tile)
.row_max(row_max_out)
.row_max(row_max_in, row_max_out)
.group_max<GroupN>(group_max_out)
.max_abs()
```

编译器会拒绝不完整或冲突的组合，例如 vector mode 没有 quant Tile、PReLU mode 没有 PReLU Tile、RowMaxInit 没有 RowMaxIn、GroupMax shape 不匹配、dst dtype 与 PreQuantMode 不匹配。

## B.FPATR 与 operand 顺序

TileOP 固定生成：

```asm
BSTART.CUBE TMATMUL.FIXP, AType
B.DATR BType, byte0, Null
B.FPATR PreQuant, Relu, GroupNCode,
         RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn
B.DIM M, 0, ->lb0
B.DIM N, 0, ->lb1
B.DIM K, 0, ->lb2
```

Local source 顺序：

1. A
2. B
3. RowMaxIn（仅 `RowMaxInit=1`）
4. vector quant Tile（仅 vector PreQuant）
5. PReLU Tile（仅 PReLU）

GPR 顺序：

1. `B.IOR SrcReg0`：scalar quant descriptor
2. `B.IOR SrcReg1`：scalar LReLU FP19

Destination 顺序：

1. D
2. RowMaxOut（若启用）
3. GroupMaxOut（若启用）

## 旧三参数接口

基础模式仍兼容旧写法：

```cpp
TMATMUL_FIXP(dst_fp32, a, b);
TMATMUL_FIXP<FixpAttr::f16()>(dst_fp16, a, b);
```

新代码建议统一使用四参数形式。四参数 options API 才能表达 scalar/vector quant、LReLU/PReLU、RowMax、GroupMax 和 Shared Right 的完整组合。
