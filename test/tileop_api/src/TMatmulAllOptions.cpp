// Full Options-path instantiation for every TMATMUL/TMATMULMX CUBE variant.
// Each uses the RowMax(AuxIn)+AuxOut path so the whole PostProcess
// operand set (RowMaxIn/Quant/Relu/RowMaxOut/GroupMaxOut + scalar B.IOR)
// is emitted, verifying SRC/EMIT macro operands match their binders.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM32<float, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;
using MXA = CubeTileM32<__fp8_e4m3, 32, 64>;
using MXB = CubeTileN8<__fp8_e4m3, 64, 32>;
using SA = Tile<Location::Scaling, __fp8_e8m0, 32, 4,
                BLayout::RowMajor, 32, 2>;
using SB = Tile<Location::Scaling, __fp8_e8m0, 4, 32,
                BLayout::RowMajor, 2, 32>;
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 1>;
using Bias = Tile<Location::Bias, float, 8, 32,
                  BLayout::RowMajor, 1, 32>;

void all_variants(D &d, D &c, Bias &bias, A &a, B &b, MXA &mxa, MXB &mxb,
                  SA &sa, SB &sb, R &rout, R &rin) {
  auto opts = fixp::Options<FixpAttr::keep_acc()>{}.row_max(rin, rout).group_max<32>(rout);
  TMATMUL_ACC(d, c, a, b, opts);
  TMATMUL_BIAS(d, a, b, bias, opts);
  TMATMUL_MX(d, mxa, sa, mxb, sb, opts);
  TMATMUL_MX_ACC(d, c, mxa, sa, mxb, sb, opts);
  TMATMUL_MX_BIAS(d, mxa, sa, mxb, sb, bias, opts);
}

void use(void *) {}

int main() {
  D d, c;
  Bias bias;
  A a;
  B b;
  MXA mxa;
  MXB mxb;
  SA sa;
  SB sb;
  R rout, rin;
  all_variants(d, c, bias, a, b, mxa, mxb, sa, sb, rout, rin);
  use(&d);
  return 0;
}
