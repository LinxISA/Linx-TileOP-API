# Matrix post-processing compatibility surface

The active CUBE contract defines the matrix operations and carries all
post-processing through the `B.FPATR` attribute; there is no separate
post-processing attribute instruction. The historical `TMATMUL*.FIXP`
suffix was an implementation-local name; the active API is the options
overload of each `TMATMUL*`/`TGEMV*` operation, which configures `B.FPATR` and emits
the base `BSTART.CUBE TMATMUL` bundle plus the matching auxiliary operand
stream.

Requirements:

- `dst`、`a` 和所有辅助 Tile 都是普通 Local Tile；`a` 必须是 `Location::Left`，`b` 必须是 `Location::Right`。
- `right` 可以是普通 `Tile<Location::Right, ...>`，也可以是 `SharedTile<RightTile>`（或 `SharedTile<LeftTile>` 作为 `a`）。
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

## C++ 操作族与签名

12 个 active Matrix 操作共用同一个 `B.FPATR` options 机制。每个操作有
无 options（canonical None）和带 `const fixp::Options &` 两种重载；options
重载承担全部 PostProcess（scalar/vector quant、LReLU/PReLU、RowMax、
GroupMax、MaxAbs）与 Shared scale。数学源顺序与 handoff Sec 1.4 一致
（`C` 为显式累加器，`Dst`/`D` 为独立目标 tile）：

```cpp
TMATMUL<Attr>(Dst, A, B, options);                    // D = A*B
TMATMUL_BIAS<Attr>(Dst, A, B, Bias, options);         // D = A*B + Bias
TMATMUL_ACC<Attr>(Dst, C, A, B, options);             // D = C + A*B
TMATMUL_MX<Attr>(Dst, A, ScaleA, B, ScaleB, options); // D = (A*ScaleA)*(B*ScaleB)
TMATMUL_MX_BIAS<Attr>(Dst, A, ScaleA, B, ScaleB, Bias, options);
TMATMUL_MX_ACC<Attr>(Dst, C, A, ScaleA, B, ScaleB, options);
TGEMV<Attr>(Dst, Mtx, Vec, options);                  // D = Vec(M=1,K) * Mtx(K,N)
TGEMV_BIAS<Attr>(Dst, Mtx, Vec, Bias, options);
TGEMV_ACC<Attr>(Dst, C, Mtx, Vec, options);
TGEMV_MX<Attr>(Dst, Mtx, ScaleMtx, Vec, ScaleVec, options);
TGEMV_MX_BIAS<Attr>(Dst, Mtx, ScaleMtx, Vec, ScaleVec, Bias, options);
TGEMV_MX_ACC<Attr>(Dst, C, Mtx, ScaleMtx, Vec, ScaleVec, options);
```

无 options 调用（如 `TMATMUL(d, a, b)`）等价于 `TMATMUL(d, a, b,
fixp::Options<FixpAttr::keep_acc()>{})`，只接受参数-free 的
keep_acc/f16/bf16/relu 模式。

## TGEMV 家族

`TGEMV` 是 M=1 的矩阵-向量乘（Function 16-18、20-22）：

- `Vec` = 1×K（Left，逻辑 `ValidRow=1`），`Mtx` = K×N（Right），
  `Dst` = 1×N（逻辑 `ValidRow=1`）。
- 物理 Tile 仍需满足 512 B..32 KB active-size，因此向量通常用
  `Tile<Location::Left, T, K, K, BLayout::RowMajor, 1, K>` 这类满物理 +
  逻辑 1×K 的 shape。
- 所有 TGEMV 都是 Local-only；任何 `B.IOS` 都 illegal（handoff Sec 1.5），
  Shared 参数在概念层被拒绝。
- `B.DIM` 角色相对 TMATMUL 反转：`LB0 = N`、`LB1 = M(=1)`、`LB2 = Col`。

## Shared / Local 存储形态

- plain `TMATMUL`/`TMATMUL_ACC`/`TMATMUL_BIAS`：允许 Local-A + Shared-B
  或 Shared-A + Shared-B；不支持仅 Shared-A（单 binder 保留给
  Shared-Right 形式）。
- `TMATMUL_MX*` options 重载：MX Shared pair 是 Shared-B/ScaleB（两个
  binder），或 Shared-A/ScaleA/B/ScaleB 全部 Shared（四个 binder）；
  scale 与配套 matrix 同存储。no-options 重载保持 scale Local-only。
- 所有 Shared binder 走独立有序 `B.IOS` 流；Local operand 仍走 `B.IOT`。

## B.FPATR 模式

`FixpPreQuantMode` 的取值及其输出数据类型（PTO ISA 0.58.3 `B.FPATR` 表）：

| 模式 | 值 | dst dtype |
| --- | ---: | --- |
| `None` | 0 | FP32（AccType 结果；v0.58 不再接受 S32 别名） |
| `F322F16` | 1 | FP16 |
| `VREQS8Pre` | 2 | S8 |
| `REQS8Pre` | 3 | S8 |
| `VDEQF16` | 4 | FP16 |
| `DEQF16` | 5 | FP16 |
| `VSHIFTS322S16` | 12 | S16 |
| `SHIFTS322S16` | 13 | S16 |
| `F322BF16` | 16 | BF16 |
| `QF322S4Pre` | 17 | S4X2 |
| `VQF322S4Pre` | 18 | S4X2 |
| `QF322S16Pre` | 19 | S16 |
| `VQF322S16Pre` | 20 | S16 |
| `VQF322S8Pre` | 23 | S8 |
| `QF322S8Pre` | 24 | S8 |
| `QF322HIF8Pre` | 25 | HiF8 |
| `QF322FP8Pre` | 26 | E4M3 |
| `QF322F32Pre` | 27 | FP32 |
| `VQF322HIF8Pre` | 28 | HiF8 |
| `QF322F16Pre` | 32 | FP16 |
| `VQF322F16Pre` | 33 | FP16 |
| `QF322BF16Pre` | 34 | BF16 |
| `QS322BF16Pre` | 35 | BF16 |
| `VQF322BF16Pre` | 36 | BF16 |
| `VQF322FP8Pre` | 37 | E4M3 |
| `VQF322F32Pre` | 38 | FP32 |
| `VQS322BF16Pre` | 39 | BF16 |

## 无额外参数的转换

```cpp
TMATMUL(dst_fp32, a, b, fixp::keep_acc());
TMATMUL(dst_fp16, a, b, fixp::f16());
TMATMUL(dst_bf16, a, b, fixp::bf16());
```

对应的 `PreQuantMode`：

| options | PreQuantMode | dst dtype |
| --- | ---: | --- |
| `fixp::keep_acc()` | `None` / 0 | FP32（AccType） |
| `fixp::f16()` | `F322F16` / 1 | FP16 |
| `fixp::bf16()` | `F322BF16` / 16 | BF16 |

普通 ReLU 通过链式调用启用，不需要额外 operand：

```cpp
TMATMUL(dst_fp16, a, b, fixp::f16().relu());
```

## Scalar quant descriptor

需要 scalar quant parameter 的模式使用：

```cpp
uint64_t quant_desc = make_quant_descriptor(...);
TMATMUL(dst_s8, a, b, fixp::s8(quant_desc));
```

`fixp::s8(uint64_t)` 是以下通用写法的快捷形式：

```cpp
TMATMUL(
    dst_s8, a, b,
    fixp::scalar<FixpPreQuantMode::QF322S8Pre>(quant_desc));
```

通用 `fixp::scalar<Mode>(descriptor)` 支持全部 scalar-parameter mode（见上表
中不带 `V` 前缀且接收 descriptor 的模式）。

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

TMATMUL(
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
TMATMUL(
    dst_fp16, a, b,
    fixp::vector<FixpPreQuantMode::VQF322F16Pre>(quant));
```

`fixp::s8(quant)` 是 `VQF322S8Pre` 的快捷形式：

```cpp
TMATMUL(dst_s8, a, b, fixp::s8(quant));
```

通用 `fixp::vector<Mode>(tile)` 支持全部 vector-parameter mode（见上表中带
`V` 前缀且接收 parameter Tile 的模式）。

vector quant Tile 的每个 64-bit element 使用与 scalar descriptor 相同的 bit
layout。

参数 Tile 的 valid shape 必须为 `1 x N`。如果 `1 x N` 的逻辑数据不足 512 B，
必须扩大物理 Rows/Cols 保证 Tile register 至少 512 B，同时用
`ValidRow=1, ValidCol=N` 保持有效区域。例如上例物理 shape 为 `2 x 32`，valid
shape 为 `1 x 32`。

## PReLU

PReLU 参数是长度 N 的 FP19 Tile，通过 `.prelu(tile)` 追加：

```cpp
using fp19_tile =
    Tile<Location::Vec, uint64_t, 2, 32,
         BLayout::RowMajor, 1, 32>;

fp19_tile quant;
fp19_tile prelu;

TMATMUL(
    dst_fp16, a, b,
    fixp::vector<FixpPreQuantMode::VQF322F16Pre>(quant)
        .prelu(prelu));
```

PReLU Tile 的 valid shape 必须为 `1 x N`，每个 element 的低 19 bit 保存 FP19
slope，高位为 0。它在 B.IOT source stream 中位于 quant parameter Tile 之后。

也可以配合无 quant 的转换：

```cpp
TMATMUL(dst_fp16, a, b, fixp::f16().prelu(prelu));
```

## RowMax

RowMax 在 ReLU/quant/convert 之前基于 FullAcc 计算，dtype 必须是 FP32/S32
AccType。

### Fresh RowMax

```cpp
using row_max_tile =
    Tile<Location::Vec, __fp32, 32, 8,
         BLayout::RowMajor, 32, 1>;

row_max_tile row_max_out;
TMATMUL(
    dst_fp32, a, b,
    fixp::keep_acc().row_max(row_max_out));
```

这会设置 `RowMaxEn=1, RowMaxInit=0`，没有 RowMaxIn。

### 累加已有 RowMax

```cpp
row_max_tile row_max_in;
row_max_tile row_max_out;

TMATMUL(
    dst_fp32, a, b,
    fixp::keep_acc().row_max(row_max_in, row_max_out));
```

这会设置 `RowMaxEn=1, RowMaxInit=1`。source 顺序为 A、B、RowMaxIn，
destination 顺序为 D、RowMaxOut。

RowMaxIn/Out 的 valid shape 必须为 `M x 1`，dtype 和 valid shape 必须一致。
物理 Tile 仍必须至少 512 B，因此可以像示例一样扩大物理列数，使用
`ValidCol=1`。

## GroupMax

GroupMax 的 group width 是编译期参数：

```cpp
using group_max_tile =
    Tile<Location::Vec, __fp32, 32, 8,
         BLayout::RowMajor, 32, 4>;

group_max_tile group_max_out;
TMATMUL(
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
TMATMUL(
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

TMATMUL(dst_fp32, a, shared_b, fixp::keep_acc());
TMATMUL(dst_s8, a, shared_b, fixp::s8(quant_desc));
```

Shared form 生成：

```asm
B.IOS S#right, mask=1111
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

编译器会拒绝不完整或冲突的组合，例如 vector mode 没有 quant Tile、PReLU
mode 没有 PReLU Tile、RowMaxInit 没有 RowMaxIn、GroupMax shape 不匹配、dst
dtype 与 PreQuantMode 不匹配。

## B.FPATR 与 operand 顺序

PTO ISA 0.58.3 在 `B.FPATR` 低位增加 `TransA` 与 `TransB`。TileOP 通过
`options.transpose_a()` / `options.transpose_b()` 设置；它们只作用于对应
的 cooperative Shared 主矩阵，Local 主矩阵使用 transpose 位属于非法组合。

TileOP 固定生成：

```asm
BSTART.CUBE TMATMUL, AType
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

基础模式仍兼容无 options 写法：

```cpp
TMATMUL(dst_fp32, a, b);
TMATMUL<FixpAttr::f16()>(dst_fp16, a, b);
```

新代码建议统一使用四参数形式。四参数 options API 才能表达 scalar/vector
quant、LReLU/PReLU、RowMax、GroupMax 和 Shared Right 的完整组合。

> 命名注记：`.FIXP` 后缀不是 PTO-ISA mnemonics 的一部分；历史
> `TMATMUL_FIXP` 函数已删除，统一由带 options 的 `TMATMUL` 承担，
> 都会发射
> `B.FPATR`；v0.58 规范用 `B.FPATR` 承载全部 post-process 属性。
