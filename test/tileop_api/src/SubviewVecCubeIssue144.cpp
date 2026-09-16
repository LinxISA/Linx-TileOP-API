#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using Parent = VecTileM32<__bf16, 32, 256>;
using Fragment = VecTileM32<__bf16, 32, 32>;
using RowOut = VecTileM32<__bf16, 32, 1>;
using Scaled = VecTileM32<__bf16, 32, 32>;

__attribute__((noinline)) void
reduce_fragment(Parent &parent, RowOut &dst, int index) {
  auto parts = TPARTVIEW<Fragment, 1, 8>(parent);
  auto fragment = parts[0][index];
  TROWMAX(dst, fragment);
  TROWMIN(dst, fragment);
  TROWPROD(dst, fragment);
}

__attribute__((noinline)) void
scale_fragment(Parent &parent, Scaled &scaled, RowOut &recip, int index) {
  auto parts = TPARTVIEW<Fragment, 1, 8>(parent);
  auto fragment = parts[0][index];
  TROWEXPANDMUL(scaled, fragment, recip);
}
