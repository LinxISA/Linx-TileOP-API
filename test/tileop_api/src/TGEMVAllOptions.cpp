// Full Options-path instantiation for every TGEMV/TGEMVMX variant.
// TGEMV: d(1xN) = vec(1xK) * mtx(KxN); M=1 semantics. Vector/output tiles
// use physical full-box shape with logical ValidRow=1 / ValidCol=N.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM16<float, 1, 32>; // 1xN
using V = CubeTileM16<float, 1, 64>;        // A: 1xK
using Mtx = CubeTileN8<float, 64, 32>;      // B: KxN
using S1 = Tile<Location::Left, float, 64, 64, BLayout::RowMajor, 1, 64>;
using SM = TileRight<float, 64, 32>;
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 1>;

void all_variants(D &d, D &c, V &v, Mtx &mtx, S1 &sv, SM &sm, R &rout, R &rin) {
  auto opts = fixp::Options<FixpAttr::keep_acc()>{}.row_max(rin, rout).group_max<32>(rout);
  TGEMV(d, mtx, v, opts);
  TGEMV_BIAS(d, mtx, v, c, opts);
  TGEMV_ACC(d, c, mtx, v, opts);
  TGEMV_MX(d, mtx, sm, v, sv, opts);
  TGEMV_MX_BIAS(d, mtx, sm, v, sv, c, opts);
  TGEMV_MX_ACC(d, c, mtx, sm, v, sv, opts);
}

void use(void *) {}

int main() {
  static D d, c;
  static V v;
  static Mtx mtx;
  static S1 sv;
  static SM sm;
  static R rout, rin;
  all_variants(d, c, v, mtx, sv, sm, rout, rin);
  use(&d);
  return 0;
}
