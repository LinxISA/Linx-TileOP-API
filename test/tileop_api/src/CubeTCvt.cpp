// PTO-SPEC #167 / ADR-0110: CUBE_M16 and CUBE_M32 TCVT preserve the
// valid shape while independently deriving destination physical shape and
// TSize from the destination dtype. LB2 must be omitted.

#include <common/pto_tileop.hpp>

using namespace pto;

using M16Fp16 = CubeTileM16<__half, 16, 12, 16, 9>;
using M16Fp32 = CubeTileM16<float, 16, 10, 16, 9>;
using M32Fp16 = CubeTileM32<__half, 32, 6, 32, 5>;
using M32Fp32 = CubeTileM32<float, 32, 5, 32, 5>;

static_assert(M16Fp16::TilesizeCode == __tilesize_512B);
static_assert(M16Fp32::TilesizeCode == __tilesize_1KB);
static_assert(M32Fp16::TilesizeCode == __tilesize_512B);
static_assert(M32Fp32::TilesizeCode == __tilesize_1KB);

__attribute__((noinline)) void tcvt_cube_m16(M16Fp32 &dst, M16Fp16 &src) {
  TCVT(dst, src);
}

__attribute__((noinline)) void tcvt_cube_m32(M32Fp32 &dst, M32Fp16 &src) {
  TCVT(dst, src);
}

int main() {
  M16Fp16 m16_src;
  M16Fp32 m16_dst;
  M32Fp16 m32_src;
  M32Fp32 m32_dst;
  tcvt_cube_m16(m16_dst, m16_src);
  tcvt_cube_m32(m32_dst, m32_src);
  return 0;
}
