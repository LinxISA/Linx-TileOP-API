#include <common/pto_tile_region_inline_asm.hpp>

#include <utility>

using namespace pto;

using Input = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;

__attribute__((noinline)) Parent assemble_unary_fragments(Input &input) {
  TileArray<Fragment, 1, 4> fragments;

  TABS(fragments[0][0], input);
  TEXP(fragments[0][1], input);
  TNEG(fragments[0][2], input);
  TRELU(fragments[0][3], input);

  return TASSEMBLY<Parent>(std::move(fragments));
}

int main() {
  Input input;
  Parent result = assemble_unary_fragments(input);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
