#include <common/pto_tileop.hpp>

using namespace pto;

using A = SharedMatrixLeft<float, 16, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using D = CubeAccumulatorM16<float, 16, 16>;

void shared_transpose(D &d, A &a, B &b) {
  auto shared_a = TMOV_L2S_INSERT(a);
  auto shared_b = TMOV_L2S_INSERT(b);
  TMATMUL(d, shared_a, shared_b,
          fixp::keep_acc().transpose_a().transpose_b());
}
