// TMRGSORT: merge two sorted single-row sources (PTO 0.58.1 TEPL Mode3 Fn13).
#include <common/pto_tileop.hpp>

using namespace pto;

using F32 = Tile<Location::Vec, float, 1, 256, BLayout::RowMajor>;
using F16 = Tile<Location::Vec, __half, 1, 256, BLayout::RowMajor>;

__attribute__((noinline)) void mf32(F32 &d, F32 &l, F32 &r) {
  TMRGSORT(d, l, r);
}
__attribute__((noinline)) void mf16_desc(F16 &d, F16 &l, F16 &r) {
  TMRGSORT(d, l, r, true);
}

void use(void *) {}

int main() {
  F32 d, l, r;
  F16 d16, l16, r16;
  mf32(d, l, r);
  mf16_desc(d16, l16, r16);
  use(&d);
  return 0;
}
