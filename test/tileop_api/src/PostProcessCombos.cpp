// Attribute-combination test: exercise the FixpAttr option matrix beyond the
// RowMax+GroupMax path already covered by TMatmulAllOptions/TGEMVAllOptions.
// Covers scalar/vector quant, LReLU/PReLU, fresh vs init RowMax, GroupMax
// and MaxAbs across the TMATMUL and TGEMV families.
#include <common/pto_tileop.hpp>

using namespace pto;

// TMATMUL shapes
using D = CubeAccumulatorM32<float, 32, 32>;
using Ds8 = CubeAccumulatorM32<int8_t, 32, 32>;
using Df16 = CubeAccumulatorM32<__half, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;
using MXA = CubeTileM32<__fp8_e4m3, 32, 64>;
using MXB = CubeTileN8<__fp8_e4m3, 64, 32>;
using MXSA = Tile<Location::Scaling, __fp8_e8m0, 32, 4,
                  BLayout::RowMajor, 32, 2>;
using MXSB = Tile<Location::Scaling, __fp8_e8m0, 4, 32,
                  BLayout::RowMajor, 2, 32>;
// quant/PReLU param: 1xN logical, physical 2x32 (>= 128 B)
using P = Tile<Location::Vec, uint64_t, 2, 32, BLayout::RowMajor, 1, 32>;
// RowMaxOut (Mx1) / GroupMaxOut (Mx2 for GroupN=16, N=32)
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 1>;
using G = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 2>;

// TGEMV shapes: d 1xN, vec 1xK, mtx KxN
using GV_D = CubeAccumulatorM16<float, 1, 32>;
using GV_Ds8 = CubeAccumulatorM16<int8_t, 1, 32>;
using GV_Df16 = CubeAccumulatorM16<__half, 1, 32>;
using GV_V = CubeTileM16<float, 1, 64>;
using GV_M = CubeTileN8<float, 64, 32>;
using GV_MXA = CubeTileM16<__fp8_e4m3, 1, 64>;
using GV_MXB = CubeTileN8<__fp8_e4m3, 64, 32>;
using GV_MXSA = Tile<Location::Scaling, __fp8_e8m0, 32, 4,
                     BLayout::RowMajor, 1, 2>;
using GV_MXSB = Tile<Location::Scaling, __fp8_e8m0, 4, 32,
                     BLayout::RowMajor, 2, 32>;

static constexpr uint64_t s8_desc =
    (static_cast<uint64_t>(0x7) << 13) |  // fp19 scale = 7
    (static_cast<uint64_t>(0x1) << 37);   // s8 offset = 1

// --- TMATMUL family ---
void tmatmul_combos(D &d, Ds8 &d8, Df16 &df16, D &c, A &a, B &b,
                    MXA &mxa, MXB &mxb, MXSA &mxsa, MXSB &mxsb, P &q, P &p,
                    R &rout, R &rin, G &gout) {
  // scalar quant (s8) + LReLU
  TMATMUL(d8, a, b, fixp::s8(s8_desc).lrelu(0x12345));
  // vector quant (s8) + PReLU
  TMATMUL(d8, a, b, fixp::s8(q).prelu(p));
  // fresh RowMax + GroupMax + MaxAbs (all max outputs)
  TMATMUL(d, a, b,
          fixp::keep_acc().row_max(rout).group_max<16>(gout).max_abs());
  // relu + RowMaxInit
  TMATMUL(df16, a, b, fixp::f16().relu().row_max(rin, rout));
  // full: scalar quant + LReLU + RowMaxInit + GroupMax + MaxAbs
  TMATMUL(d8, a, b, fixp::s8(s8_desc).lrelu(0x123).row_max(rin, rout)
                        .group_max<16>(gout).max_abs());

  TMATMUL_ACC(d, c, a, b, fixp::keep_acc().row_max(rin, rout));
  TMATMUL_BIAS(df16, a, b, c, fixp::f16().relu());
  TMATMUL_MX(d, mxa, mxsa, mxb, mxsb,
             fixp::keep_acc().group_max<16>(gout));
  TMATMUL_MX_ACC(df16, c, mxa, mxsa, mxb, mxsb, fixp::f16());
  TMATMUL_MX_BIAS(df16, mxa, mxsa, mxb, mxsb, c,
                  fixp::f16().prelu(p));
}

// --- TGEMV family ---
void tgemv_combos(GV_D &d, GV_Ds8 &d8, GV_Df16 &df16, GV_D &c,
                  GV_V &v, GV_M &m, GV_MXA &mxa, GV_MXB &mxb,
                  GV_MXSA &mxsa, GV_MXSB &mxsb,
                  P &q, P &p, R &rout, R &rin, G &gout) {
  TGEMV(d8, m, v, fixp::s8(s8_desc).lrelu(0x123));
  TGEMV(d, m, v, fixp::keep_acc().row_max(rout).group_max<16>(gout).max_abs());
  TGEMV_ACC(d, c, m, v, fixp::keep_acc().row_max(rin, rout));
  TGEMV_BIAS(df16, m, v, c, fixp::f16().relu());
  TGEMV_MX(d, mxb, mxsb, mxa, mxsa,
           fixp::keep_acc().group_max<16>(gout));
  TGEMV_MX_ACC(df16, c, mxb, mxsb, mxa, mxsa, fixp::f16());
  TGEMV_MX_BIAS(df16, mxb, mxsb, mxa, mxsa, c,
                fixp::f16().prelu(p));
}

void use(void *) {}

int main() {
  static D d, c;
  static Ds8 d8;
  static Df16 df16;
  static A a;
  static B b;
  static MXA mxa;
  static MXB mxb;
  static MXSA mxsa;
  static MXSB mxsb;
  static P q, p;
  static R rout, rin;
  static G gout;
  static GV_D gd, gc;
  static GV_Ds8 gd8;
  static GV_Df16 gdf16;
  static GV_V gv;
  static GV_M gm;
  static GV_MXA gmxa;
  static GV_MXB gmxb;
  static GV_MXSA gmxsa;
  static GV_MXSB gmxsb;
  tmatmul_combos(d, d8, df16, c, a, b, mxa, mxb, mxsa, mxsb,
                 q, p, rout, rin, gout);
  tgemv_combos(gd, gd8, gdf16, gc, gv, gm, gmxa, gmxb, gmxsa, gmxsb,
               q, p, rout, rin, gout);
  use(&d);
  return 0;
}
