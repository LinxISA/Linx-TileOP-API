#include <common/pto_tileop.hpp>

using namespace pto;

using Local = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using SharedLocal = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Shared = SharedTile<SharedLocal>;

__attribute__((noinline)) void shared_region_sources(Shared &parent,
                                                     Fragment &result) {
  auto fragments = TPARTVIEW<Fragment, 1, 4>(parent);
  auto source = fragments[0][1];
  TEXP(result, source);
}

int main() {
  Shared shared_parent;
  Fragment result;
  shared_region_sources(shared_parent, result);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
