#include <common/pto_tileop.hpp>

using namespace pto;

using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Result = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

__attribute__((noinline)) void unary_region_sources(Parent &parent,
                                                    Result &result) {
  auto fragments = TPARTVIEW<Fragment, 1, 4>(parent);
  auto source = fragments[0][1];
  TABS(result, source);
  TNOT(result, source);
  TNEG(result, source);
  TEXP(result, source);
  TLOG(result, source);
  TRECIP(result, source);
  TSQRT(result, source);
  TRSQRT(result, source);
  TRELU(result, source);
}

int main() {
  Parent parent;
  Result result;
  unary_region_sources(parent, result);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
