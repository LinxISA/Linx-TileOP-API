#include <common/pto_tile_region_inline_asm.hpp>
using namespace pto;
using Parent = VecTileM32<float, 32, 256>;
using Fragment = VecTileM32<float, 32, 32>;
using Result = VecTileM32<float, 32, 32>;

using BF16Parent = VecTileM32<__bf16, 32, 256>;
using BF16Fragment = VecTileM32<__bf16, 32, 32>;
using BF16Result = VecTileM32<__bf16, 32, 1>;
using BF16Scaled = VecTileM32<__bf16, 32, 32>;

__attribute__((noinline)) void reduce_part(Parent &parent, Result &dst, int index) {
  auto parts = TPARTVIEW<Fragment, 1, 8>(parent);
  auto source = parts[0][index];
  TROWMAX(dst, source);
}

__attribute__((noinline)) void reduce_min_prod_part(
    BF16Parent &parent, BF16Result &dst, int index) {
  auto parts = TPARTVIEW<BF16Fragment, 1, 8>(parent);
  auto source = parts[0][index];
  TROWMIN(dst, source);
  TROWPROD(dst, source);
}

__attribute__((noinline)) void expand_fragment_part(
    BF16Parent &parent, BF16Scaled &scaled, BF16Result &recip, int index) {
  auto parts = TPARTVIEW<BF16Fragment, 1, 8>(parent);
  auto source = parts[0][index];
  TROWEXPANDMUL(scaled, source, recip);
}
