// TQUANT/TDEQUANT: FP32<->S8/U8 with B.DATR RMode/Sat and B.IOR
// multiplier/zero-point (PTO 0.58.1 TEPL Mode3 Fn10/Fn11).
#include <common/pto_tileop.hpp>

using namespace pto;

using F32 = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using S8 = Tile<Location::Vec, int8_t, 8, 256, BLayout::RowMajor>;
using U8 = Tile<Location::Vec, uint8_t, 8, 256, BLayout::RowMajor>;

__attribute__((noinline)) void q_s8_sat(S8 &d, F32 &s) {
  TQUANT<RoundMode::RNE, true>(d, s, 0.5f, 1);
}
__attribute__((noinline)) void q_u8_default(U8 &d, F32 &s) {
  TQUANT(d, s);
}
__attribute__((noinline)) void dq_rtz(F32 &d, S8 &s) {
  TDEQUANT<RoundMode::RTZ>(d, s, 2.0f, 0);
}

void use(void *) {}

int main() {
  F32 s, d;
  S8 s8;
  U8 u8;
  q_s8_sat(s8, s);
  q_u8_default(u8, s);
  dq_rtz(d, s8);
  use(&d);
  return 0;
}
