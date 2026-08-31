#include <common/pto_tileop.hpp>

using namespace pto;

using M16Fp16 = CubeTileM16<__half, 16, 12, 16, 9>;
using M16Fp32 = CubeTileM16<float, 16, 10, 16, 9>;
using M32Fp32 = CubeTileM32<float, 32, 9, 16, 9>;
using N8Fp16 = CubeTileN8<__half, 16, 8>;
using N8Fp32 = CubeTileN8<float, 16, 8>;

#if defined(SHOULD_FAIL_TCVT_CUBE_LAYOUT)
void mismatched_cube_layout(M32Fp32 &dst, M16Fp16 &src) {
  TCVT(dst, src);
}
#endif

#if defined(SHOULD_FAIL_TCVT_CUBE_VALID_SHAPE)
using BadValid = CubeTileM16<float, 16, 10, 16, 8>;
void mismatched_cube_valid_shape(BadValid &dst, M16Fp16 &src) {
  TCVT(dst, src);
}
#endif

#if defined(SHOULD_FAIL_TCVT_CUBE_N8)
void unsupported_cube_n8(N8Fp32 &dst, N8Fp16 &src) {
  TCVT(dst, src);
}
#endif

int main() { return 0; }
