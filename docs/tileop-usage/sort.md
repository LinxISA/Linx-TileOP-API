# Sorting operations

`TSORT` implements the PTO ISA 0.58.3 stable row-group sort operation. It sorts
FP16 or FP32 values and produces both the reordered values and their original
zero-based column indices within each group.

## TSORT

```cpp
template <is_tile_data_v ValueDstTile, is_tile_data_v IndexDstTile,
          is_tile_data_v SourceTile>
void TSORT(ValueDstTile &valueDst, IndexDstTile &indexDst,
           SourceTile &source, uint32_t sortWidth = 32,
           bool descending = false);
```

- `source` and `valueDst` must have the same FP16 or FP32 dtype.
- `indexDst` must have U32 elements.
- All operands must be ordinary Local VEC Tiles using RowMajor layout.
- Both destinations must have the same physical `Rows`/`Cols` and compatible
  valid-row/valid-column shape as `source`.
- `sortWidth=0` and `sortWidth=32` select groups of 32 elements; values from
  1 through 64 select that exact group width. Values above 64 are illegal.
- `descending=false` selects ascending order; `true` selects descending order.
- Sorting is stable. Each index result is the source element's original
  zero-based column offset within its group.
- Numeric values precede NaNs in both directions. Equal values and NaNs retain
  their original source order.

Each valid row is divided from column zero into consecutive `sortWidth`
groups. A final short group contains only the remaining valid elements and
does not read physical padding.

Example:

```cpp
using Values = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using Indices = Tile<Location::Vec, uint32_t, 32, 32, BLayout::RowMajor>;

Values source, sortedValues;
Indices originalIndices;

TSORT(sortedValues, originalIndices, source);            // width 32, ascending
TSORT(sortedValues, originalIndices, source, 16, true);  // width 16, descending
```

## ISA bundle

TSORT is the SFU operation at TEPL Mode 3, Function 12, selector `0x06C`.
Canonical PTO assembly uses `BSTART.SFU TSORT`. Until the LinxV5 backend
accepts that canonical mnemonic, the TileOP implementation uses the equivalent
`BSTART.TEPL 108` encoding carrier.

The Local Tile binding stream contains one persistent source and two newly
allocated destinations. Only the final binding terminates the stream:

```asm
BSTART.TEPL 108, DataType
B.DIM sortWidth, 0, ->lb0
B.IOR [descending], []
B.IOT source, mask=1111, ->valueDst<ValueTSize>
B.IOT mask=1111, last, ->indexDst<IndexTSize>
```

The value and index destinations use their own logical Tile-size encodings.
For example, a 32x32 FP16 value destination needs 2KB, while its 32x32 U32
index destination needs 4KB.

LB1, LB2, B.IOS, nonzero B.DATR fields, mixed PE masks, unsupported dtypes,
and any additional Local Tile binding are illegal for TSORT.

## TMRGSORT

`TMRGSORT` merges two sorted single-row sources into one destination
(PTO ISA 0.58.3 TEPL Mode 3 Function 13 / selector 0x06D; canonical
`BSTART.SFU TMRGSORT`).

```cpp
template <is_tile_data_v DstTile, is_tile_data_v LeftTile,
          is_tile_data_v RightTile>
void TMRGSORT(DstTile &dst, LeftTile &left, RightTile &right,
              bool descending = false);
```

- `dst`, `left` and `right` must share one FP16 or FP32 dtype.
- The two sources are persistent sorted single-row Local tiles; the
  destination is a new Local tile.
- The bundle carries **no B.DIM**: only `B.IOR` RegSrc0 (0 ascending / 1
  descending, `descending` default `false`) and one TwoSrc_Dst `B.IOT` with
  `<last>` and the shared nonzero PE mask.
- `descending` uses the same volatile anti-fold as the other flag-carrying
  operations so the flag never lands on the zero register.

Example:

```cpp
using Row = Tile<Location::Vec, float, 1, 256, BLayout::RowMajor>;

Row a, b, out;
TMRGSORT(out, a, b);          // ascending merge
TMRGSORT(out, a, b, true);    // descending merge
```

## Deprecated interface

The historical single-output `TSORT32(dst, src)` interface does not represent
the PTO ISA 0.58.3 dual-output contract and now fails at template instantiation.
Use `TSORT(valueDst, indexDst, source, sortWidth, descending)` instead.

## Test coverage

`test/tileop_api/src/TSort.cpp` instantiates FP16 and FP32 forms with widths
1, 16, 32, and 64, including ascending and descending operation. The emitted
bundle must contain two `B.IOT` bindings with matching nonzero PE masks, and
only the second binding may carry `last`.
