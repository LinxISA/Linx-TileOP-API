#include <jcore/template_asm.hpp>
#include <common/pto_tileop.hpp>
#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using Parent = CubeTileM32<float, 32, 64>;
using Fragment = CubeTileM32<float, 32, 16>;
using Result = CubeTileM32<float, 32, 16>;
using Scale = CubeTileM32<float, 32, 1>;

__attribute__((noinline)) void row_expand_mixed(Parent &parent, Result &result,
                                                Scale &scale, int index) {
  auto fragments = TPARTVIEW<Fragment, 1, 4>(parent);
  auto source = fragments[0][index];
  TROWEXPANDMUL(result, source, scale);
}

int main() {
  Parent parent;
  Result result;
  Scale scale;
  row_expand_mixed(parent, result, scale, 0);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
