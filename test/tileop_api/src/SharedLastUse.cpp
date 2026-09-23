// Explicit Shared B.IOS last-use coverage. The ordinary call intentionally
// precedes the opt-in call so its Shared operand must retain `.reuse`; each
// *_LAST_USE call is the final dynamic read of its Shared primary operands.
#include <common/pto_tileop.hpp>

using namespace pto;

using LocalA = CubeTileM16<float, 16, 16>;
using SharedA = SharedMatrixLeft<float, 16, 16>;
using SharedB = SharedMatrixRight<float, 16, 16>;
using Acc = CubeAccumulatorM16<float, 16, 16>;
using SharedAHandle = SharedTile<SharedA>;
using SharedBHandle = SharedTile<SharedB>;
using Global = global_tensor<float, RowMajor<16, 16>>;

__attribute__((noinline)) void local_shared_last_use(float *lhs, float *rhs0,
                                                      float *rhs1) {
  Global global_a(lhs);
  Global global_b0(rhs0);
  Global global_b1(rhs1);
  LocalA a;
  SharedBHandle retained_then_killed;
  SharedBHandle acc_killed;
  Acc d;
  Acc c;
  TLOAD_CUBE(a, global_a);
  TLOAD(retained_then_killed, global_b0);
  TLOAD(acc_killed, global_b1);
  TMATMUL(d, a, retained_then_killed);
  TMATMUL_LAST_USE(d, a, retained_then_killed);

  auto options = fixp::Options<FixpAttr::keep_acc()>{};
  TMATMUL_ACC_LAST_USE(d, c, a, acc_killed, options.raw_acc());
}

__attribute__((noinline)) void shared_shared_last_use(float *lhs, float *rhs) {
  Global global_a(lhs);
  Global global_b(rhs);
  SharedAHandle a;
  SharedBHandle b;
  Acc d;
  Acc c;
  TLOAD(a, global_a);
  TLOAD(b, global_b);
  auto options = fixp::Options<FixpAttr::keep_acc()>{};
  TMATMUL_ACC_LAST_USE(d, c, a, b, options, 16);
}

int main() { return 0; }
