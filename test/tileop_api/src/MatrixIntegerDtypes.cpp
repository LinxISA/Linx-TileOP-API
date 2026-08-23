#include <common/pto_tileop.hpp>

using namespace pto;

using SA = CubeTileM16<int8_t, 16, 32>;
using SB = CubeTileN8<int8_t, 32, 16>;
using SD = CubeAccumulatorM16<int32_t, 16, 16>;
using SBias = Tile<Location::Bias, int32_t, 8, 16,
                   BLayout::RowMajor, 1, 16>;

using UA = CubeTileM16<uint8_t, 16, 32>;
using UB = CubeTileN8<uint8_t, 32, 16>;
using UD = CubeAccumulatorM16<uint32_t, 16, 16>;
using UBias = Tile<Location::Bias, uint32_t, 8, 16,
                   BLayout::RowMajor, 1, 16>;
using SRow = Tile<Location::Vec, int32_t, 16, 8,
                  BLayout::RowMajor, 16, 1>;
using URow = Tile<Location::Vec, uint32_t, 16, 8,
                  BLayout::RowMajor, 16, 1>;

void signed_matrix(SD &d, SD &c, SA &a, SB &b, SBias &bias,
                   SRow &row) {
  TMATMUL(d, a, b);
  TMATMUL_ACC(d, c, a, b);
  TMATMUL_BIAS(d, a, b, bias);
  TMATMUL(d, a, b, fixp::keep_acc().row_max(row));
}

void unsigned_matrix(UD &d, UD &c, UA &a, UB &b, UBias &bias, URow &row) {
  TMATMUL(d, a, b);
  TMATMUL_ACC(d, c, a, b);
  TMATMUL_BIAS(d, a, b, bias);
  TMATMUL(d, a, b, fixp::keep_acc().row_max(row));
}
