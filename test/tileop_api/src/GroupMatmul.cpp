// PTO 0.58.3 cooperative Shared TMATMUL keeps logical M/K and K/N independent
// of the four publication quarters.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = SharedMatrixLeft<float, 16, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using AC = CubeAccumulatorM16<float, 16, 16>;
using Bias = Tile<Location::Bias, float, 8, 16,
                  BLayout::RowMajor, 1, 16>;

using A128 = SharedMatrixLeft<float, 128, 64>;
using B128 = SharedMatrixRight<float, 64, 32>;
using C128 = CubeAccumulatorM32<float, 32, 32>;

__attribute__((noinline)) void group_basic(C &c, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a);
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL(c, sa, sb);
}
__attribute__((noinline)) void group_acc(AC &d, AC &c0, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a);
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c0, sa, sb);
}
__attribute__((noinline)) void group_bias(C &c, A &a, B &b, Bias &bias) {
  auto sa = TMOV_L2S_INSERT(a);
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_BIAS(c, sa, sb, bias);
}
__attribute__((noinline)) void group_m128(C128 &c, A128 &a, B128 &b) {
  auto sa = TMOV_L2S_INSERT(a);
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL(c, sa, sb);
}

void use(void *) {}

int main() {
  C c;
  Bias bias;
  AC d, c0;
  A a;
  B b;
  A128 a128;
  B128 b128;
  C128 c128;
  group_basic(c, a, b);
  group_acc(d, c0, a, b);
  group_bias(c, a, b, bias);
  group_m128(c128, a128, b128);
  use(&c);
  return 0;
}
