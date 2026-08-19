// TIMG2COL: image-to-column with feature-map posM/posK
// (PTO 0.58.1 TEPL Mode3 Fn4 / selector 0x064; B.IOR PosMGPR, PosKGPR).
#include <common/pto_tileop.hpp>

using namespace pto;

using T = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;

__attribute__((noinline)) void ic_pos(T &d, T &s) { TIMG2COL(d, s, 3, 5); }
__attribute__((noinline)) void ic_default(T &d, T &s) { TIMG2COL(d, s); }

void use(void *) {}

int main() {
  T d, s;
  ic_pos(d, s);
  ic_default(d, s);
  use(&d);
  return 0;
}
