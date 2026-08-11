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

Each core owns one Shared register file visible to its four PEs. A Shared destination allocates a
new physical register through compiler allocation and hardware renaming. A Shared source does not
modify the descriptor. Shared read-modify-write operations are atomic, but the architecture does
not impose ordering between PEs. Programs must avoid conflicting offsets. Reading an uninitialized
Shared register has the same undefined-value behavior as reading an undefined scalar register.

## Engine classes

- **VEC**: elementwise operations only.
- **SFU**: reductions, broadcasts, transforms, sorting, and other complex-hardware operations.
- **TLSU**: tile movement and memory access.
- **CUBE**: matrix operations.

The encoding carrier behind VEC and SFU is unchanged; public assembly uses the semantic aliases.
