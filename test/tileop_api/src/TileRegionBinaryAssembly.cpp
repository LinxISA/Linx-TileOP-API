#include <common/pto_tile_region_inline_asm.hpp>

#include <utility>

using namespace pto;

using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

__attribute__((noinline)) Parent assemble_binary_fragments(Parent &parent) {
  auto views = TPARTVIEW<Fragment, 1, 4>(parent);
  auto lhs = views[0][1];
  auto rhs = views[0][2];
  TileArray<Fragment, 1, 4> fragments;

  TADD(fragments[0][0], lhs, rhs);
  TSUB(fragments[0][1], lhs, rhs);
  TMAX(fragments[0][2], lhs, rhs);
  TMIN(fragments[0][3], lhs, rhs);

  return TASSEMBLY<Parent>(std::move(fragments));
}

int main() {
  Parent parent;
  Parent result = assemble_binary_fragments(parent);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
