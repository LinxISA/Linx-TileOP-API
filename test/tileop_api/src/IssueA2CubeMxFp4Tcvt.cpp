// A2 regression coverage for MX-FP4 CUBE-M TCVT.  The packed FP4 source and
// BF16 destination exercise the destination-dtype-dependent CELL geometry
// while preserving the source CUBE-M layout and valid shape.

#include <common/pto_tileop.hpp>

using namespace pto;

using M16Fp4 = CubeTileM16<__fp4_e2m1x2, 16, 32, 16, 32>;
using M16Bf16 = CubeTileM16<__bf16, 16, 32, 16, 32>;
using M32Fp4 = CubeTileM32<__fp4_e2m1x2, 32, 32, 32, 32>;
using M32Bf16 = CubeTileM32<__bf16, 32, 32, 32, 32>;

static_assert(M16Fp4::BFractal == M16Bf16::BFractal);
static_assert(M32Fp4::BFractal == M32Bf16::BFractal);
static_assert(M16Fp4::ValidRow == M16Bf16::ValidRow &&
              M16Fp4::ValidCol == M16Bf16::ValidCol);
static_assert(M32Fp4::ValidRow == M32Bf16::ValidRow &&
              M32Fp4::ValidCol == M32Bf16::ValidCol);

__attribute__((noinline)) void tcvt_a2_m16(M16Bf16 &dst, M16Fp4 &src) {
  TCVT(dst, src);
}

__attribute__((noinline)) void tcvt_a2_m32(M32Bf16 &dst, M32Fp4 &src) {
  TCVT<LINX_RDN>(dst, src);
}

int main() {
  M16Fp4 m16_src;
  M16Bf16 m16_dst;
  M32Fp4 m32_src;
  M32Bf16 m32_dst;
  tcvt_a2_m16(m16_dst, m16_src);
  tcvt_a2_m32(m32_dst, m32_src);
  return 0;
}