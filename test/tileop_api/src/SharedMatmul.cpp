#include <common/pto_tileop.hpp>

using namespace pto;

using Global = global_tensor<float, RowMajor<16, 16>>;
using A = TileLeft<float, 16, 16>;
using B = TileRight<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

void shared_matmul_roundtrip(float *dst, float *lhs, float *rhs) {
  Global global_a(lhs);
  Global global_b(rhs);
  Global global_c(dst);
  A a;
  B b;
  A restored_a;
  B restored_b;
  C shared_result;
  C roundtrip_result;

  TLOAD(a, global_a);
  TLOAD(b, global_b);

  auto shared_a = TMOV_L2S_INSERT(a);
  auto shared_b = TMOV_L2S_PUBLISH(b);
  TMOV_S2L_BROADCAST(restored_a, shared_a);
  TMOV_S2L_EXTRACT(restored_b, shared_b);

  TMATMUL(shared_result, a, shared_b);
  TMATMUL(roundtrip_result, restored_a, restored_b);
  TADD(shared_result, shared_result, roundtrip_result);
  TSTORE(global_c, shared_result);
}

int main() { return 0; }
