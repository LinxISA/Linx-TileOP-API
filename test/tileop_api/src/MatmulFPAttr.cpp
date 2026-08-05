#include <common/pto_tileop.hpp>

// MatmulFPAttr: pins the v5 FPATR contract for all 12 TMATMUL* public
// interfaces. Every CUBE bundle must carry exactly one B.FPATR after B.DATR.
// Two call shapes are exercised per interface:
//   * no options argument  -> default Attr (FixpAttr{}) -> all-zero B.FPATR
//   * fixp::keep_acc().relu()         -> non-zero ReluMode, still parameter-free
// Non-FIXP variants reject parameterized options (quant/PReLU/RowMax/...) at
// compile time via static_assert(is_basic_fixp_attr); FIXP variants share the
// same restriction in this overload tier.

using namespace pto;

using D = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using A = TileLeft<float, 16, 16>;
using B = TileRight<float, 16, 16>;
using ScaleA = TileLeft<float, 16, 16>;
using ScaleB = TileRight<float, 16, 16>;

void fpatr_all_default(D &d, D &extra, A &a, B &b, ScaleA &sa, ScaleB &sb) {
  TMATMUL(d, a, b);
  TMATMUL_ACC(d, extra, a, b);
  TMATMUL_BIAS(d, a, b, extra);
  TMATMUL_MX(d, a, sa, b, sb);
  TMATMUL_MX_ACC(d, extra, a, sa, b, sb);
  TMATMUL_MX_BIAS(d, a, sa, b, sb, extra);
  TMATMUL_FIXP(d, a, b);
  TMATMUL_ACC_FIXP(d, extra, a, b);
  TMATMUL_BIAS_FIXP(d, a, b, extra);
  TMATMUL_MX_FIXP(d, a, sa, b, sb);
  TMATMUL_MX_ACC_FIXP(d, extra, a, sa, b, sb);
  TMATMUL_MX_BIAS_FIXP(d, a, sa, b, sb, extra);
}

void fpatr_all_relu(D &d, D &extra, A &a, B &b, ScaleA &sa, ScaleB &sb) {
  TMATMUL(d, a, b, fixp::keep_acc().relu());
  TMATMUL_ACC(d, extra, a, b, fixp::keep_acc().relu());
  TMATMUL_BIAS(d, a, b, extra, fixp::keep_acc().relu());
  TMATMUL_MX(d, a, sa, b, sb, fixp::keep_acc().relu());
  TMATMUL_MX_ACC(d, extra, a, sa, b, sb, fixp::keep_acc().relu());
  TMATMUL_MX_BIAS(d, a, sa, b, sb, extra, fixp::keep_acc().relu());
  TMATMUL_FIXP(d, a, b, fixp::keep_acc().relu());
  TMATMUL_ACC_FIXP(d, extra, a, b, fixp::keep_acc().relu());
  TMATMUL_BIAS_FIXP(d, a, b, extra, fixp::keep_acc().relu());
  TMATMUL_MX_FIXP(d, a, sa, b, sb, fixp::keep_acc().relu());
  TMATMUL_MX_ACC_FIXP(d, extra, a, sa, b, sb, fixp::keep_acc().relu());
  TMATMUL_MX_BIAS_FIXP(d, a, sa, b, sb, extra, fixp::keep_acc().relu());
}

int main() { return 0; }
