# Tile datatype reinterpret

`reinterpret_tile<NewDType>(src)` creates a zero-instruction view of an existing
Local Tile with a different compile-time datatype. It does not convert values
and does not copy the Tile payload.

```cpp
using F32Tile = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

F32Tile src;
auto bits_as_s32 = reinterpret_tile<int32_t>(src);
```

The view keeps the source Tile's:

- underlying Tile register/storage carrier;
- Local location and Left/Right/Acc role;
- physical storage size and TileSizeCode;
- Rows, Cols, ValidRow, and ValidCol;
- layout, fractal, padding, and compactness attributes.

Only the static `DType` changes. When a later Tile operation consumes the view,
it obtains the PTO datatype encoding from `type_traits<NewDType>::TypeCode`.

## Reinterpret versus conversion

`reinterpret_tile` preserves the bits. It must not be used when the numerical
value needs to be converted:

```cpp
auto bits_as_s32 = reinterpret_tile<int32_t>(src); // bits unchanged

Tile<Location::Vec, int32_t, 16, 16, BLayout::RowMajor> converted;
TCVT(converted, src);                               // numerical conversion
```

`TCVT` may change representation through rounding, saturation, or conversion
rules and emits a hardware conversion operation. `reinterpret_tile` emits no
`TCVT`, copy, memory transfer, or other instruction by itself.

## Current restrictions

The first implementation supports only:

- Local Tile sources;
- source and destination datatypes with the same bit width;
- target datatypes registered in the PTO type system;
- layouts that remain valid without changing shape or packing.

The following are rejected at compile time:

```cpp
reinterpret_tile<__half>(fp32_tile); // different bit width
reinterpret_tile<SomeUnregisteredType>(tile); // no PTO TypeCode
reinterpret_tile<int32_t>(shared_tile); // Shared view is not enabled yet
```

Different-width reinterpretation would change the logical element interpretation
and may change shape, packing, or TileSizeCode. It requires a separate API with
an explicit new shape and is not implicit in this interface. Shared Tile
reinterpretation is also a separate follow-up because it must preserve the
Shared handle and `Sr`/`B.IOS` binding contract.

## Using the view

The view is intended to be consumed by existing Tile APIs without creating a
new Tile object:

```cpp
TileLeft<float, 32, 64> left;
TileRight<float, 64, 32> right;
Tile<Location::Vec, int32_t, 32, 32, BLayout::RowMajor> output;

auto left_s32 = reinterpret_tile<int32_t>(left);
auto right_s32 = reinterpret_tile<int32_t>(right);
TMATMUL(output, left_s32, right_s32);
```

The consuming operation must still accept the view's role and layout. The
reinterpret view itself is non-owning and holds a reference to the source Tile;
keep the source Tile alive for the entire lifetime of the view.

## Verification expectations

A conforming implementation should show that:

- `DType` changes to `NewDType`;
- the storage carrier and TileSizeCode remain unchanged;
- invalid width or unregistered-dtype uses fail during template instantiation;
- the generated consumer uses the new datatype encoding;
- no `TCVT`, Tile copy, memory copy, or GPR copy is emitted;
- Shared Tile sources remain rejected until the Shared view design is implemented.
