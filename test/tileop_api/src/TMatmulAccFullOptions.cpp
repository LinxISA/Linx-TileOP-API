#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM32<float, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;
// RowMaxOut: physical 32x32, logical valid M x 1 (ValidRow=32, ValidCol=1).
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 1>;

// TMATMUL.ACC with full PostProcess: RowMax output (RowMaxEn=true) via the
// options chain builder.
void tmatmul_acc_rowmax(D &d, D &c, A &a, B &b, R &rout) {
  TMATMUL_ACC(d, c, a, b, fixp::Options<FixpAttr::keep_acc()>{}.row_max(rout));
}

int main() { return 0; }
