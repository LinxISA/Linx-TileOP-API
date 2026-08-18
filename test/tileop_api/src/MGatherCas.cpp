// MGATHER_CAS: atomic compare-and-swap at byte displacements
// (PTO 0.58.1 TLSU function 8; canonical BSTART.MGATHER.CAS).
#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using Idx16 = Tile<Location::Vec, int16_t, 8, 256, BLayout::RowMajor>;

__attribute__((noinline)) void mg_u32(D &d, Idx16 &ix, D &ex, D &rp) {
  MGATHER_CAS(d, 0x1000ull, ix, ex, rp, 256, 2);
}

void use(void *) {}

int main() {
  D d, ex, rp;
  Idx16 ix;
  mg_u32(d, ix, ex, rp);
  use(&d);
  return 0;
}
