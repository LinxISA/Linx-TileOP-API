// Issue #241: both binary operands may be zero-copy reduction-prefix views.
#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using FloatReduction = VecTileM32<float, 32, 128, 32, 1>;
using FloatPrefix = VecTileM32<float, 32, 1, 32, 1>;
using FloatOutput = VecTileM32<float, 32, 1, 32, 1>;

__attribute__((noinline)) void prefix_prefix_arithmetic(
    FloatOutput &dst, FloatReduction &reduction0, FloatReduction &reduction1) {
  auto prefix0 = TREDUCEPREFIXVIEW<FloatPrefix>(reduction0);
  auto prefix1 = TREDUCEPREFIXVIEW<FloatPrefix>(reduction1);

  TADD(dst, prefix0, prefix1);
  TSUB(dst, prefix0, prefix1);
  TMUL(dst, prefix0, prefix1);
  TDIV(dst, prefix0, prefix1);
  TMAX(dst, prefix0, prefix1);
  TMIN(dst, prefix0, prefix1);
}

using IntegerReduction = VecTileM32<int32_t, 32, 128, 32, 1>;
using IntegerPrefix = VecTileM32<int32_t, 32, 1, 32, 1>;
using IntegerOutput = VecTileM32<int32_t, 32, 1, 32, 1>;

__attribute__((noinline)) void prefix_prefix_bitwise(
    IntegerOutput &dst, IntegerReduction &reduction0,
    IntegerReduction &reduction1) {
  auto prefix0 = TREDUCEPREFIXVIEW<IntegerPrefix>(reduction0);
  auto prefix1 = TREDUCEPREFIXVIEW<IntegerPrefix>(reduction1);

  TAND(dst, prefix0, prefix1);
  TOR(dst, prefix0, prefix1);
  TXOR(dst, prefix0, prefix1);
  TREM(dst, prefix0, prefix1);
}

int main() {
  FloatOutput float_dst;
  FloatReduction float_reduction0;
  FloatReduction float_reduction1;
  prefix_prefix_arithmetic(float_dst, float_reduction0, float_reduction1);

  IntegerOutput integer_dst;
  IntegerReduction integer_reduction0;
  IntegerReduction integer_reduction1;
  prefix_prefix_bitwise(integer_dst, integer_reduction0, integer_reduction1);

  asm volatile("" : : "Tr"(float_dst.data()), "Tr"(integer_dst.data()));
  return 0;
}