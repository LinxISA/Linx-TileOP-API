// PTO-ISA #291: the unified Local layout model. Exercises the four TileOP
// behaviours the spec change closes:
//   1. elementwise TEPL operations select the operand Local layout through
//      B.DATR.Layout, so a CUBE_M16/M32 source keeps its physical CUBE
//      representation instead of collapsing to the NORM (RowMajor) default;
//   2. TCVT is legal on Vec-location CUBE_M16/M32 tiles (issue #267): the
//      CUBE layout is preserved while the element width may change;
//   3. GMOV copies same-layout CUBE_M16/M32 peers (CUBE_N8 stays illegal);
//   4. Matrix Bias carries the resolver-selected M layout and matches D.
#include <common/pto_tileop.hpp>

using namespace pto;

// --- 1. elementwise layout selection ---
using RowMajorVec = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using CubeM16Vec = VecTileM16<float, 16, 16>;
using CubeM32Vec = VecTileM32<float, 32, 32>;

__attribute__((noinline)) void tadd_rowmajor(RowMajorVec &d, RowMajorVec &a,
                                             RowMajorVec &b) {
  TADD(d, a, b);
}

__attribute__((noinline)) void tadd_cube_m16(CubeM16Vec &d, CubeM16Vec &a,
                                             CubeM16Vec &b) {
  TADD(d, a, b);
}

__attribute__((noinline)) void tadd_cube_m32(CubeM32Vec &d, CubeM32Vec &a,
                                             CubeM32Vec &b) {
  TMUL(d, a, b);
}

// Tile-scalar and unary forms share the same B.DATR Layout selector.
__attribute__((noinline)) void tmuls_cube_m32(CubeM32Vec &d, CubeM32Vec &a) {
  TMULS(d, a, 2.0f);
}

__attribute__((noinline)) void tabs_cube_m16(CubeM16Vec &d, CubeM16Vec &a) {
  TABS(d, a);
}

__attribute__((noinline)) void texpands_cube_m32(CubeM32Vec &d) {
  TEXPANDS(d, 1.0f);
}

// --- 2. Vec-location CUBE_M16/M32 TCVT (issue #267) ---
using VecM16Fp32 = VecTileM16<float, 16, 16>;
using VecM16Fp16 = VecTileM16<__half, 16, 16>;
using VecM32Fp32 = VecTileM32<float, 32, 32>;
using VecM32Fp16 = VecTileM32<__half, 32, 32>;
using VecM32E8M0 = VecTileM32<__fp8_e8m0, 32, 32>;

__attribute__((noinline)) void tcvt_vec_m16(VecM16Fp16 &dst, VecM16Fp32 &src) {
  TCVT(dst, src);
}

__attribute__((noinline)) void tcvt_vec_m32(VecM32Fp16 &dst, VecM32Fp32 &src) {
  TCVT(dst, src);
}

__attribute__((noinline)) void tcvt_vec_m32_e8m0(VecM32E8M0 &dst,
                                                 VecM32Fp32 &src) {
  // MX scale quantization needs the floor rounding mode (OCP MX 6.3).
  TCVT<LINX_RDN>(dst, src);
}

// The Matrix-location forms remain available; location is no longer part of
// the TCVT legality predicate, only the layout and the valid shape are.
using MatrixM16Fp32 = CubeTileM16<float, 16, 16>;
using MatrixM16Fp16 = CubeTileM16<__half, 16, 16>;

__attribute__((noinline)) void tcvt_matrix_m16(MatrixM16Fp16 &dst,
                                               MatrixM16Fp32 &src) {
  TCVT(dst, src);
}

// --- 3. GMOV same-layout peer copies ---
__attribute__((noinline)) void gmov_rowmajor(RowMajorVec &d, RowMajorVec &s) {
  GMOV<15>(d, 0, s);
}

__attribute__((noinline)) void gmov_cube_m16(CubeM16Vec &d, CubeM16Vec &s) {
  GMOV<15>(d, 0, s);
}

__attribute__((noinline)) void gmov_cube_m32(CubeM32Vec &d, CubeM32Vec &s) {
  GMOV<15>(d, 0, s);
}

// --- 4. Matrix Bias carries the resolved M layout ---
using M16A = CubeTileM16<float, 16, 16>;
using M16B = CubeTileN8<float, 16, 16>;
using M16D = CubeAccumulatorM16<float, 16, 16>;
using M16Bias = CubeBias<float, 16>;

using M32A = CubeTileM32<float, 32, 32>;
using M32B = CubeTileN8<float, 32, 32>;
using M32D = CubeAccumulatorM32<float, 32, 32>;
using M32Bias = CubeBias<float, 32, 32>;

static_assert(M16Bias::BFractal == BLayout::CubeM16 && M16Bias::ValidRow == 1,
              "CubeBias<_, 16> declares the CUBE_M16 M layout");
static_assert(M32Bias::BFractal == BLayout::CubeM32 && M32Bias::ValidRow == 1,
              "CubeBias<_, 32> declares the CUBE_M32 M layout");
static_assert(M16Bias::Loc == Location::Bias && M32Bias::Loc == Location::Bias,
              "CubeBias keeps the Bias operand role");

__attribute__((noinline)) void bias_m16(M16D &d, M16A &a, M16B &b,
                                        M16Bias &bias) {
  TMATMUL_BIAS(d, a, b, bias);
}

__attribute__((noinline)) void bias_m32(M32D &d, M32A &a, M32B &b,
                                        M32Bias &bias) {
  TMATMUL_BIAS(d, a, b, bias);
}

int main() {
  RowMajorVec rm_d, rm_a, rm_b;
  CubeM16Vec m16_d, m16_a, m16_b;
  CubeM32Vec m32_d, m32_a, m32_b;
  tadd_rowmajor(rm_d, rm_a, rm_b);
  tadd_cube_m16(m16_d, m16_a, m16_b);
  tadd_cube_m32(m32_d, m32_a, m32_b);
  tmuls_cube_m32(m32_d, m32_a);
  tabs_cube_m16(m16_d, m16_a);
  texpands_cube_m32(m32_d);

  VecM16Fp32 v16s;
  VecM16Fp16 v16d;
  VecM32Fp32 v32s;
  VecM32Fp16 v32d;
  VecM32E8M0 v32e;
  tcvt_vec_m16(v16d, v16s);
  tcvt_vec_m32(v32d, v32s);
  tcvt_vec_m32_e8m0(v32e, v32s);
  MatrixM16Fp32 m16s;
  MatrixM16Fp16 m16d;
  tcvt_matrix_m16(m16d, m16s);

  gmov_rowmajor(rm_d, rm_a);
  gmov_cube_m16(m16_d, m16_a);
  gmov_cube_m32(m32_d, m32_a);

  M16A a16;
  M16B b16;
  M16D d16;
  M16Bias bias16;
  M32A a32;
  M32B b32;
  M32D d32;
  M32Bias bias32;
  bias_m16(d16, a16, b16, bias16);
  bias_m32(d32, a32, b32, bias32);
  return 0;
}
