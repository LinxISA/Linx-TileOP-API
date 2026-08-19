// Issue #18: Group TMATMUL (Shared A + Shared B, local C per PE) must use
// per-PE compute window: LB0 = local C rows = SharedA.Rows / PECount(4).
// Group: SharedA=[64,16] SharedB=[16,16] C=[16,16] -> LB0=16, LB1=16, LB2=16.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = TileLeft<float, 64, 16>;
using B = TileRight<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using AC = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

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
