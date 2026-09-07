// TIMG2COL: GM feature-map image-to-column with packed parameter GPRs.
#include <common/pto_tileop.hpp>

using namespace pto;

using T = CubeTileM32<float, 32, 256>;
using GM = global_tensor<float, RowMajor<8, 256>>;

__attribute__((noinline)) void ic(T &d, GM &s) {
  TIMG2COL(d, s, 0x0008000800040008ULL, 0, 0);
}

void use(void *) {}

int main() {
  T d;
  float data[8 * 256] = {};
  GM s(data);
  ic(d, s);
  use(&d);
  return 0;
}
