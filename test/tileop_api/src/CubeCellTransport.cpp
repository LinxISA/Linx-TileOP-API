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
