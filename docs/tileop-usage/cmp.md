# Comparison operations

The elementwise comparison family is declared in `CmpMode`
(`include/common/pto_tile.hpp`) and implemented by `TCMP` / `TCMPS` on the
VEC file class.

## CmpMode

```cpp
enum class CmpMode {
  EQ,  ///< Equal (==)
  NE,  ///< Not equal (!=)
  GT,  ///< Greater than (>)
  LT,  ///< Less than (<)
  GE,  ///< Greater than or equal (>=)
  LE,  ///< Less than or equal (<=)
};
```

The result is a `0` or `1` value (`Word`) per element.

> Encoding note: the v0.58 catalog encodes the comparison selector in the
> 3-bit `B.DATR.CMode` field and maps the six comparisons in the numeric
> order `EQ NE LT GT LE GE` (so the architectural code for `LT` is one less
> than for `GT`). The C++ `CmpMode` enum keeps a source-friendly order;
> which C++ value maps to which `CMode` code is determined by the lowering
> path.

## TCMP

```cpp
template <typename tile_shape_out, typename tile_shape_in>
void TCMP(tile_shape_out &dst, tile_shape_in &src0, tile_shape_in &src1,
          CmpMode cmpMode);
```

- Elementwise comparison of two same-shaped source Tiles.
- `dst` and `src0/src1` must share the same `Rows/Cols` and inner shape; all
  three operands must be VEC (`Location::Vec`) ordinary Local Tiles.
- Executed by VEC through the `BSTART.VEC TCMP` carrier
  (`TEPL sel=0x00D mode=0 fn=13`).
- The backend also provides the SIMT-style kernel implementations
  `TCmp_Vec_RowMajor` / `TCmp_Vec_ColMajor` (used for the `<<< ... >>>`
  launch form).

### DType support

| DType | EQ | NE | GT | LT | GE | LE |
| --- | --- | --- | --- | --- | --- | --- |
| `int32_t` | yes | yes | yes | yes | yes | yes |
| `int64_t` | no | yes | yes | yes | yes | yes |
| `float` | no | yes | yes | yes | yes | yes |
| `__half` | no | yes | yes | yes | yes | yes |

`EQ` is restricted to `int32_t`; all other modes are available for the four
supported dtypes. Passing an unsupported dtype fails at compile time
(`TCMP not support Boxed Layout!` / `Dtype not support!`).

## TCMPS

`TCMPS` is the tile-scalar comparison form (`TEPL sel=0x02D mode=1 fn=13`).
It compares a Tile against a scalar/immediate operand and is listed as
`BSTART.VEC TCMPS` selector 45 in [engines.md](engines.md).

## Test coverage

`test/tileop_api/src/TCmp.cpp` exercises every `CmpMode` for `float`,
`__half`, `int32_t`, and `int64_t` in both RowMajor and ColMajor shapes.