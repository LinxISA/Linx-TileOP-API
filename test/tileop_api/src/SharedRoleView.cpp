// Shared operand-role views alias one published Shared handle.
#include <common/pto_tileop.hpp>
using namespace pto;
using SharedLeft = SharedTile<SharedMatrixLeft<float, 16, 16>>;
using SharedRight = SharedTile<SharedMatrixRight<float, 16, 16>>;
using LocalA = CubeTileM16<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
void shared_role_view(C &out, LocalA &dS_staged, SharedLeft &source,
                      SharedRight &q) {
  auto as_right = reinterpret_shared_tile<Location::Right>(source);
  // dQ = (dS^T)^T * K: the published K handle is consumed as B.
  TMATMUL(out, dS_staged, as_right);
  auto as_left = reinterpret_shared_tile<Location::Left>(source);
  // S^T = K * Q^T: the same published K handle is consumed as A.  Q remains
  // a separately published Shared-Right operand; no second K load is needed.
  TMATMUL(out, as_left, q, fixp::keep_acc().transpose_b());
}
int main() { return 0; }