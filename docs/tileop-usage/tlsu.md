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

## Load and store stride

`TLOAD` and `TSTORE` carry the global-memory base and row stride through
`B.IOR`. An omitted scalar input uses the operation's dense-row default
(straight to the physical columns). An explicitly encoded zero register means
zero stride; it is not the omitted form. The C++ wrapper passes the row stride
in **logical elements** (not bytes) exactly as required by the v0.58
architecture contract; address scaling by element size is architectural.
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
B.IOR [a3,a4], []       ; [base, logical-row-stride]
```

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
row stride are scalar inputs. Destination/source tile size remains the per-PE
128 B..8 KB `TSize` domain.

## Peer movement

`GMOV` moves a Tile payload between PEs of the same core. All four PEs must
reach the same dynamic instance; `PEMask` only selects the requesters and does
not reduce the Core4 collective. The peer TID is carried by `B.IOR`.

```cpp
GMOV<15>(dst, peer_tid, src);
```

## Shared naming note

The v0.58 reissue renamed the old DCU/matrix-transfer gather/scatter family to the
`MGATHER*` / `MSCATTER*` / `GMOV` surface above; no retired prefix remains in
the active catalog. `MatrixPadValue` appears only as a host-side C++ enum in this
header and is not emitted as assembly.
