# Tile and Shared-register constraints

## Per-PE tile size

`TSize` describes the storage allocated for one participating PE.

| TSize | Bytes per PE |
| ---: | ---: |
| 1 | 128 B |
| 2 | 256 B |
| 3 | 512 B |
| 4 | 1 KiB |
| 5 | 2 KiB |
| 6 | 4 KiB |
| 7 | 8 KiB |

The physical allocation is `popcount(PE_MASK) * bytes_per_PE`. `PE_MASK=0000` is a strict no-op.
Rows and columns are powers of two. The valid region is protected independently and must satisfy
`valid_rows <= rows` and `valid_cols <= cols`. Matrix dimensions M, N, and K are powers of two.

## Local and Shared tile registers

`B.IOT` binds Local tile sources and destinations. `B.IOS` binds an absolute core-private Shared
register `S0` through `S255`:

```asm
B.IOS S7, mask=1111
B.IOS mask=0011, ->S9<4>
```

Operations with several same-shaped operands require callers to construct them
with matching runtime valid dimensions. Matrix operations derive `M`, `N`, and
`K` from their Left/Right operands. `SharedTile` preserves these runtime values
when produced by `TMOV_L2S_INSERT` or `TMOV_L2S_PUBLISH`, so Shared Matmul uses
the same dynamic `B.DIM` path as Local Matmul.

Both Shared-producing TMOV operations support return-value and output-parameter
forms:

```cpp
auto shared = TMOV_L2S_PUBLISH(local);

SharedTile<LocalTile> shared;
TMOV_L2S_PUBLISH(shared, local);
```

The output-parameter form copies the Local Tile's runtime valid dimensions into
the `SharedTile` metadata. The Shared handle must still remain inside an inlined
SSA flow; a non-inlined function that writes a caller-owned `SharedTile&` would
require a separate Shared register ABI.
