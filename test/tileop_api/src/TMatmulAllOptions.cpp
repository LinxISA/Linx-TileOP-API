// Full Options-path instantiation for every TMATMUL/TMATMULMX CUBE variant.
// Each uses the RowMax(AuxIn)+AuxOut path so the whole PostProcess
// operand set (RowMaxIn/Quant/Relu/RowMaxOut/GroupMaxOut + scalar B.IOR)
// is emitted, verifying SRC/EMIT macro operands match their binders.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM32<float, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;
using S = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 1>;

void all_variants(D &d, D &c, A &a, B &b, S &sa, S &sb, R &rout, R &rin) {
  auto opts = fixp::Options<FixpAttr::keep_acc()>{}.row_max(rin, rout).group_max<32>(rout);
  TMATMUL_ACC(d, c, a, b, opts);
  TMATMUL_BIAS(d, a, b, c, opts);
  TMATMUL_MX(d, a, sa, b, sb, opts);
  TMATMUL_MX_ACC(d, c, a, sa, b, sb, opts);
  TMATMUL_MX_BIAS(d, a, sa, b, sb, c, opts);
}

void use(void *) {}

int main() {
  D d, c;
  A a;
  B b;
  S sa, sb;
  R rout, rin;
  all_variants(d, c, a, b, sa, sb, rout, rin);
  use(&d);
  return 0;
}
