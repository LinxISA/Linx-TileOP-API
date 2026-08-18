// TPREFETCH: cache-line prefetch from GM without a Tile destination
// (PTO 0.58.1 TLSU function 3; implicit PE 1111, no B.IOT/B.IOS members).
#include <common/pto_tileop.hpp>

using namespace pto;

using GM = global_tensor<float, RowMajor<8, 512>>;

__attribute__((noinline)) void pf(GM &g) { TPREFETCH(g, 512, 2); }

void use(void *) {}

int main() {
  static float buf[8 * 512];
  GM g(buf);
  pf(g);
  use(&g);
  return 0;
}
