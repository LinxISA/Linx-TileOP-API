#include <common/pto_tile_region_inline_asm.hpp>
using namespace pto;
using Parent = VecTileM32<float, 32, 256>;
using Fragment = VecTileM32<float, 32, 32>;
using Result = VecTileM32<float, 32, 32>;
__attribute__((noinline)) void reduce_part(Parent &parent, Result &dst, int index) {
  auto parts = TPARTVIEW<Fragment, 1, 8>(parent);
  auto source = parts[0][index];
  TROWMAX(dst, source);
}
