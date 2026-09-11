# Tile range modifiers (B.SUBVIEW / B.ASSEMBLE)

PTO-ISA 0.58.4 (ADR-0098) adds two *range modifiers* that attach to the
immediately preceding `B.IOT`/`B.IOS` binder in the same contiguous
block-command group and describe a byte range against a base GPR:

```asm
B.SUBVIEW SrcSelect, RegSrc, uimm11, SubviewSizeCode    ; source side
B.ASSEMBLE INIT, LAST, RegSrc, uimm11, ParentSizeCode   ; destination side
```

The derived byte address is `(GPR[RegSrc] + ZeroExtend(uimm11)) * 128 B mod XLEN`, where
`RegSrc` is an absolute GPR selector `0..23`; both the selected GPR value and `uimm11` are counts of 128-byte units. `uimm11` is `0..2047`.
The high-level TileArray path passes each slot's 128-byte-unit range base in a
compiler-allocated GPR and uses `uimm11=0`; this preserves the ISA's
`GPR[RegSrc] + uimm11` semantics without exposing register numbers.

The API exposes a small view-building layer, inspired by block-pointer APIs:
create a range view once and pass it to the consuming operation. The low-level
single-binder carrier surface is `pto::range::subview(parent, base)` and
`pto::range::assemble(parent, base)`. These factories remain supported for
compatibility code, Shared `B.IOS` bindings, fixed-ABI tests, and instruction
encoding tests. For parent/fragment business code, prefer `TPARTVIEW` for
source partitioning and `TileArray`/`TASSEMBLY` for destination assembly. The
lower-level `Subview` and `Assemble` carrier types remain available for unusual
compile-time contracts.

At the current PTO-ISA 0.58.6 authority, the low-level `B.SUBVIEW` contract is
legal for an assigned **Local or Shared Matrix** parent (`Mat`, `Left`, `Right`,
or `Acc`) using a persistent CUBE CELL layout. Shared sources attach to `B.IOS`
and use the 0.58.5+ per-PE offset semantics. RowMajor/ColMajor parents and
`Location::Vec` parents (including Vec+CUBE) remain rejected at compile time.

The high-level `TPARTVIEW` API is narrower in the current implementation: it
currently accepts Local Matrix+CUBE parents only. A Shared `B.IOS` source through
`range::subview` is therefore not evidence that Shared `TPARTVIEW` is supported;
the latter remains a compile-time negative case until its multi-PE array
transport is implemented. The source restriction does not apply to a
`B.ASSEMBLE` destination, whose legality is determined separately by the
destination operation and carrier contract.

## Syntax and encoding

| Field | B.SUBVIEW | B.ASSEMBLE |
| --- | --- | --- |
| `match` | `0x53` | `0x1053` |
| bit 31 | `SrcSelect` | `INIT` |
| bits 30..20 | `uimm11` | `uimm11` |
| bits 19..15 | `RegSrc` | `RegSrc` |
| bit 11 | — | `LAST` |
| bits 10..7 | `SubviewSizeCode` | `ParentSizeCode` |

Legal ranges (enforced by `asl BundleRangeSub*RawLegal` and mirrored by the
LLVM assembler/disassembler and by the wrapper `static_assert`s):

- `SubviewSizeCode` `1..12` (128 B..256 KiB per PE); `0`, `13..15` reserved.
- `ParentSizeCode` `0..12`; `13..15` reserved. `0` is legal **only** on a
  non-INIT modifier:
  - `INIT=1` requires `ParentSizeCode=1..12` (`INIT` / `INIT_LAST` forms);
  - `INIT=0` requires `ParentSizeCode=0` (`MIDDLE` / `LAST` forms).
- `uimm11` `0..2047`; `RegSrc` an absolute GPR selector `0..23` (`r24..r31`
  and the VBX `t#1..4`/`u#1..4` encodings are rejected).

## Recommended view API

The common case derives the range size from the tile type and does not require
the caller to spell out a carrier type or size code:

```cpp
using TileT = CubeTileM16<float, 16, 16>;
using GM = global_tensor<float, RowMajor<16, 16>>;

TileT tile;
GM gm;

auto zero_based_source = range::subview(tile);
TSTORE(gm, zero_based_source);

auto runtime_based_source = range::subview(tile, base_units);
TSTORE(gm, runtime_based_source);

auto destination = range::assemble(tile, base_units);
TLOAD(destination, gm);
```

For a final assembled range, use the named lifecycle helper instead of
encoding `INIT=false, LAST=true` as positional template booleans:

```cpp
auto destination = range::assemble_last(tile, base_units);
TLOAD(destination, gm);
```

The source and destination factories use the same range arguments: an optional
byte length and an optional compile-time offset measured in 128-byte units:

```cpp
auto full_tile = range::subview(tile);                // default length, offset 0
auto sized = range::subview<1>(tile, base_units);     // 128 B (1 unit), offset 0
auto shifted = range::subview<1, 3>(tile, base_units);
auto destination = range::assemble<1, 3>(tile, base_units);
```

`LengthUnits`/`OffsetUnits` 都以 128 B 为单位（与编码的 uimm11 粒度一致）。
`LengthUnits` must be one of `1`, `2`, `4`, ..., `2048`（128 B × 2 的幂，即
128 B..256 KiB），and
must not exceed `tile::LogicalTileBytes`. The factory converts it to the ISA
`SubviewSizeCode` or INIT `ParentSizeCode` automatically and rejects invalid or
oversized values at compile time.
The low-level `subview_sized_at` helper remains available for direct ISA contract tests.

The explicit carrier forms documented below remain supported for code that
needs every descriptor field visible in the type.

## TileArray region API

For a parent Tile split into fixed-size fragments, prefer the region API:

```cpp
// TPARTVIEW currently requires a Local Matrix+CUBE parent.
using Parent = CubeTileM16<float, 16, 64>;
using Fragment = CubeTileM16<float, 16, 16>;
using GM = global_tensor<float, RowMajor<16, 64>>;

Parent parent;
GM gm;
auto source_tile = TPARTVIEW<Fragment, 1, 4>(parent)[0][2];
TSTORE(gm, source_tile);
```

`TPARTVIEW` returns a borrowed view and does not allocate another Tile register.
For destination assembly, `TileArray` owns the destination carrier and
`TASSEMBLY` materializes it as the requested parent Tile; it does not emit a
second standalone instruction. When a producer writes a destination slot, the
inline-asm wrapper emits the corresponding `B.ASSEMBLE` lifecycle form and the
slot range. For slot `ordinal`, the range base is
`ordinal * (FragmentBytes / 128)` and is supplied through a compiler-allocated
GPR.

`CubeTileM16` and `CubeTileM32` are supported by the Tile type and partition
contract checks. The current region producer inline-asm path is intentionally
limited to `RowMajor + NoneBox`; do not use `TCVT` or row-wise region producers
with Cube fragments until the Cube binder/CELL ordering path is implemented and
validated. The example above documents the high-level type contract; it must not
be read as evidence that the Cube producer/runtime path has already been
validated.

For a developer-oriented guide with complete lifecycle, Local/Shared, validation,
and generated-assembly examples, see [B.SUBVIEW / B.ASSEMBLE Developer Guide](range-modifiers-developer-guide.md).

## `range::Subview` — source-side range carrier

```cpp
template <typename Parent, unsigned SubviewSizeCode, unsigned OffsetUnits = 0,
          unsigned RegSrc = 2>
class Subview;
```

`Subview` 是底层 carrier 类型。普通 kernel 应优先使用统一的
`range::subview<LengthUnits = tile capacity, OffsetUnits = 0>(tile [, base_units])`
factory；调用者以 128 B 单位填写分片长度和偏移，而不是 ISA 编码。`RegSrc` 仅用于固定 ABI 或编码测试，
运行时 base 的高层接口不会暴露寄存器编号。

- `SubviewSizeCode` must be `1..12`; the high-level factory derives it from
  the requested byte length.
- `OffsetUnits` (the `uimm11` 128-byte-unit adder) is a compile-time constant `0..2047`.
- `RegSrc` is the low-level absolute GPR selector `0..23`; high-level factories
  use `zero` or compiler allocation and do not expose this field.

```cpp
using Src = CubeTileM16<float, 16, 16>;
using GM = global_tensor<float, RowMajor<16, 16>>;

Src s;
GM gm;
auto zero_based = range::subview(s);
TSTORE(gm, zero_based);  // source B.IOT; B.SUBVIEW 0, zero, 0, 4

auto runtime_based = range::subview(s, base_units);
TSTORE(gm, runtime_based); // compiler selects the encoded GPR
```

### Base-register selection

The high-level API does not expose `RegSrc`:

```cpp
range::subview(s);                    // zero base
range::subview(s, base_units);         // compiler-allocated GPR
range::subview<1>(s);                   // 128 B (1 unit), zero + offset 0
range::subview<1>(s, base_units);       // 128 B, runtime base + offset 0
range::subview<1, 3>(s, base_units);    // 128 B, runtime base + 384 B
```

Explicit register selection is retained only for fixed ABI and encoding tests:

```cpp
auto sv = range::subview_sized_at_reg<12, 2047, 23>(s, base_units);
TSTORE(gm, sv);  // source B.IOT; B.SUBVIEW 0, r23, 2047, 12
```

## `range::Assemble` — destination-side range carrier

```cpp
template <typename Parent, unsigned ParentSizeCode, bool INIT = true,
          bool LAST = false, unsigned OffsetUnits = 0, unsigned RegSrc = 2>
class Assemble;
```

- `ParentSizeCode` `0..12`, with the INIT contract above.
- `INIT`/`LAST` select the form (`INIT`, `INIT_LAST`, `MIDDLE`, `LAST`).
- `OffsetUnits` is `0..2047` and is measured in 128-byte units.
- `RegSrc` is a low-level field. Public factories use `zero` when no
  `base_units` is supplied, or let the compiler allocate a GPR when it is.
- INIT helpers derive `ParentSizeCode` from `LengthUnits`; MIDDLE/LAST encode
  the ISA-required value `0`.

```cpp
using Dst = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;

Dst d;
auto as = range::assemble<128, 3>(d, base_units);
TLOAD(as, gm);  // B.ASSEMBLE 1, 0, <compiler-gpr>, 3, 1
```

Non-INIT forms use `ParentSizeCode=0`:

```cpp
auto as = range::assemble_last<128, 2047>(d);
TLOAD(as, gm);  // B.ASSEMBLE 0, 1, zero, 2047, 0
```

## Vector `*_ASS` 接口

Vector TEPL 操作提供带 `_ASS` 后缀的 destination-assembly 版本。它们与同名
普通接口的主要区别是：destination 必须是 `range::assemble(...)` 返回的 carrier，
并且参数顺序统一为“assembled destination 在前、输入在后”。`range::assemble`
只描述 destination 的 assembly range，不复制 Tile 数据。`_ASS` 将已经关联的
destination 作为第一个 C++ 参数对应的 input-only destination binder 使用，不分配新的 Tile generation，
也不在该计算 bundle 中重新发出 `B.ASSEMBLE`。

```cpp
using TileT = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

TileT a, b, c, result;
auto assembled = range::assemble(result);       // INIT=1, LAST=0
TFMA_ASS(assembled, a, b, c);
```

需要描述一次 assembly session 的多个 fragment 时，carrier 仍按
`INIT -> MIDDLE* -> LAST` 构造（例如 `range::assemble`、
`range::assemble_middle` 和 `range::assemble_last`）。这些字段描述关联关系；
TEPL `_ASS` 只消费该 carrier 的现有 parent register。不能把普通 Tile 直接作为
TEPL `*_ASS` 的 destination，也不能把 `range::subview` 当作 destination。

**接口选择约束**（编译期 `static_assert` 强制）：session 的 INIT slot 必须用
普通分配型接口（`TLOAD` 等）写入，后续 MIDDLE/LAST slot 才用 `_ASS` 追加——
`_ASS` 收到 INIT carrier、或普通 `TLOAD` 收到非 INIT carrier 都会被拒绝。
详见开发者指南的"INIT slot 与后续 slot 的接口选择约束"。

### 接口形式

当前新增的 vector `_ASS` 接口按参数类别分为：

| 类别 | 接口形式 |
| --- | --- |
| tile/tile | `TADD_ASS`、`TSUB_ASS`、`TMUL_ASS`、`TDIV_ASS`、`TREM_ASS`、`TAND_ASS`、`TOR_ASS`、`TXOR_ASS`、`TSHL_ASS`、`TSHR_ASS`、`TMAX_ASS`、`TMIN_ASS(assembled_dst, src0, src1)` |
| 一元 | `TABS_ASS`、`TNOT_ASS`、`TNEG_ASS`、`TEXP_ASS`、`TLOG_ASS`、`TRECIP_ASS(assembled_dst, src)` |
| tile/scalar | `TADDS_ASS`、`TSUBS_ASS`、`TMULS_ASS`、`TDIVS_ASS`、`TREMS_ASS`、`TANDS_ASS`、`TORS_ASS`、`TXORS_ASS`、`TSHLS_ASS`、`TSHRS_ASS`、`TMAXS_ASS`、`TMINS_ASS(assembled_dst, src, scalar)` |
| 比较 | `TCMP_ASS<Mode>(assembled_dst, src0, src1)`、`TCMPS_ASS<Mode>(assembled_dst, src, scalar)`；省略 `Mode` 时默认为 `CmpMode::EQ` |
| 三元/一元/转换 | `TFMA_ASS(assembled_dst, a, b, c)`；`TSQRT_ASS`、`TRSQRT_ASS`、`TRELU_ASS`、`TTRANS_ASS`、`TCVT_ASS(assembled_dst, src)` |
| 归约 | `TROWSUM_ASS`、`TROWMAX_ASS`、`TROWMIN_ASS`、`TROWPROD_ASS`、`TCOLSUM_ASS`、`TCOLMAX_ASS`、`TCOLMIN_ASS`、`TCOLPROD_ASS(assembled_dst, src)` |
| 广播 | `TROWEXPAND_ASS(assembled_dst, src)`、`TCOLEXPAND_ASS(assembled_dst, src)` |
| 广播复合 | `TROWEXPAND{ADD,SUB,MUL,DIV,MAX,MIN,EXPDIF}_ASS`、`TCOLEXPAND{ADD,SUB,MUL,DIV,MAX,MIN,EXPDIF}_ASS(assembled_dst, src0, src1)` |
| 拼接 | `TCONCAT_ASS(assembled_dst, left, right)` |

`TCMP_ASS` 的两个输入 dtype 必须相同；`TCMPS_ASS` 的 scalar 必须是 source
的 `DType`；普通逐元素、归约和广播接口要求 source 与 destination dtype 匹配。
`TCVT_ASS` 允许转换 dtype，但 source 和 destination 的物理 shape 必须相同。
其余形状规则与对应普通接口页面相同。

### 生成的 TEPL bundle

`_ASS` wrapper 生成的共同结构如下；具体 opcode 由操作决定：

```asm
BSTART.TEPL <opcode>, <source-dtype>
B.DIM      <valid-col>, 0, ->lb0
B.DIM      <valid-row>, 0, ->lb1
B.DIM      zero, <physical-cols>, ->lb2
B.IOT      <source-operands>, mask=1111
B.IOT      <assembled-destination>, mask=1111, last
```

`TCVT_ASS` 另外发出 `B.DATR <destination-dtype>, RNONE`；比较接口另外发出
`B.DATR zero, <CmpMode>`，标量比较还发出 `B.IOR [<scalar>], []`。因此，
`range::assemble` 提供已有 destination 的关联描述，而 `_ASS` 负责 TEPL 计算
bundle；上述 bundle 中没有 destination arrow、Tile SizeCode 或 `B.ASSEMBLE`。
此前验证的 14 个特殊接口均生成了对应的 `BSTART.TEPL`：
`TCMP`、`TCMPS`、`TFMA`、`TSQRT`、`TRSQRT`、`TRELU`、`TCVT`、`TTRANS`、
`TROWSUM`、`TROWMAX`、`TCOLSUM`、`TROWEXPAND`、`TCOLEXPAND` 和 `TCONCAT`。

## Forwarded members

Both carriers expose every tile-shaped static member of `Parent`
(`Loc`, `Rows`, `Cols`, `RowStride`, `ColStride`, `ValidRow`, `ValidCol`,
`BFractal`, `SFractal`, `SFractalSize`, `isRowMajor`, `isBoxedLayout`,
`isInnerRowMajor`, `isInnerColMajor`, `InnerRows`, `InnerCols`, `Numel`,
`LogicalTileBytes`, `TilesizeCode`, `IsValidActiveSize`) plus `DType`,
`TileDType`, and:

- `GetValidRow()` / `GetValidCol()` — forward the parent's valid region;
- `GetRangeBase()` — the runtime base-address value given at construction;
- `data()` — the parent storage (Local parents only);
- `handle()` / `handle_ref()` — the Shared handle (SharedTile parents only).

Because the descriptor fields are compile-time, an operation can exercise an
arbitrary number of distinct ranges purely by specializing the wrapper — the
`uimm11`, `INIT`/`LAST` and size-code slots stay immediate operands.

## Emitted asm

A `TLOAD` on an `Assemble`-wrapped destination:

```asm
BSTART.TLSU TLOAD, F32
B.DIM <vcol>, 0, ->lb0
B.DIM <vrow>, 0, ->lb1
B.DIM zero, <cols>, ->lb2
B.IOT mask=1111, last, ->t<tsize>
B.ASSEMBLE <INIT>, <LAST>, <zero-or-compiler-gpr>, <OffsetUnits>, <ParentSizeCode>
B.IOR [<src>,<stride>], []
```

A `TSTORE` on a `Subview`-wrapped source:

```asm
BSTART.TLSU TSTORE, F32
B.DIM <vcol>, 0, ->lb0
B.DIM <vrow>, 0, ->lb1
B.DIM zero, <cols>, ->lb2
B.IOT <src>, mask=1111, last
B.SUBVIEW <SrcSelect>, <zero-or-compiler-gpr>, <OffsetUnits>, <SubviewSizeCode>
B.IOR [<dst>,<stride>], []
```

## Compile-time rejection

The wrapper rejects at template-instantiation time:

```cpp
range::Subview<Src, 0, 0>(s, 0);       // SubviewSizeCode 0 reserved
range::Subview<Src, 13, 0>(s, 0);      // 13..15 reserved
range::Subview<Src, 1, 2048>(s, 0);    // uimm11 > 2047
range::Subview<Src, 1, 0, 25>(s, 0);   // RegSrc outside 0..23

range::Assemble<Dst, 0, true>(d, 0);   // INIT=1 requires size 1..12
range::Assemble<Dst, 12, false>(d, 0); // non-INIT requires size 0
range::Assemble<Dst, 13, true>(d, 0);  // ParentSizeCode reserved
```

The same illegal words are rejected by the LLVM assembler (INIT/size
contradictions, reserved size codes, `RegSrc` out of range) and fail closed
in the disassembler (`<unknown>`).

## Shared Tile ranges

`B.SUBVIEW` may modify a Shared binder in the 0.58.5+ contract. A
`SharedTile` source passed to `range::subview` emits `B.IOS` followed
immediately by `B.SUBVIEW`; the range base is resolved independently for each
PE. `TPARTVIEW` remains Local-only until its multi-PE Shared array transport
is implemented. `B.ASSEMBLE` may still bind a Shared destination through
`B.IOS`.

```cpp
using Local = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using Shared = SharedTile<Local>;
using GM = global_tensor<float, RowMajor<4, 8>>;

// Shared destination emits B.IOS then B.ASSEMBLE.
auto assembled = range::assemble<128, 3>(dst, base_units);
TLOAD(assembled, gm);  // B.IOS ... / B.ASSEMBLE 1, 0, <compiler-gpr>, 3, 1
```

Shared sizes follow the Shared `B.IOS` contract (`128 B..256 KiB`,
SizeCode `1..12`) rather than the Local `B.IOT` `1..10` range.

## Lineage and status

- **Implemented**: Local Matrix+CUBE source `Subview` and Local
  destination `Assemble` over `TLOAD`, with zero-base and compiler-allocated
  runtime-base paths. LLVM MC round-trips every legal combination and rejects the
  illegal matrices (`v5-subview-assemble{-neg,-encoding}.s`).
- **Implemented**: Shared source `Subview` through `B.IOS` and Shared
  destination `Assemble` through `B.IOS`. RowMajor and Vec+CUBE source
  `Subview` forms remain compile-negative tests.
- Range modifiers do not change the PE-mask contract, the Tile size code,
  or the logical shape of the bound operand.
