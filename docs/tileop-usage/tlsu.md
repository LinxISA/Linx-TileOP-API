# TLSU operations

TLSU owns `TLOAD`, `TSTORE`, `TMOV`, `TPREFETCH`, `MGATHER`, `MSCATTER`, their
masked forms, `MGATHER_CAS`, and `GMOV`. The canonical operation list and
function values are generated in [engines.md](engines.md); the PTO-ISA v0.58
catalog assigns the TLSU function numbers `TLOAD=0 TSTORE=1 TMOV=2
TPREFETCH=3 MGATHER=4 MSCATTER=5 MGATHER_MASK=6 MSCATTER_MASK=7 MGATHER_CAS=8
GMOV=13` and reserves `9..12,14..31` (functions 9..12 carry the Shared TMOV
Local-to-Shared / Shared-to-Local semantics).

This header emits the block forms using the current assembler spelling;
PTO-ISA v0.58 canonicalizes them to named block starts (`BSTART.TLOAD`,
`BSTART.TSTORE`, `BSTART.TMOV`, `BSTART.MGATHER.*`, `BSTART.GMOV`).

## Prefetch

`TPREFETCH(src, valid_col, valid_row)` emits TLSU function 3 without a Tile or
Shared binding. `LB0` and `LB1` carry the requested valid shape, `LB2` carries
the physical row width, and `B.IOR` carries the global base plus logical row
stride. The current assembler accepts the numeric `BSTART.TLSU 3` spelling
and disassembles it as `BSTART.TLSU TPREFETCH`.

For a static RowMajor `global_tensor`, the physical row width comes from its
compile-time column count. For `RowMajor<Rows, DYNAMIC>`, the constructor's
runtime stride supplies both the physical row width and row stride, so no
`DYNAMIC` sentinel is emitted into `B.DIM` or `B.IOR`.

## Load and store stride

`TLOAD` and `TSTORE` carry the global-memory base and row stride through
`B.IOR`. An omitted scalar input uses the operation's dense-row default
(straight to the physical columns). An explicitly encoded zero register means
zero stride; it is not the omitted form. The C++ wrapper passes the row stride
as a row stride in **bytes** exactly as required by PTO ISA 0.58.3. The
`global_tensor` object continues to expose element strides to ordinary C++;
`GetStrideBytes()` performs the checked boundary conversion for TLOAD/TSTORE.
For dynamic `global_tensor` layouts, the wrapper reads the stride stored in
the tensor object with `GetStride`; it never passes the layout template's
`-1` dynamic sentinel to `B.IOR`.

Per the v0.58 contract the TLOAD/TSTORE block layout is:

```asm
BSTART.TLOAD F32
B.DIM a0, 0, ->lb0      ; LB0 = valid columns
B.DIM a1, 0, ->lb1      ; LB1 = valid rows
B.DIM zero, a2, ->lb2   ; LB2 = physical columns
B.IOT mask=1111, last, ->t0<3>
B.IOR [a3,a4], []       ; [base, byte-row-stride]
```

### Shared store (TSTORE / TSTORE.SPART)

Storing a **Shared** Tile to GM uses TLSU Function 1 (full, `PE_MASK=1111`)
or Function 14 `TSTORE.SPART` (explicit nonzero PE subset); both use exactly
one source `B.IOS` binder and no `B.IOT`.

```cpp
TSTORE(gm, SharedTile);                 // full: B.IOS mask=1111
TSTORE_PART<PEMask>(gm, SharedTile);    // TSTORE.SPART: any nonzero subset
```

- The GM element type must equal the Shared tile's dtype
  (`static_assert`), and the Local source must be RowMajor / non-boxed
  (NORM; no `B.DATR` Layout is emitted).
- Both wrappers are `always_inline` so the opaque Shared handle never
  crosses the ordinary C++ ABI.

## Local and Shared moves

Local operands use `B.IOT`. Shared operands use `B.IOS`; the PE mask on
`B.IOS` selects the participating quarters, and a Shared destination carries
its own per-PE `TSize`. The v0.58 Shared register file is `S0..S255`.

The v0.58 catalog distinguishes the Shared TMOV semantics as TLSU functions
9..12 (`TMOVLocalToShared` and `TMOVSharedToLocal`), exposed through the
`TMOV_L2S_INSERT`, `TMOV_L2S_PUBLISH`, `TMOV_S2L_BROADCAST`, and
`TMOV_S2L_EXTRACT` wrappers. Because the current standalone assembler spelling
does not distinguish these four source forms, each wrapper either emits its
named form or fails at compile time rather than silently emitting base `TMOV`;
object-level support for the four distinct encodings is tracked by LinxISA
issue 166 and LLVM issue 37.

## Gather and scatter

Gather/scatter offset and mask tiles are Local operands. The global base and
row stride are scalar inputs. Destination/source tile size uses the per-PE `SizeCode` domain
(B.IOT Local: 1..10 = 128 B..64 KB; B.IOS Shared: 1..12 = 128 B..256 KB).
The public `PEMask` template argument is accepted only for the fixed decoder
set `0000`, `1000`, `0100`, `0010`, `0001`, `1100`, `1110`, and `1111`;
operations requiring participation reject mode zero.

## CUBE CELL transport

`TLOAD_CUBE` and `TSTORE_CUBE` are the explicit GM transport boundary for
persistent Local CUBE storage. They select `ND2M32`, `ND2M16`, or `ND2N8` on
load and the inverse `M322ND`, `M162ND`, or `N82ND` on store. These forms
carry only LB0/LB1 logical dimensions, never LB2, and retain byte row stride
in `B.IOR.RegSrc1`. SizeCode is capacity; it is independent of logical M/N/K.

### MGATHER_CAS (atomic compare-and-swap)

`MGATHER_CAS` atomically compares-and-swaps GM elements at byte displacements
(TLSU function 8; canonical `BSTART.MGATHER.CAS`). Each lane reads
`base + displacement`, compares with `expected`, stores `replacement` on
match, and publishes the observed old value to `observedOld`.

```cpp
template <is_tile_data_v DstTile, is_tile_data_v IndexTile,
          is_tile_data_v ExpectedTile, is_tile_data_v ReplacementTile>
void MGATHER_CAS(DstTile &observedOld, uint64_t base,
                 IndexTile &byteDisplacements, ExpectedTile &expected,
                 ReplacementTile &replacement,
                 uint32_t validCol, uint32_t validRow = 1);
```

- `expected` / `replacement` / `observedOld` share one transfer DataType;
  `byteDisplacements` is an integer byte-displacement tile (S/U 8..64).
- All four tiles must match the resolved `ValidRow x ValidCol`.
- The bundle is exactly two `B.IOT`: IndexTile+ExpectedTile (TwoSrc_NoDst,
  no `last`) then ReplacementTile+`last` -> DstTile, with one common nonzero
  PE mask; `B.IOR` carries only the byte-address base; the destination is an
  early-clobbered output so it never aliases the replacement source.
- Duplicate-address lanes serialize in an implementation-defined order; all
  lane addresses are preflighted before the first atomic event.

Example:

```cpp
using D = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using Idx = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;

D old, expected, replacement;
Idx offsets;
MGATHER_CAS(old, /*base=*/0x1000, offsets, expected, replacement, 256, 2);
```

## Peer movement

`GMOV` moves a Tile payload between PEs of the same core. All four PEs must
reach the same dynamic instance; `PEMask` only selects the requesters and does
not reduce the Core4 collective. The peer TID is carried by `B.IOR`.

```cpp
GMOV<15>(dst, peer_tid, src);
```

## Shared / TMA naming note

The v0.58 reissue renamed the old DCU/TMA gather/scatter family to the
`MGATHER*` / `MSCATTER*` / `GMOV` surface above; no `TMA*` prefix remains in
the active catalog. `TmaPadValue` appears only as a host-side C++ enum in this
header and is not emitted as assembly.
