#include <common/pto_tileop.hpp>

using namespace pto;

using A = TileLeft<float, 16, 16>;
using B = TileRight<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using ScaleA = TileLeft<float, 16, 16>;
using ScaleB = TileRight<float, 16, 16>;

void shared_matmul_all(C &dst, C &extra, A &a, B &b, ScaleA &scale_a,
                       ScaleB &scale_b) {
  auto shared_a = TMOV_L2S_PUBLISH(a);
  auto shared_b = TMOV_L2S_PUBLISH(b);

  TMATMUL(dst, shared_a, shared_b);
  TMATMUL_ACC(dst, extra, shared_a, shared_b, fixp::keep_acc());
  TMATMUL_FIXP(dst, shared_a, shared_b);
  TMATMUL_FIXP(dst, shared_a, shared_b, fixp::keep_acc());
  TMATMUL_BIAS(dst, shared_a, shared_b, extra, fixp::keep_acc());
  TMATMUL_MX(dst, shared_a, scale_a, shared_b, scale_b, fixp::keep_acc());
  TMATMUL_MX_ACC(dst, extra, shared_a, scale_a, shared_b, scale_b, fixp::keep_acc());
  TMATMUL_MX_BIAS(dst, shared_a, scale_a, shared_b, scale_b, extra, fixp::keep_acc());
}

int main() { return 0; }
