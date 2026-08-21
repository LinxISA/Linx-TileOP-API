# Contiguous integer sequence generation

`TCI` creates a one-row Local Tile containing a contiguous integer sequence.
The sequence may increase or decrease by one and wraps modulo the destination
element width.

## Interface

```cpp
template <is_tile_data_v Tile, typename T, int descending = 0>
void TCI(Tile &dst, T start);
```

The template direction is:

- `descending=0`: element at logical column `k` is `start + k`.
- `descending=1`: element at logical column `k` is `start - k`.

The default form generates an ascending sequence:

```cpp
TCI(dst, start);
```

The explicit template form remains available when selecting the direction:

```cpp
TCI<TileType, ElementType, 0>(dst, start);  // ascending
TCI<TileType, ElementType, 1>(dst, start);  // descending
```

## Operand contract

- `dst` must be an ordinary Local VEC Tile.
- The Tile must use unboxed `BLayout::RowMajor` layout.
- The element type must be exactly `int32_t`, `int16_t`, `uint32_t`, or
  `uint16_t`, corresponding to PTO `S32`, `S16`, `U32`, and `U16`.
- `start` must have exactly the same C++ type as the destination element.
- `ValidRow` must be exactly one.
- `ValidCol` must be a positive compile-time value and must not exceed `Cols`.
- `descending` must be the compile-time value zero or one.

Invalid dtype, layout, valid shape, scalar type, or direction combinations are
rejected by `static_assert`.

Only the one-row valid region receives sequence values. Arithmetic wraps modulo
the selected element width. For example, ascending S16 from `32767` produces
`32767, -32768, -32767, ...`; descending U16 from zero produces
`0, 65535, 65534, ...`. Physical coordinates outside the valid region are PTO
Null padding.

## Examples

```cpp
using S16Sequence =
    Tile<Location::Vec, int16_t, 1, 128, BLayout::RowMajor, 1, 64>;
using U32Sequence =
    Tile<Location::Vec, uint32_t, 1, 64, BLayout::RowMajor, 1, 32>;

S16Sequence ascending;
U32Sequence descending;

TCI(ascending, static_cast<int16_t>(64));
TCI<U32Sequence, uint32_t, 1>(descending, static_cast<uint32_t>(64));
```

The physical column count may be larger than `ValidCol`. In the example above,
only the first 64 S16 elements and first 32 U32 elements belong to the valid
sequence; the remaining physical columns are Null padding.

## ISA bundle

TCI is PTO v0.58.1 TEPL Mode 3, Function 6, selector `0x066`. Canonical PTO
assembly describes it as `BSTART.SFU TCI`; the current LinxV5 backend uses the
equivalent selector carrier `BSTART.TEPL 102`.

TileOP emits:

```asm
BSTART.TEPL 102, S32|S16|U32|U16
B.DIM ValidCol, 0, ->lb0
B.DIM zero, Col, ->lb2
B.IOR [Start,Direction],[]
B.IOT mask=1111, last, ->DstTile<SizeCode>
```

`LB1` is omitted and therefore defaults to one. `LB2` explicitly preserves the
Tile's physical `Cols`; PTO permits it to be omitted only when `Col` equals
`ValidCol`. `B.DATR` is omitted because TCI permits only its all-zero form and
always applies Null physical padding.

`B.IOR` uses RegSrc0 for the raw start value and RegSrc1 for direction. Bits
above the selected element width are ignored. RegSrc2 and RegDst remain zero.
There are no source Tile bindings, Shared bindings, or memory effects.

## Test coverage

`test/tileop_api/src/TCI.cpp` instantiates S16, U16, S32, and U32 forms and
covers both ascending and descending directions. The API's `static_assert`
contract rejects unsupported dtypes, non-RowMajor or multi-valid-row Tiles,
mismatched start types, and direction values other than zero or one.
