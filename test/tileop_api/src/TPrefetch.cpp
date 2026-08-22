// TPREFETCH: cache-line prefetch from GM without a Tile destination
// (PTO ISA 0.58.3 TLSU function 3; implicit PE 1111, no B.IOT/B.IOS members).
#include <common/pto_tileop.hpp>

using namespace pto;

using GM = global_tensor<float, RowMajor<8, 512>>;
using DynamicGM = global_tensor<float, RowMajor<8, DYNAMIC>>;

__attribute__((noinline)) void pf(GM &g) { TPREFETCH(g, 512, 2); }
__attribute__((noinline)) void pf_dynamic(DynamicGM &g, uint32_t validCol,
                                          uint32_t validRow) {
  TPREFETCH(g, validCol, validRow);
}

void use(void *) {}

int main() {
  static float buf[8 * 512];
  GM g(buf);
  DynamicGM dynamicG(buf, 512);
  pf(g);
  pf_dynamic(dynamicG, 256, 4);
  use(&g);
  return 0;
}
