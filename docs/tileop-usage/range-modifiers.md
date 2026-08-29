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

The API exposes a small view-building layer, inspired by block-pointer APIs:
create a range view once and pass it to the consuming operation. The ordinary
surface is `pto::range::subview(parent, base)` and
`pto::range::assemble(parent, base)`. The lower-level `Subview` and `Assemble`
carrier types remain available for unusual compile-time contracts.

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
using TileT = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<4, 8>>;

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
auto sized = range::subview<128>(tile, base_units);   // 128 B, offset 0
auto shifted = range::subview<128, 3>(tile, base_units);
auto destination = range::assemble<128, 3>(tile, base_units);
```

`LengthBytes` must be one of `128`, `256`, `512`, `1*1024`, ..., `256*1024`, and
must not exceed `tile::LogicalTileBytes`. The factory converts it to the ISA
`SubviewSizeCode` or INIT `ParentSizeCode` automatically and rejects invalid or
oversized values at compile time.
The low-level `subview_sized_at` helper remains available for direct ISA contract tests.

The explicit carrier forms documented below remain supported for code that
needs every descriptor field visible in the type.

For a developer-oriented guide with complete lifecycle, Local/Shared, validation,
and generated-assembly examples, see [B.SUBVIEW / B.ASSEMBLE Developer Guide](range-modifiers-developer-guide.md).

## `range::Subview` — source-side range carrier

```cpp
template <typename Parent, unsigned SubviewSizeCode, unsigned OffsetUnits = 0,
          unsigned RegSrc = 2>
class Subview;
```

`Subview` 是底层 carrier 类型。普通 kernel 应优先使用统一的
`range::subview<LengthBytes = tile capacity, OffsetUnits = 0>(tile [, base_units])`
factory；调用者填写字节长度而不是 ISA 编码。`RegSrc` 仅用于固定 ABI 或编码测试，
运行时 base 的高层接口不会暴露寄存器编号。

- `SubviewSizeCode` must be `1..12`; the high-level factory derives it from
  the requested byte length.
- `OffsetUnits` (the `uimm11` 128-byte-unit adder) is a compile-time constant `0..2047`.
- `RegSrc` is the low-level absolute GPR selector `0..23`; high-level factories
  use `zero` or compiler allocation and do not expose this field.

```cpp
using Src = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<4, 8>>;

Src s;
GM gm;
auto zero_based = range::subview(s);
TSTORE(gm, zero_based);  // B.SUBVIEW 0, zero, 0, 1

auto runtime_based = range::subview(s, base_units);
TSTORE(gm, runtime_based); // compiler selects the encoded GPR
```

### Base-register selection

The high-level API does not expose `RegSrc`:

```cpp
range::subview(s);                    // zero base
range::subview(s, base_units);         // compiler-allocated GPR
range::subview<128>(s);                 // 128 B, zero + offset 0
range::subview<128>(s, base_units);     // 128 B, runtime base + offset 0
range::subview<128, 3>(s, base_units);  // 128 B, runtime base + 384 B
```

Explicit register selection is retained only for fixed ABI and encoding tests:

```cpp
auto sv = range::subview_sized_at_reg<12, 2047, 23>(s, base_units);
TSTORE(gm, sv);  // B.SUBVIEW 0, r23, 2047, 12
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
- INIT helpers derive `ParentSizeCode` from `LengthBytes`; MIDDLE/LAST encode
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

Range carriers over a `SharedTile` parent bind through the Shared `B.IOS`
binder instead of `B.IOT`, and expose `handle()` / `handle_ref()` instead of
`data()`. The wrapper template arguments are identical; the base value is
again passed to the constructor.

```cpp
using Local = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using Shared = SharedTile<Local>;
using GM = global_tensor<float, RowMajor<4, 8>>;

// Shared source emits B.IOS then B.SUBVIEW.
auto view = range::subview<128, 3>(src, base_units);
TSTORE(gm, view);  // B.IOS ... / B.SUBVIEW 0, <compiler-gpr>, 3, 1

// Shared destination emits B.IOS then B.ASSEMBLE.
auto assembled = range::assemble<128, 3>(dst, base_units);
TLOAD(assembled, gm);  // B.IOS ... / B.ASSEMBLE 1, 0, <compiler-gpr>, 3, 1
```

Shared sizes follow the Shared `B.IOS` contract (`128 B..256 KiB`,
SizeCode `1..12`) rather than the Local `B.IOT` `1..10` range.

## Lineage and status

- **Implemented**: Local source `Subview` over `TSTORE` and Local
  destination `Assemble` over `TLOAD`, with zero-base and compiler-allocated
  runtime-base paths. LLVM MC round-trips every legal combination and rejects the
  illegal matrices (`v5-subview-assemble{-neg,-encoding}.s`).
- **Implemented**: `SharedTile` range carriers through the Shared `B.IOS`
  binder (`TLOAD`/`TSTORE` Shared overloads with an `Assemble`/`Subview`
  operand), covered by `SharedRange.cpp` and the `RangeNegatives.cpp`
  role-mismatch cases (`Subview` on a TLOAD destination and `Assemble` on a
  TSTORE source are rejected).
- Range modifiers do not change the PE-mask contract, the Tile size code,
  or the logical shape of the bound operand.
