// TLSU GM_ATOM_VALUE family: atomic RMW gather at logical element indices with
// observed-old-value publication (PTO ISA functions 9-18; canonical
// BSTART.TLSU MGATHER.<op>). Requires a function-aware assembler (0.58.6+) to
// lower the bundles. U32 is legal for every member below, so one transfer tile
// covers ADD/MAX/MIN/AND/OR/XOR/EXCH/INC/DEC.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using Idx32 = Tile<Location::Vec, int32_t, 8, 256, BLayout::RowMajor>;

__attribute__((noinline)) void mg_add(D &old, Idx32 &ix, D &v) {
  MGATHER_ADD(old, 0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void mg_max(D &old, Idx32 &ix, D &v) {
  MGATHER_MAX(old, 0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void mg_min(D &old, Idx32 &ix, D &v) {
  MGATHER_MIN(old, 0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void mg_and(D &old, Idx32 &ix, D &v) {
  MGATHER_AND(old, 0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void mg_or(D &old, Idx32 &ix, D &v) {
  MGATHER_OR(old, 0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void mg_xor(D &old, Idx32 &ix, D &v) {
  MGATHER_XOR(old, 0x1000ull, ix, v, 256, 2);
}
__attribute__((noinline)) void mg_exch(D &old, Idx32 &ix, D &rep) {
  MGATHER_EXCH(old, 0x1000ull, ix, rep, 256, 2);
}
__attribute__((noinline)) void mg_inc(D &old, Idx32 &ix, D &lim) {
  MGATHER_INC(old, 0x1000ull, ix, lim, 256, 2);
}
__attribute__((noinline)) void mg_dec(D &old, Idx32 &ix, D &lim) {
  MGATHER_DEC(old, 0x1000ull, ix, lim, 256, 2);
}

void use(void *) {}

int main() {
  D old, v;
  Idx32 ix;
  mg_add(old, ix, v);
  mg_max(old, ix, v);
  mg_min(old, ix, v);
  mg_and(old, ix, v);
  mg_or(old, ix, v);
  mg_xor(old, ix, v);
  mg_exch(old, ix, v);
  mg_inc(old, ix, v);
  mg_dec(old, ix, v);
  use(&old);
  return 0;
}
