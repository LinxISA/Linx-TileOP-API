#include <common/pto_tileop.hpp>

using namespace pto;

using GM16 = global_tensor<float, RowMajor<16, 32>>;
using GM32 = global_tensor<float, RowMajor<32, 32>>;
using VecM16 = VecTileM16<float, 16, 32>;
using VecM32 = VecTileM32<float, 32, 32>;

// Vector-role tiles use the ordinary local tile transport path.  Their CUBE
// layout must select the same conversion selectors as matrix-role tiles:
// M16 => ND2M16/M162ND and M32 => ND2M32/M322ND.
void vec_m16_roundtrip(GM16 &gm, VecM16 &tile) {
  TLOAD(tile, gm);
  TSTORE(gm, tile);
}

void vec_m32_roundtrip(GM32 &gm, VecM32 &tile) {
  TLOAD(tile, gm);
  TSTORE(gm, tile);
}
