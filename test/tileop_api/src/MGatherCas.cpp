// MGATHER_CAS: atomic compare-and-swap at logical element indices
// (PTO ISA 0.58.6 TLSU function 8; canonical BSTART.TLSU MGATHER.CAS).
#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using Idx32 = Tile<Location::Vec, int32_t, 8, 256, BLayout::RowMajor>;

__attribute__((noinline)) void mg_u32(D &d, Idx32 &ix, D &ex, D &rp) {
  MGATHER_CAS(d, 0x1000ull, ix, ex, rp, 512, 256, 2);
}

void use(void *) {}

int main() {
  D d, ex, rp;
  Idx32 ix;
  mg_u32(d, ix, ex, rp);
  use(&d);
  return 0;
}
