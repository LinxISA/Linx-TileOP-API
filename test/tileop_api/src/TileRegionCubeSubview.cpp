#include <common/pto_tileop.hpp>

using namespace pto;

using Parent = CubeTileM32<float, 32, 64>;
using Fragment = CubeTileM32<float, 32, 16>;
using Result = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

__attribute__((noinline)) void cube_region_sources(Parent &parent,
                                                  Result &result) {
  auto fragments = TPARTVIEW<Fragment, 1, 4>(parent);
  auto source = fragments[0][1];
  TEXP(result, source);
}

int main() {
  Parent parent;
  Result result;
  cube_region_sources(parent, result);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
