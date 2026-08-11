# TileOP API usage

This directory documents the active LinxISA v0.58 surface implemented by
`include/jcore/template_asm.hpp`.

| Area | Reference |
| --- | --- |
| VEC / SFU / TLSU / CUBE operation index | [engines.md](engines.md) |
| Tile load, store, move, gather, scatter, and peer move | [tlsu.md](tlsu.md) |
| Matrix operations | [cube.md](cube.md) |
| Tile size, valid region, Shared register, and PE mask rules | [constraints.md](constraints.md) |
| Layout helpers | [layout.md](layout.md) |
| Fixed-point matrix wrappers | [tmatmul-fixp.md](tmatmul-fixp.md) |

The generated engine index and checked-in machine contract are the navigation roots. New public
operations must first exist in LinxISA, then be projected into `contracts/`, implemented in the
header, and covered by `make check`.
