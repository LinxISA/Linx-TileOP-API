#include <common/pto_tileop.hpp>

using namespace pto;

using WordsM16 = CubeTileM16<uint32_t, 16, 16>;
using WordsM32 = CubeTileM32<uint32_t, 32, 32>;

#if defined(SHOULD_FAIL_PACK_LEFT_ZERO)
void zero_left_width(WordsM32 &dst, WordsM32 &a, WordsM32 &b) {
  TPACK(dst, a, b, 0x00000200);
}
#endif

#if defined(SHOULD_FAIL_PACK_LEFT_TOO_WIDE)
void oversized_left_width(WordsM32 &dst, WordsM32 &a, WordsM32 &b) {
  TPACK(dst, a, b, 0x00000204);
}
#endif

#if defined(SHOULD_FAIL_PACK_SUM_TOO_WIDE)
void combined_width_overflow(WordsM32 &dst, WordsM32 &a, WordsM32 &b) {
  TPACK(dst, a, b, 0x00000303);
}
#endif

#if defined(SHOULD_FAIL_PACK_HIGH_BITS)
void nonzero_high_control_bits(WordsM32 &dst, WordsM32 &a, WordsM32 &b) {
  TPACK(dst, a, b, 0x0000000100000202);
}
#endif

#if defined(SHOULD_FAIL_PACK_RIGHT_ZERO)
void zero_right_width(WordsM16 &dst, WordsM16 &a, WordsM16 &b) {
  TPACK(dst, a, b, 0x00000002);
}
#endif

#if defined(SHOULD_FAIL_UNPACK_OFFSET_TOO_LARGE)
void oversized_offset(WordsM32 &dst, WordsM32 &a) {
  TUNPACK(dst, a, 0x00000104);
}
#endif

#if defined(SHOULD_FAIL_UNPACK_COUNT_ZERO)
void zero_count(WordsM32 &dst, WordsM32 &a) {
  TUNPACK(dst, a, 0x00000001);
}
#endif

#if defined(SHOULD_FAIL_UNPACK_COUNT_TOO_LARGE)
void oversized_count(WordsM32 &dst, WordsM32 &a) {
  TUNPACK(dst, a, 0x00000500);
}
#endif

#if defined(SHOULD_FAIL_UNPACK_SUM_TOO_WIDE)
void offset_count_overflow(WordsM32 &dst, WordsM32 &a) {
  TUNPACK(dst, a, 0x00000402);
}
#endif

#if defined(SHOULD_FAIL_UNPACK_HIGH_BITS)
void nonzero_high_control_bits(WordsM16 &dst, WordsM16 &a) {
  TUNPACK(dst, a, 0x0000000100000201);
}
#endif

int main() { return 0; }
