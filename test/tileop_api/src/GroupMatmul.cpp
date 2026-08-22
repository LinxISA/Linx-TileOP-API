// PTO 0.58.3 cooperative Shared TMATMUL keeps logical M/K and K/N independent
// of the four publication quarters.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = SharedMatrixLeft<float, 16, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using AC = CubeAccumulatorM16<float, 16, 16>;

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
__attribute__((noinline)) void group_bias(C &c, A &a, B &b, C &bias) {
  auto sa = TMOV_L2S_INSERT(a);
  auto sb = TMOV_L2S_INSERT(b);
  auto sc = TMOV_L2S_INSERT(bias);
  TMATMUL_BIAS(c, sa, sb, bias);
}

void use(void *) {}

int main() {
  C c, bias;
  AC d, c0;
  A a;
  B b;
  group_basic(c, a, b);
  group_acc(d, c0, a, b);
  group_bias(c, a, b, bias);
  use(&c);
  return 0;
}
