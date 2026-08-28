# Comparison operations

The elementwise comparison family is declared in `CmpMode`
(`include/common/pto_tile.hpp`) and implemented by `TCMP` / `TCMPS` on the
VEC file class. The comparison mode is a **compile-time template parameter**
encoded into the 3-bit `B.DATR.CMode[31:29]` field, matching the PTO v0.58
contract.

## CmpMode

```cpp
// PTO 0.58 B.DATR CMode[31:29] encoding; values are explicit and MUST match
// the ISA. Do not rely on declaration order.
enum class CmpMode : uint8_t {
  EQ = 0,  ///< Equal (==)
  NE = 1,  ///< Not equal (!=)
  LT = 2,  ///< Less than (<)
  GT = 3,  ///< Greater than (>)
  LE = 4,  ///< Less than or equal (<=)
  GE = 5,  ///< Greater than or equal (>=)
};

constexpr bool is_valid_cmp_mode(CmpMode Mode);   // true for EQ..GE
constexpr unsigned cmp_mode_code(CmpMode Mode);    // = static_cast<unsigned>(Mode)
```

The result is a `0` or `1` value (`Word`) per element.

> v0.58 encodes the comparison selector in the 3-bit `B.DATR.CMode` field in
> the numeric order `EQ NE LT GT LE GE`. The C++ enum uses the same explicit
> values (LT=2, GT=3, LE=4, GE=5), so `cmp_mode_code(Mode)` feeds
> `B.DATR.CMode` directly. `is_valid_cmp_mode` rejects any out-of-range
> value a bogus `static_cast` would otherwise smuggle in.

## TCMP

```cpp
template <CmpMode Mode, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void TCMP(tile_shape_out &dst, tile_shape_in &src0, tile_shape_in &src1);
```

- Elementwise comparison of two same-shaped source Tiles.
- `dst` and `src0/src1` must share the same `Rows/Cols` and inner shape;
  all three operands must be VEC (`Location::Vec`) ordinary Local Tiles.
- The mode is a template NTTP; each instantiation emits
  `B.DATR Zero, cmode<0..5>` (the assembler matches the numeric mode, e.g.
  `eq`/`gt`, not a numeric immediate) followed by the canonical `B.IOT`
  bundle.
- Executed by VEC through the `BSTART.VEC TCMP` carrier
  (`TEPL sel=0x00D mode=0 fn=13`).

Example:

```cpp
using Src = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using Dst = Tile<Location::Vec, int32_t, 16, 16, BLayout::RowMajor>;

Dst d;
Src a, b;
TCMP<CmpMode::GT>(d, a, b);   // d[i] = a[i] > b[i]
TCMP<CmpMode::EQ>(d, a, b);   // d[i] = a[i] == b[i]
```

### Deprecated no-mode form

The historical three-operand form is kept as an EQ default:

```cpp
TCMP(d, a, b);                // == TCMP<CmpMode::EQ>(d, a, b)
```

New code should pass the mode explicitly.

### DType support

| DType | EQ | NE | GT | LT | GE | LE |
| --- | --- | --- | --- | --- | --- | --- |
| `int32_t` | yes | yes | yes | yes | yes | yes |
| `int64_t` | no | yes | yes | yes | yes | yes |
| `float` | no | yes | yes | yes | yes | yes |
| `__half` | no | yes | yes | yes | yes | yes |

`EQ` is restricted to `int32_t`; all other modes are available for the four
supported dtypes. Passing an unsupported dtype fails at compile time
(`TCMP not support Boxed Layout!` / `Dtype not support!`). A non-ISA mode is
rejected by `static_assert(is_valid_cmp_mode(...))`.

## TCMPS

`TCMPS` is the tile-scalar comparison form (`TEPL sel=0x02D mode=1 fn=13`).
It compares a Tile against a scalar operand; the scalar travels via the
canonical `B.IOR` slot, never as a Tile source.

```cpp
template <CmpMode Mode, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void TCMPS(tile_shape_out &dst, tile_shape_in &src,
           typename tile_shape_in::DType scalar);

TCMPS<CmpMode::GE>(d, a, 0.5f);   // d[i] = a[i] >= 0.5f
```

The deprecated no-mode form `TCMPS(d, a, scalar)` equals
`TCMPS<CmpMode::EQ>(d, a, scalar)`.

## B.DATR CMode encoding

Each `TCMP<Mode>` / `TCMPS<Mode>` bundle carries exactly one `B.DATR` whose
only non-zero field is `CMode` (padding is `Zero`):

```asm
B.DATR Zero, cmode0  ; CMode=0
B.DATR Zero, cmode1  ; CMode=1
B.DATR Zero, cmode2  ; CMode=2
B.DATR Zero, cmode3  ; CMode=3
B.DATR Zero, cmode4  ; CMode=4
B.DATR Zero, cmode5  ; CMode=5
```

The encodings are verified by `llvm-objdump`:
`EQ=0x01f01023 NE=0x21f01023 LT=0x41f01023 GT=0x61f01023 LE=0x81f01023
GE=0xa1f01023` (CMode in `Inst[31:29]`).

## Test coverage

`test/tileop_api/src/TCmpMode.cpp` instantiates every `CmpMode` for both
`TCMP` and `TCMPS` (plus the deprecated forms) and checks the emitted
`B.DATR CMode`; the CPU simulator exercises the same six comparisons
(`TCmp_Vec_RowMajor` / `TCmp_Vec_ColMajor` kernels and the runtime
`TCMP_Impl` switch, which asserts on an unknown mode).
