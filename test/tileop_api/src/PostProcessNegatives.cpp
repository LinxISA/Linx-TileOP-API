// Negative tests: each block below must fail to compile (the assertion named
// in the message fires). Driven by run_negatives.sh which compiles this file
// once per -DSHOULD_FAIL_<N> and expects failure.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using Ds8 = Tile<Location::Vec, int8_t, 32, 32, BLayout::RowMajor>;
using A = TileLeft<float, 32, 64>;
using B = TileRight<float, 64, 32>;
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 2>;  // bad ValidCol
using G = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 8>;  // bad for N=32/GroupN=16
using GVD = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 1, 32>;
using GVV = Tile<Location::Left, float, 32, 64, BLayout::RowMajor, 1, 64>;
using GVVBad = Tile<Location::Left, float, 64, 64, BLayout::RowMajor, 1, 64>;
using GVM = TileRight<float, 64, 32>;
using GVPPBad = Tile<Location::Vec, float, 64, 64, BLayout::RowMajor, 1, 1>;

void fail_cases(D &d, Ds8 &d8, A &a, B &b, R &r, G &g, GVD &gd,
                GVV &gv, GVVBad &gv_bad, GVM &gm, GVPPBad &pp_bad) {
#if defined(SHOULD_FAIL_dtype)
  // QF322S8Pre requires an S8 destination, not FP32.
  TMATMUL(d, a, b, fixp::s8(0x7));
#endif
#if defined(SHOULD_FAIL_maxabs_no_max)
  // max_abs() without RowMax or GroupMax is rejected by the builder.
  TMATMUL(d, a, b, fixp::keep_acc().max_abs());
#endif
#if defined(SHOULD_FAIL_rowmax_shape)
  // RowMaxOut ValidCol must be 1.
  TMATMUL(d, a, b, fixp::keep_acc().row_max(r));
#endif
#if defined(SHOULD_FAIL_groupmax_shape)
  // GroupMaxOut ValidCol must be ceil(N/GroupN); N=32,GroupN=16 -> 2, not 8.
  TMATMUL(d, a, b, fixp::keep_acc().group_max<16>(g));
#endif
#if defined(SHOULD_FAIL_lone_shared_a)
  // Shared A without Shared B is rejected (single binder = Shared-Right).
  auto sa = TMOV_L2S_INSERT(a);
  TMATMUL(d, sa, b);
#endif
#if defined(SHOULD_FAIL_basic_f16_dtype)
  // A parameter-free F16 conversion requires an F16 destination.
  TMATMUL<FixpAttr::f16()>(d, a, b);
#endif
#if defined(SHOULD_FAIL_tgemv_oversize)
  // A 64x64 FP32 physical vector is 16 KiB and cannot be encoded by TSize.
  TGEMV(gd, gm, gv_bad, fixp::keep_acc());
#endif
#if defined(SHOULD_FAIL_tgemv_dtype)
  // S8 pre-quant cannot write an FP32 TGEMV destination.
  TGEMV(gd, gm, gv, fixp::s8(0x7));
#endif
#if defined(SHOULD_FAIL_tgemv_pp_oversize)
  // A postprocess RowMax tile must also fit the 8 KiB TSize ceiling.
  TGEMV(gd, gm, gv,
        fixp::Options<FixpAttr::keep_acc()>{}.row_max(pp_bad));
#endif
}

void use(void *) {}

int main() {
  static D d;
  static Ds8 d8;
  static A a;
  static B b;
  static R r;
  static G g;
  static GVD gd;
  static GVV gv;
  static GVVBad gv_bad;
  static GVM gm;
  static GVPPBad pp_bad;
  fail_cases(d, d8, a, b, r, g, gd, gv, gv_bad, gm, pp_bad);
  use(&d);
  return 0;
}
