// Preserve byte-offset validation for ordinary and SubTileView emitters.
#include <jcore/template_asm.hpp>
#include <common/pto_tile_region_inline_asm.hpp>
using namespace pto;

#ifdef ROWMAJOR
using Output = Tile<Location::Vec, __bf16, 32, 32>;
#else
using Output = VecTileM32<__bf16, 32, 32>;
#endif
using Matrix = VecTileM32<__bf16, 32, 32>;
using Carrier = VecTileM32<__bf16, 32, 2>;
using MatrixParent = VecTileM32<__bf16, 32, 64>;
using CarrierParent = VecTileM32<__bf16, 32, 4>;

void bad(Output &dst, Matrix &matrix, Carrier &carrier,
         MatrixParent &matrix_parent, CarrierParent &carrier_parent) {
#if EXPAND_PATH != 0
  auto matrices = TPARTVIEW<Matrix, 1, 2>(matrix_parent);
  auto carriers = TPARTVIEW<Carrier, 1, 2>(carrier_parent);
  auto matrix_view = matrices[0][1];
  auto carrier_view = carriers[0][1];
#endif
#if EXPAND_PATH == 0
  TROWEXPANDADD<Output, Matrix, Carrier, BAD_OFFSET>(dst, matrix, carrier);
#elif EXPAND_PATH == 1
  TROWEXPANDADD<Output, MatrixParent, Matrix, CarrierParent, Carrier, BAD_OFFSET>(
      dst, matrix_view, carrier_view);
#elif EXPAND_PATH == 2
  TROWEXPANDADD<Output, MatrixParent, Matrix, Carrier, BAD_OFFSET>(
      dst, matrix_view, carrier);
#elif EXPAND_PATH == 3
  TROWEXPANDADD<Output, Matrix, CarrierParent, Carrier, BAD_OFFSET>(
      dst, matrix, carrier_view);
#else
#error select EXPAND_PATH 0..3
#endif
}
