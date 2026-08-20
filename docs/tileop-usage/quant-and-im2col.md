# Quantization and image-to-column

## RoundMode

`TQUANT` / `TDEQUANT` encode the rounding mode into the `B.DATR` `RMode`
field and the saturation flag into `Sat`. Both are compile-time attributes
(`B.DATR` fields are immediates), mirrored by the `RoundMode` enum:

```cpp
enum class RoundMode : uint8_t {
  RNONE = 0,  ///< no rounding (pass-through)
  RNE = 1,    ///< round to nearest even
  RTZ = 2,    ///< round toward zero
  RDN = 3,    ///< round down
  RUP = 4,    ///< round up
  RNA = 5,    ///< round to nearest, ties away
  RHB = 7,    ///< reciprocal-half bias
};
```

## TQUANT (FP32 -> S8/U8)

```cpp
template <RoundMode Mode = RoundMode::RNE, bool Saturate = false,
          is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TQUANT(tile_shape_out &dst, tile_shape_in &src,
            float multiplier = 1.0f, int32_t zeroPoint = 0);
```

- `src` must be FP32; `dst` must be S8 or U8 (the `B.DATR` DataType is
  derived from the destination type).
- `B.DATR` carries `<S8|U8>, <RMode>, <SAT|NOSAT>`.
- `multiplier` travels as its **raw FP32 bit pattern** in a GPR (never an
  FP->int conversion); `zeroPoint` travels in another; they are emitted by
  `B.IOR` (the spec default 1.0 / 0 is still emitted explicitly).
- The bundle ends with one OneSrc_Dst `B.IOT` carrying `<last>`.

Example:

```cpp
using F32 = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using S8  = Tile<Location::Vec, int8_t,  8, 256, BLayout::RowMajor>;

F32 src;
S8  dst;
TQUANT<RoundMode::RNE, /*saturate=*/true>(dst, src, 0.5f, 1);
TQUANT(dst, src);                 // RNE, no saturate, 1.0/0
```

## TDEQUANT (S8/U8 -> FP32)

```cpp
template <RoundMode Mode = RoundMode::RNE, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void TDEQUANT(tile_shape_out &dst, tile_shape_in &src,
              float multiplier = 1.0f, int32_t zeroPoint = 0);
```

- `dst` must be FP32; `src` must be S8 or U8.
- `B.DATR` carries `FP32 + <RMode>` (Sat is always false per the spec).
- `multiplier`/`zeroPoint` travel via `B.IOR` as raw bits, like TQUANT.
- The deprecated `TDEQUANT(dst, src)` form forwards to `TDEQUANT<RNE>`.

Example:

```cpp
TDEQUANT<RoundMode::RTZ>(fp32_dst, s8_src, 2.0f, 0);
```

## TIMG2COL

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TIMG2COL(tile_shape_out &dst, tile_shape_in &src,
              uint32_t posM = 0, uint32_t posK = 0);
```

Implements the PTO 0.58.1 image-to-column transform (TEPL Mode 3 Function 4 /
selector 0x064; canonical `BSTART.SFU TIMG2COL`).

- The feature-map position selectors `posM` / `posK` are encoded into
  `B.IOR` RegSrc0/RegSrc1 (low 16 bits each); omitting them selectors 0/0.
- The source's feature-map descriptor (NC1HWC0 / NDC1HWC0 layout, filter,
  stride, dilation, padding) is a property of the persistent Matrix-location
  source tile; TileOP exposes only the position selectors. The full
  descriptor state and layout legality are enforced by the model.
- The bundle is `B.DIM` (ValidCol/ValidRow/Col) + `B.IOR` + one OneSrc_Dst
  `B.IOT` with `<last>`.

Example:

```cpp
using T = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;

T dst, src;
TIMG2COL(dst, src, /*posM=*/3, /*posK=*/5);
TIMG2COL(dst, src);               // posM=posK=0
```

## Test coverage

`test/tileop_api/src/TQuant.cpp` and `test/tileop_api/src/TImg2Col.cpp`
exercise the SAT/RTZ/default paths and the posM/posK/default paths; the
rebuilt `test_v058_engine_contract.py` asserts the `B.DATR` RMode/Sat and
`B.IOR` multiplier/zero-point bundle fixtures.