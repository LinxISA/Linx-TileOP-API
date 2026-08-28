# TileOP API usage

This directory documents the active LinxISA / PTO ISA 0.58.3 surface implemented by
`include/jcore/template_asm.hpp` and the sibling `include/jcore/*.hpp`
kernel wrappers. The operation list, engine classes, and function values are
projected from the pinned LinxISA release recorded in
[`contracts/linxisa-v0.58-engine-ops.json`](../../contracts/linxisa-v0.58-engine-ops.json),
which mirrors the PTO ISA 0.58.3 tile-operation catalog
(`spec/catalog/tile-operations.json`).

| Area | Reference |
| --- | --- |
| VEC / SFU / TLSU / CUBE operation index | [engines.md](engines.md) |
| PTO ISA 0.58.3 migration and compatibility boundary | [pto-0583-migration.md](pto-0583-migration.md) |
| Tile load, store, move, gather, scatter, and peer move | [tlsu.md](tlsu.md) |
| Matrix operations | [cube.md](cube.md) |
| Tile size, valid region, Shared register, and PE mask rules | [constraints.md](constraints.md) |
| Layout helpers | [layout.md](layout.md) |
| Matrix PostProcess (B.FPATR, quant/PReLU/RowMax/GroupMax) | [matrix-postprocess.md](matrix-postprocess.md) |
| Comparison operations (`CmpMode`, `TCMP`, `TCMPS`) | [cmp.md](cmp.md) |
| Sorting operations (`TSORT`, `TMRGSORT`) | [sort.md](sort.md) |
| Contiguous integer sequence generation (`TCI`) | [tci.md](tci.md) |
| Quantization and image-to-column (`TQUANT`, `TDEQUANT`, `TIMG2COL`) | [quant-and-im2col.md](quant-and-im2col.md) |
| Tile datatype reinterpret view | [reinterpret-tile.md](reinterpret-tile.md) |
| B.SUBVIEW / B.ASSEMBLE range modifiers | [range-modifiers.md](range-modifiers.md) |
| Experimental compiler-semantic aligned Tile arrays | [tile-arrays.md](tile-arrays.md) |

The generated engine index and checked-in machine contract are the navigation roots. New public
operations must first exist in LinxISA, then be projected into `contracts/`, implemented in the
header, and covered by `make check`.
