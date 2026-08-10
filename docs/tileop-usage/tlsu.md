# TLSU operations

TLSU owns `TLOAD`, `TSTORE`, `TMOV`, `TPREFETCH`, `MGATHER`, `MSCATTER`, their masked forms,
compare-and-swap gather, and `GMOV`. The canonical operation list and function values are generated
in [engines.md](engines.md).

## Load and store stride

`TLOAD` and `TSTORE` carry the global-memory base and row stride through `B.IOR`. An omitted scalar
input uses the operation's dense-row default. An explicitly encoded zero register means zero stride;
it is not the omitted form. The C++ wrapper passes the row stride in logical elements exactly as
required by the v0.58 architecture contract; address scaling by element size is architectural.

```asm
BSTART.TLOAD F32
B.DIM a0, 0, ->lb0
B.DIM a1, 0, ->lb1
B.DIM zero, a2, ->lb2
B.IOT mask=1111, last, ->t0<3>
B.IOR [a3,a4], []
```

## Local and Shared moves

Local operands use `B.IOT`. Shared operands use `B.IOS`; the PE mask on `B.IOS` selects the
participating quarters. A Shared destination carries its own `TSize` and allocates a new descriptor.

The released catalog assigns distinct encodings to `TMOV.L2S.INSERT`, `TMOV.L2S.PUBLISH`,
`TMOV.S2L.BROADCAST`, and `TMOV.S2L.EXTRACT`, but the current standalone assembler spelling does
not distinguish them. The four C++ wrappers therefore fail at compile time instead of silently
emitting base `TMOV`. Object-level support remains tracked by LinxISA issue 166 and LLVM issue 37.

## Gather and scatter

Gather/scatter offset and mask tiles are Local operands. The global base and row stride are scalar
inputs. Destination/source tile size remains the per-PE 128 B..8 KB `TSize` domain.
