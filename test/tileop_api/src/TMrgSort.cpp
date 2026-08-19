// TMRGSORT: merge two sorted single-row sources (PTO 0.58.1 TEPL Mode3 Fn13).
#include <common/pto_tileop.hpp>

using namespace pto;

using F32Src = Tile<Location::Vec, float, 1, 128, BLayout::RowMajor>;
using F32Dst = Tile<Location::Vec, float, 1, 256, BLayout::RowMajor>;
using F16Src = Tile<Location::Vec, __half, 1, 128, BLayout::RowMajor>;
using F16Dst = Tile<Location::Vec, __half, 1, 256, BLayout::RowMajor>;

__attribute__((noinline)) void mf32(F32Dst &d, F32Src &l, F32Src &r) {
  TMRGSORT(d, l, r);
}
__attribute__((noinline)) void mf16_desc(F16Dst &d, F16Src &l, F16Src &r) {
  TMRGSORT(d, l, r, true);
}

void use(void *) {}

int main() {
  F32Dst d;
  F32Src l, r;
  F16Dst d16;
  F16Src l16, r16;
  mf32(d, l, r);
  mf16_desc(d16, l16, r16);
  use(&d);
  return 0;
}
