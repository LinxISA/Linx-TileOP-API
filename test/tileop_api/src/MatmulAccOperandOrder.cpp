#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM32<float, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;

void matmul_acc_operand_order(D &d, D &c, A &a, B &b) {
  TMATMUL_ACC(d, c, a, b);
}

int main() { return 0; }
