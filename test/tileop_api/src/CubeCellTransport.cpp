#include <common/pto_tileop.hpp>

using namespace pto;

using GM = global_tensor<float, RowMajor<32, 32>>;
using A = CubeTileM32<float, 32, 32>;
using B = CubeTileN8<float, 32, 32>;
using C = CubeAccumulatorM32<float, 32, 32>;

void cube_cell_roundtrip(GM &ga, GM &gb, GM &gc, A &a, B &b, C &c) {
  TLOAD_CUBE(a, ga);
  TLOAD_CUBE(b, gb);
  TMATMUL(c, a, b);
  TSTORE_CUBE(gc, c);
}

// CUBE_M16 transport and ACC: D = C + A*B.  This covers the M16 load/store
// selectors in addition to the M32/N8 selectors in the basic roundtrip.
using AAcc = CubeTileM16<float, 16, 16>;
using BAcc = CubeTileN8<float, 16, 8>;
using CAcc = CubeAccumulatorM16<float, 16, 8>;
using DAcc = CubeAccumulatorM16<float, 16, 8>;
using GMA = global_tensor<float, RowMajor<16, 16>>;
using GMB = global_tensor<float, RowMajor<16, 8>>;
using GMD = global_tensor<float, RowMajor<16, 8>>;

void cube_acc_roundtrip(GMA &ga, GMB &gb, GMD &gd, AAcc &a, BAcc &b,
                        CAcc &c, DAcc &d) {
  TLOAD(a, ga);
  TLOAD(b, gb);
  TMATMUL_ACC(d, c, a, b);
  TSTORE(gd, d);
}

int main() {
  float a32_data[32 * 32] = {};
  float b32_data[32 * 32] = {};
  float c32_data[32 * 32] = {};
  GM ga(a32_data), gb(b32_data), gc(c32_data);
  A a; B b; C c;
  cube_cell_roundtrip(ga, gb, gc, a, b, c);
  float a16_data[16 * 16] = {};
  float b16_data[16 * 8] = {};
  float d16_data[16 * 8] = {};
  GMA ga2(a16_data); GMB gb2(b16_data); GMD gd2(d16_data);
  AAcc a2; BAcc b2; CAcc c2; DAcc d2;
  cube_acc_roundtrip(ga2, gb2, gd2, a2, b2, c2, d2);
  return 0;
}
