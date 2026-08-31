# TCVT

`TCVT(dst, src)` converts every valid element of `src` to the destination element type described by `dst`. The C++ Tile types determine the source data type, destination data type, layout, valid shape, physical storage shape, and destination Tile capacity.

## C++ interface

```cpp
template <is_tile_data_v DstTile, is_tile_data_v SrcTile>
void TCVT(DstTile &dst, SrcTile &src);
```

Developers provide typed source and destination Tiles. No ISA encoding value, Tile register number, or TSize code is passed manually.

## Ordinary layouts

For ordinary layouts such as `RowMajor` and `ColMajor`, source and destination must have:

- identical physical `Rows` and `Cols`;
- identical `ValidRow` and `ValidCol`;
- a valid region contained by each physical Tile;
- a legal source/destination dtype combination and Tile location;
- enough destination capacity for the converted result.

Example:

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using Src = Tile<Location::Vec, float, 2, 1024, BLayout::RowMajor>;
using Dst = Tile<Location::Vec, __half, 2, 1024, BLayout::RowMajor>;

void convert(Dst &dst, Src &src) {
  TCVT(dst, src);
}
```

## CUBE_M16 and CUBE_M32 conversion

PTO-SPEC ADR-0110 closes the conversion rules for `CUBE_M16` and `CUBE_M32`. These layouts differ from ordinary TCVT in one important way: changing dtype can change the number of elements stored by each 128-byte CELL. Therefore, source and destination physical columns and TSize are derived independently from their own dtype.

The developer must preserve:

- the same CUBE layout (`CUBE_M16` to `CUBE_M16`, or `CUBE_M32` to `CUBE_M32`);
- the same logical valid rows and valid columns;
- a Matrix Tile location;
- Local Tile capacities in the ISA range `128 B` through `64 KiB`.

The physical `Rows`, physical `Cols`, CELL count, required bytes, and `TilesizeCode` do **not** need to match between source and destination. `CubeTileM16` and `CubeTileM32` calculate them from the element type and declared shape.

Example: the FP16 source uses `512 B`, while the FP32 destination needs `1 KiB`. Both represent the same valid `16 x 9` matrix.

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using Src = CubeTileM16<__half, 16, 12, 16, 9>;
using Dst = CubeTileM16<float, 16, 10, 16, 9>;

static_assert(Src::ValidRow == Dst::ValidRow);
static_assert(Src::ValidCol == Dst::ValidCol);
static_assert(Src::TilesizeCode == __tilesize_512B);
static_assert(Dst::TilesizeCode == __tilesize_1KB);

void convert_cube(Dst &dst, Src &src) {
  TCVT(dst, src);
}
```

`CUBE_N8` conversion is not included in ADR-0110 and is rejected by the current API.

## Mapping to the ISA bundle

For an ordinary Tile, TileOP emits all three logical-dimension bindings:

```asm
BSTART.TEPL TCVT, SrcDataType
B.DATR DstDataType, RNONE
B.DIM rValidCol, 0, ->lb0
B.DIM rValidRow, 0, ->lb1
B.DIM zero, PhysicalCol, ->lb2
B.IOT SrcTile, mask=1111, last, ->DstTile<DstTSize>
```

For `CUBE_M16` and `CUBE_M32`, ADR-0110 defines:

- `LB0 = source ValidCol`;
- `LB1 = source ValidRow`;
- `LB2` is omitted;
- the `B.IOT` destination TSize comes from the destination Tile type.

```asm
BSTART.TEPL TCVT, SrcDataType
B.DATR DstDataType, RNONE
B.DIM rValidCol, 0, ->lb0
B.DIM rValidRow, 0, ->lb1
B.IOT SrcTile, mask=1111, last, ->DstTile<DstTSize>
```

The current implementation uses extended inline assembly. An intrinsic is not required because the typed C++ operands already provide every field required by this bundle: source and destination Tile registers, dtype selectors, valid dimensions, layout checks, and destination TSize.

## Compile-time diagnostics

TileOP rejects calls when it can prove that:

- a CUBE conversion changes between `CUBE_M16` and `CUBE_M32`;
- source and destination valid shapes differ;
- a CUBE operand is not in a Matrix Tile location;
- source or destination Local TSize is outside `128 B..64 KiB`;
- the destination is a CUBE layout but the source is not `CUBE_M16` or `CUBE_M32`;
- either operand uses unsupported `CUBE_N8` conversion;
- an ordinary conversion changes physical `Rows` or `Cols`.

Other dtype legality, aliasing, numerical behavior, saturation, rounding, padding, and exception behavior follow the PTO-SPEC TCVT contract.

## Specification

See the PTO-SPEC `TCVT` operation and ADR-0110, introduced by PTO-SPEC issue #167 / PR #176.
