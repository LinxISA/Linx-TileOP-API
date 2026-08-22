#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM16<float, 1, 32>;
using A = CubeTileM16<__half, 1, 64>;
using B = CubeTileN8<__bf16, 64, 32>;

// Mixed A/B dtypes make a reversed PTO_MATMUL_COMMON_INPUTS order visible:
// BSTART must carry A=FP16 while B.DATR carries B=BF16, and B.IOT binds A
// before B.
void mixed_dtype_tgemv(D &d, B &matrix_b, A &vector_a) {
  TGEMV(d, matrix_b, vector_a);
}
