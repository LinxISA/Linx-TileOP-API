# CUBE operations

CUBE owns the matrix-multiply and matrix-vector families. The canonical
operation list is generated in [engines.md](engines.md) and the v0.58 catalog
assigns the CUBE function numbers:

```text
TMATMUL=0  TMATMUL.BIAS=1  TMATMUL.ACC=2  TMATMULMX=4
TMATMULMX.BIAS=5  TMATMULMX.ACC=6
TGEMV=16  TGEMV.BIAS=17  TGEMV.ACC=18
TGEMVMX=20  TGEMVMX.BIAS=21  TGEMVMX.ACC=22
```

There is no separate post-processing operation: the `B.FPATR`
attribute on the matrix operation describes every post-processing option
(PreQuantMode, ReLU mode, group width, RowMax/GroupMax enable flags) and the
Shared-primary `TransA`/`TransB` controls.

```cpp
CubeTileM32<float, 32, 32> a;
CubeTileN8<float, 32, 32> b;
CubeAccumulatorM32<float, 32, 32> out;
TMATMUL(out, a, b);
```

## Dimensions

M, N, and K are powers of two; valid rows and columns may describe a smaller
active region but may not exceed the allocated rows and columns. For TMATMUL
the `B.DIM` roles are `LB0 = M` (result rows), `LB1 = N` (result columns),
and `LB2 = K` (the shared inner dimension). The K value is derived from the
two source descriptors' matching inner dimension; `LB2` is not a second
encoding of K. Fixed-point attributes and extra parameter tiles are carried by
the matching matrix operation; they do not introduce a separate conversion
instruction.

```asm
BSTART.CUBE TMATMUL F16
B.DATR F16, byte0, Null
B.FPATR PreQuant, Relu, GroupNCode, RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn, TransA, TransB
B.DIM M, 0, ->lb0
B.DIM N, 0, ->lb1
B.DIM K, 0, ->lb2
```

## Accumulate forms

`.ACC` variants read the explicit accumulator `C` as the first source and
write the result `D` as a distinct destination (read-old/write-new alias
behavior); there is no implicit single accumulator in v0.58.
The inline-asm destination is early-clobbered so the compiler must allocate a
different Local Tile index for D while C remains live. D, optional reductions,
and numeric status publish as one completed bundle.

## Persistent CELL layouts

Local matrix primaries use the 128-byte width-parametric CELL layouts exposed
as `CubeTileM16`, `CubeTileM32`, and `CubeTileN8`. `CubeRequiredBytes` derives
the required CELL count from dtype and valid dimensions; the encoded SizeCode
is the next legal power-of-two capacity and does not redefine M/N/K. Use
`TLOAD_CUBE`/`TSTORE_CUBE` for the explicit GM conversion boundary.

## Shared operands

For cooperative Shared matrix operands, `B.IOS Sx, mask=1111` contributes the
ordered Shared source while Local operands continue to use `B.IOT`. Shared
primaries are ordinary RowMajor rectangles and may be transposed while they
are materialized by selecting `fixp::keep_acc().transpose_a()` or
`.transpose_b()`. Transpose is illegal when the corresponding primary is
Local. The compiler allocates absolute Shared register names.
Use `SharedMatrixLeft` / `SharedMatrixRight` as the role-carrying RowMajor
source types before publishing them through `TMOV_L2S_*`.

## Matrix-vector family

`TGEMV` and its `*.BIAS/*.ACC/*.MX*` forms operate on a matrix and a vector.
Per the v0.58 contract the `B.DIM` roles are reversed relative to TMATMUL:
`LB0 = N`, `LB1 = M`, `LB2 = Col`. `TGEMV` rejects all Shared binders.

## Note on canonical block spellings

PTO-ISA v0.58 canonicalizes the CUBE operations to named block starts
(`BSTART.TMATMUL`, `BSTART.TMATMUL.BIAS`, `BSTART.TMATMULMX`, `BSTART.TGEMV`,
...). The historical DavinciOO `TMATMUL*.FIXP` spelling was an
implementation-local name and has been removed; post-processing is
carried by `B.FPATR`, so the canonical emission is `BSTART.CUBE TMATMUL` +
`B.FPATR`. See [matrix-postprocess.md](matrix-postprocess.md).
