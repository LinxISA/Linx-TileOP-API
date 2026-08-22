#include <common/pto_tileop.hpp>

using namespace pto;

using Global = global_tensor<float, RowMajor<16, 16>>;
using SharedA = SharedMatrixLeft<float, 16, 16>;
using SharedB = SharedMatrixRight<float, 16, 16>;
using A = CubeTileM16<float, 16, 16>;
using B = CubeTileN8<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;

void shared_matmul_roundtrip(float *dst, float *lhs, float *rhs) {
  Global global_a(lhs);
  Global global_b(rhs);
  Global global_c(dst);
  SharedA shared_source_a;
  SharedB shared_source_b;
  SharedA restored_a;
  SharedB restored_b;
  A a;
  B b;
  C shared_result;
  C roundtrip_result;

  TLOAD(shared_source_a, global_a);
  TLOAD(shared_source_b, global_b);
  TLOAD_CUBE(a, global_a);
  TLOAD_CUBE(b, global_b);

  auto shared_a = TMOV_L2S_INSERT(shared_source_a);
  auto shared_b = TMOV_L2S_PUBLISH(shared_source_b);
  TMOV_S2L_BROADCAST(restored_a, shared_a);
  TMOV_S2L_EXTRACT(restored_b, shared_b);

  TMATMUL(shared_result, a, shared_b);
  TMATMUL(roundtrip_result, a, b);
  TADD(shared_result, shared_result, roundtrip_result);
  TSTORE(global_c, shared_result);
}

int main() { return 0; }
