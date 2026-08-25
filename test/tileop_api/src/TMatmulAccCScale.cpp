#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM32<float, 32, 32>;
using A = CubeTileM32<__half, 32, 64>;
using B = CubeTileN8<__half, 64, 32>;
using CScale = Tile<Location::Vec, uint8_t, 32, 32,
                    BLayout::CubeM32, 32, 1>;

void tmatmul_acc_cscale(D &d, D &c, A &a, B &b, CScale &scale) {
  TMATMUL_ACC(d, c, a, b, fixp::keep_acc().cscale(scale));
}

void tmatmul_mx_acc_cscale(D &d, D &c, A &a, B &b, CScale &scale) {
  TMATMUL_MX_ACC(d, c, a, b, fixp::keep_acc().cscale(scale));
}

int main() { return 0; }
