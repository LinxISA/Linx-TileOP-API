// TLSU GM_RED_VALUE / GM_RED_POPC family: atomic reduction scatter at logical
// element indices with no destination (PTO ISA functions 19-27; canonical
// BSTART.TLSU MSCATTER.<op>). Requires a function-aware assembler (0.58.6+) to
// lower the bundles. U32 is legal for every reduction member; POPC takes only
// an index tile.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using Idx32 = Tile<Location::Vec, int32_t, 8, 256, BLayout::RowMajor>;

__attribute__((noinline)) void ms_add(Idx32 &ix, D &v) {
  MSCATTER_ADD(0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void ms_max(Idx32 &ix, D &v) {
  MSCATTER_MAX(0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void ms_min(Idx32 &ix, D &v) {
  MSCATTER_MIN(0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void ms_and(Idx32 &ix, D &v) {
  MSCATTER_AND(0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void ms_or(Idx32 &ix, D &v) {
  MSCATTER_OR(0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void ms_xor(Idx32 &ix, D &v) {
  MSCATTER_XOR(0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void ms_inc(Idx32 &ix, D &lim) {
  MSCATTER_INC(0x1000ull, ix, lim, 256, 2);
}
__attribute__((noinline)) void ms_dec(Idx32 &ix, D &lim) {
  MSCATTER_DEC(0x1000ull, ix, lim, 256, 2);
}
__attribute__((noinline)) void ms_popc(Idx32 &ix) {
  MSCATTER_POPC(0x1000ull, ix, 256, 2);
}

void use(void *) {}

int main() {
  D v;
  Idx32 ix;
  ms_add(ix, v);
  ms_max(ix, v);
  ms_min(ix, v);
  ms_and(ix, v);
  ms_or(ix, v);
  ms_xor(ix, v);
  ms_inc(ix, v);
  ms_dec(ix, v);
  ms_popc(ix);
  use(&v);
  return 0;
}
