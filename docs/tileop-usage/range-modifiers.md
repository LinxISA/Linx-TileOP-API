# Tile range modifiers (B.SUBVIEW / B.ASSEMBLE)

PTO-ISA 0.58.4 (ADR-0098) adds two *range modifiers* that attach to the
immediately preceding `B.IOT`/`B.IOS` binder in the same contiguous
block-command group and describe a byte range against a base GPR:

```asm
B.SUBVIEW SrcSelect, RegSrc, uimm11, SubviewSizeCode    ; source side
B.ASSEMBLE INIT, LAST, RegSrc, uimm11, ParentSizeCode   ; destination side
```

The derived offset is `GPR[RegSrc] + ZeroExtend(uimm11) mod XLEN`, where
`RegSrc` is an absolute GPR selector `0..23` and `uimm11` is `0..2047`.

The API models these descriptors as compile-time *range carriers*:
`pto::range::Subview` for a source operand and `pto::range::Assemble` for a
destination operand. They forward the wrapped tile's shape/dtype/storage
unchanged (they never create a second Tile register namespace), and the
consuming operation emits the corresponding modifier line right after the
binder.

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

## `range::Subview` — source-side range carrier

```cpp
template <typename Parent, unsigned SubviewSizeCode, unsigned Offset = 0,
          unsigned RegSrc = 2>
class Subview;
```

- `SubviewSizeCode` must be `1..12`.
- `Offset` (the `uimm11` adder) is a compile-time constant `0..2047`.
- `RegSrc` is the absolute GPR selector `0..23` (`a0`/R2 by default), with a
  **base address supplied at construction** — see below.

```cpp
using Src = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<4, 8>>;

Src s;
GM gm;
auto sv = range::Subview<Src, 1, /*Off*/ 0, /*RegSrc*/ 0>(s, 0);
TSTORE(gm, sv);  // emits B.SUBVIEW 0, r0, 0, 1 after the source B.IOT
```

### RegSrc selects the base register

The wrapper template parameter `RegSrc` fixes **which** GPR the descriptor
uses. The construction argument is the runtime base-address value that must
sit in that GPR:

```cpp
range::Subview<Src, 12, /*Off*/ 2047, /*RegSrc*/ 23> sv(s, base_addr);
TSTORE(gm, sv);  // B.SUBVIEW 0, r23, 2047, 12, base_addr in r23
```

The consuming operation binds the base value into the selected GPR with a
local-register inline-asm slot, then emits the canonical line. `RegSrc` is
statically enumerable `0..23`; each value expands to its own `rN` asm form.

## `range::Assemble` — destination-side range carrier

```cpp
template <typename Parent, unsigned ParentSizeCode, bool INIT = true,
          bool LAST = false, unsigned Offset = 0, unsigned RegSrc = 2>
class Assemble;
```

- `ParentSizeCode` `0..12`, with the INIT contract above.
- `INIT`/`LAST` select the form (`INIT`, `INIT_LAST`, `MIDDLE`, `LAST`).
- `Offset` `0..2047`; `RegSrc` `0..23` (default `a0`/R2), base value passed
  to the constructor.

```cpp
using Dst = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;

Dst d;
auto as = range::Assemble<Dst, 12, /*INIT*/ true, /*LAST*/ false,
                          /*Off*/ 0, /*RegSrc*/ 0>(d, base_addr);
TLOAD(as, gm);  // B.IOT ..., last, ->d<...> / B.ASSEMBLE 1, 0, r0, 0, 12
```

Non-INIT forms use `ParentSizeCode=0`:

```cpp
range::Assemble<Dst, 0, /*INIT*/ false, /*LAST*/ true,
                /*Off*/ 2047, /*RegSrc*/ 2> as(d, base);
TLOAD(as, gm);  // B.ASSEMBLE 0, 1, r2, 2047, 0
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
B.ASSEMBLE <INIT>, <LAST>, rN, <off>, <parent-size>
B.IOR [<src>,<stride>], []
```

A `TSTORE` on a `Subview`-wrapped source:

```asm
BSTART.TLSU TSTORE, F32
B.DIM <vcol>, 0, ->lb0
B.DIM <vrow>, 0, ->lb1
B.DIM zero, <cols>, ->lb2
B.IOT <src>, mask=1111, last
B.SUBVIEW <SrcSelect>, rN, <off>, <subview-size>
B.IOR [<dst>,<stride>], []
```

## Compile-time rejection

The wrapper rejects at template-instantiation time:

```cpp
range::Subview<Src, 0, 0>(s, 0);       // SubviewSizeCode 0 reserved
range::Subview<Src, 13, 0>(s, 0);      // 13..15 reserved
range::Subview<Src, 1, 2048>(s, 0);    // uimm11 > 2047
range::Subview<Src, 1, 0, 24>(s, 0);   // RegSrc outside 0..23

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
range::Subview<Shared, 12, 2047, 23> view(src, 23);
TSTORE(gm, view);  // B.IOS ... / B.SUBVIEW 0, r23, 2047, 12

// Shared destination emits B.IOS then B.ASSEMBLE.
range::Assemble<Shared, 12, true, false, 0, 0> assembled(dst, 0);
TLOAD(assembled, gm);  // B.IOS mask=1111, ->dst<tsize> / B.ASSEMBLE 1, 0, r0, 0, 12
```

Shared sizes follow the Shared `B.IOS` contract (`128 B..256 KiB`,
SizeCode `1..12`) rather than the Local `B.IOT` `1..10` range.

## Lineage and status

- **Implemented**: Local source `Subview` over `TSTORE` and Local
  destination `Assemble` over `TLOAD`, with the RegSrc base-value binding
  above. LLVM MC round-trips every legal combination and rejects the
  illegal matrices (`v5-subview-assemble{-neg,-encoding}.s`).
- **Implemented**: `SharedTile` range carriers through the Shared `B.IOS`
  binder (`TLOAD`/`TSTORE` Shared overloads with an `Assemble`/`Subview`
  operand), covered by `SharedRange.cpp` and the `RangeNegatives.cpp`
  role-mismatch cases (`Subview` on a TLOAD destination and `Assemble` on a
  TSTORE source are rejected).
- Range modifiers do not change the PE-mask contract, the Tile size code,
  or the logical shape of the bound operand.

## 使用要求

`range::Subview` 只能作为 source-side range carrier，`range::Assemble` 只能作为 destination-side range carrier，并且必须传给支持该 carrier 的 `TLOAD`/`TSTORE` 重载。范围参数在编译期指定，base address 在构造时提供。

## 默认值

`Subview` 的 `Offset` 默认为 `0`、`RegSrc` 默认为 `2`；`Assemble` 的 `INIT` 默认为 `true`、`LAST` 默认为 `false`、`Offset` 默认为 `0`、`RegSrc` 默认为 `2`。省略参数使用这些模板默认值，不等同于任意显式编码的零值。

## 异常和边界行为

`SubviewSizeCode` 必须为 `1..12`，`ParentSizeCode` 必须符合 INIT 规则，`Offset` 必须为 `0..2047`，`RegSrc` 必须为 `0..23`。越界值、role mismatch、错误 location、非法 PE mask 或不连续的 binder group 会在模板实例化、汇编或执行前被拒绝。范围 modifier 不改变逻辑 shape、Tile size code 或 PE-mask 语义。

## 完整语义

范围修饰符的完整编码、约束和边界行为请参阅 [PTO-SPEC v0.58.4.1 tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。