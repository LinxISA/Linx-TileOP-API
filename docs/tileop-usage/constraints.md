# Tile and Shared-register constraints

## Per-PE tile size

`SizeCode` describes the storage allocated for one participating PE.

| SizeCode | Bytes per PE | Destination domain |
| ---: | ---: | --- |
| 1 | 128 B | Local / Shared |
| 2 | 256 B | Local / Shared |
| 3 | 512 B | Local / Shared |
| 4 | 1 KiB | Local / Shared |
| 5 | 2 KiB | Local / Shared |
| 6 | 4 KiB | Local / Shared |
| 7 | 8 KiB | Local / Shared |
| 8 | 16 KiB | Local / Shared |
| 9 | 32 KiB | Local / Shared |
| 10 | 64 KiB | Local / Shared |
| 11 | 128 KiB | Shared only |
| 12 | 256 KiB | Shared only |

The physical allocation is `popcount(PE_MASK) * bytes_per_PE`. `PE_MASK=0000`
is a strict no-op: no allocation, no rename, no source read, no memory side
effect, no binder consumption, and no fault. The valid region is protected independently and must satisfy
`valid_rows <= rows` and `valid_cols <= cols`. Matrix dimensions M, N, and K
are arbitrary positive values independent of SizeCode; CUBE CELL geometry and
capacity rounding do not redefine them.

## Local and Shared tile registers

`B.IOT` binds Local tile sources and destinations, and carries the `last-use`
bit and the destination `SizeCode`/`DstTile` metadata. `B.IOS` (the v0.58 reissue
32-bit form) binds an absolute core-private Shared register `S0` through
`S255` with a per-PE size code and a four-PE participation mask:

```asm
B.IOS S7, mask=1111
B.IOS mask=1100, ->S9<4>
```

For `B.IOS`, `SizeCode=0` denotes a Shared **source** and `SizeCode=1..12`
denotes a Shared **destination** (per-PE capacity, 128 B..256 KB per PE;
B.IOT Local destinations use 1..10 = 128 B..64 KB per PE). The core allocation is
`popcount(PE_MASK) * per_pe_size`. The first non-zero allocation write records
an immutable allocation mask; reading an uninitialized lane returns an
undefined value without trapping.

PTO ISA 0.58.3 replaces arbitrary masks with a fixed three-bit PEMode decoder.
Only `0000`, `1000`, `0100`, `0010`, `0001`, `1100`, `1110`, and `1111` are
representable. TileOP keeps the readable mask spelling but rejects all other
template values at compile time.

## Matrix operands

Operations with several same-shaped operands require callers to construct
them with matching runtime valid dimensions. Matrix operations derive `M`, `N`,
and `K` from their Left/Right operands. Local A/C/D use `CubeM16` or
`CubeM32`, Local B uses `CubeN8`, and matching A/C/D layouts are mandatory.
`SharedTile` preserves these runtime
values when produced by `TMOV_L2S_INSERT` or `TMOV_L2S_PUBLISH`, so Shared
Matmul uses the same dynamic `B.DIM` path as Local Matmul.

Both Shared-producing TMOV operations support return-value and
output-parameter forms:

```cpp
auto shared = TMOV_L2S_PUBLISH(local);

SharedTile<LocalTile> shared;
TMOV_L2S_PUBLISH(shared, local);
```

The output-parameter form copies the Local Tile's runtime valid dimensions
into the `SharedTile` metadata. The Shared handle must still remain inside an
inlined SSA flow; a non-inlined function that writes a caller-owned
`SharedTile&` would require a separate Shared register ABI.
