#include <common/pto_tileop.hpp>

using namespace pto;

// Issue #178: a CUBE_M16/M32 source keeps the Tile descriptors' CUBE layout
// while B.DATR.Layout stays NORM (omitted); the destination CELL shape is
// derived from the destination DataType and LB2 must be omitted. The
// emitted bundle must therefore be "BSTART.TEPL TCVT, <SrcDtype> /
// B.DATR <DstDtype>, <RMode>" with no CUBE_M16/CUBE_M32 layout token.

using Bf16Cube32 = CubeTileM32<__bf16, 32, 1>;
using E8M0Cube32 = CubeTileM32<__fp8_e8m0, 32, 1>;
using Bf16Cube16 = CubeTileM16<__bf16, 16, 1>;
using E8M0Cube16 = CubeTileM16<__fp8_e8m0, 16, 1>;

__attribute__((noinline)) void bf16_to_e8m0_m32(E8M0Cube32 &dst,
                                                Bf16Cube32 &src) {
  TCVT<LINX_RDN>(dst, src);
}

__attribute__((noinline)) void e8m0_to_bf16_m32(Bf16Cube32 &dst,
                                                E8M0Cube32 &src) {
  TCVT<LINX_RNONE>(dst, src);
}

__attribute__((noinline)) void bf16_to_e8m0_m16(E8M0Cube16 &dst,
                                                Bf16Cube16 &src) {
  TCVT<LINX_RDN>(dst, src);
}

// Dynamic valid-row CUBE_M32 goes through the register-B.DIM branch and
// must keep the same NORM B.DATR spelling.
using Bf16Cube32Dyn = CubeTileM32<__bf16, 32, 1, -1, 1>;
using E8M0Cube32Dyn = CubeTileM32<__fp8_e8m0, 32, 1, -1, 1>;

__attribute__((noinline)) void bf16_to_e8m0_m32_dyn(E8M0Cube32Dyn &dst,
                                                    Bf16Cube32Dyn &src) {
  TCVT<LINX_RDN>(dst, src);
}
