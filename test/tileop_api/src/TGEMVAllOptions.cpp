// Full Options-path instantiation for every TGEMV/TGEMVMX variant.
// TGEMV: d(1xN) = vec(1xK) * mtx(KxN); M=1 semantics. Vector/output tiles
// use physical full-box shape with logical ValidRow=1 / ValidCol=N.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM16<float, 1, 32>; // 1xN
using V = CubeTileM16<float, 1, 64>;        // A: 1xK
using Mtx = CubeTileN8<float, 64, 32>;      // B: KxN
using MXV = CubeTileM16<__fp8_e4m3, 1, 64>;
using MXM = CubeTileN8<__fp8_e4m3, 64, 32>;
using SV = Tile<Location::Scaling, __fp8_e8m0, 32, 4,
                BLayout::RowMajor, 1, 2>;
using SM = Tile<Location::Scaling, __fp8_e8m0, 4, 32,
                BLayout::RowMajor, 2, 32>;
using R = Tile<Location::Vec, float, 16, 8, BLayout::RowMajor, 1, 1>;
using G = Tile<Location::Vec, float, 16, 8, BLayout::RowMajor, 1, 1>;
using Bias = Tile<Location::Bias, float, 8, 32,
                  BLayout::RowMajor, 1, 32>;

void all_variants(D &d, D &c, Bias &bias, V &v, Mtx &mtx,
                  MXV &mxv, MXM &mxm, SV &sv, SM &sm,
                  R &rout, R &rin, G &gout) {
  auto opts = fixp::Options<FixpAttr::keep_acc()>{}
                  .row_max(rin, rout).group_max<32>(gout);
  TGEMV(d, mtx, v, opts);
  TGEMV_BIAS(d, mtx, v, bias, opts);
  TGEMV_ACC(d, c, mtx, v, opts);
  TGEMV_MX(d, mxm, sm, mxv, sv, opts);
  TGEMV_MX_BIAS(d, mxm, sm, mxv, sv, bias, opts);
  TGEMV_MX_ACC(d, c, mxm, sm, mxv, sv, opts);
}

void use(void *) {}

int main() {
  static D d, c;
  static Bias bias;
  static V v;
  static Mtx mtx;
  static MXV mxv;
  static MXM mxm;
  static SV sv;
  static SM sm;
  static R rout, rin;
  static G gout;
  all_variants(d, c, bias, v, mtx, mxv, mxm, sv, sm, rout, rin, gout);
  use(&d);
  return 0;
}
