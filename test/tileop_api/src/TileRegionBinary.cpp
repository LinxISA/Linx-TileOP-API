#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

__attribute__((noinline)) void binary_region_sources(Parent &parent,
                                                     Fragment &result) {
  auto fragments = TPARTVIEW<Fragment, 1, 4>(parent);
  auto lhs = fragments[0][1];
  auto rhs = fragments[0][2];

  TADD(result, lhs, rhs);
  TSUB(result, lhs, rhs);
  TMUL(result, lhs, rhs);
  TDIV(result, lhs, rhs);
  TREM(result, lhs, rhs);
  TAND(result, lhs, rhs);
  TOR(result, lhs, rhs);
  TXOR(result, lhs, rhs);
  TMAX(result, lhs, rhs);
  TMIN(result, lhs, rhs);
}

int main() {
  Parent parent;
  Fragment result;
  binary_region_sources(parent, result);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
