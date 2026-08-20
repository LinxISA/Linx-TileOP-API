# TileOP API usage

This directory documents the active LinxISA v0.58 surface implemented by
`include/jcore/template_asm.hpp` and the sibling `include/jcore/*.hpp`
kernel wrappers. The operation list, engine classes, and function values are
projected from the pinned LinxISA release recorded in
[`contracts/linxisa-v0.58-engine-ops.json`](../../contracts/linxisa-v0.58-engine-ops.json),
which mirrors the PTO-ISA v0.58 tile-operation catalog
(`spec/catalog/tile-operations.json`).

| Area | Reference |
| --- | --- |
| VEC / SFU / TLSU / CUBE operation index | [engines.md](engines.md) |
| Tile load, store, move, gather, scatter, and peer move | [tlsu.md](tlsu.md) |
| Matrix operations | [cube.md](cube.md) |
| Tile size, valid region, Shared register, and PE mask rules | [constraints.md](constraints.md) |
| Layout helpers | [layout.md](layout.md) |
| Matrix PostProcess (B.FPATR, quant/PReLU/RowMax/GroupMax) | [matrix-postprocess.md](matrix-postprocess.md) |
| Comparison operations (`CmpMode`, `TCMP`, `TCMPS`) | [cmp.md](cmp.md) |
| Sorting operations (`TSORT`, `TMRGSORT`) | [sort.md](sort.md) |
| Quantization and image-to-column (`TQUANT`, `TDEQUANT`, `TIMG2COL`) | [quant-and-im2col.md](quant-and-im2col.md) |
| Tile datatype reinterpret view | [reinterpret-tile.md](reinterpret-tile.md) |

The generated engine index and checked-in machine contract are the navigation roots. New public
operations must first exist in LinxISA, then be projected into `contracts/`, implemented in the
header, and covered by `make check`.
