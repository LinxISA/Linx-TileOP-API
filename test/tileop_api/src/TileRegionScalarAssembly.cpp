#include <common/pto_tile_region_inline_asm.hpp>

#include <utility>

using namespace pto;

using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

__attribute__((noinline)) Parent scalar_assemble(Parent &parent, float scalar) {
  auto views = TPARTVIEW<Fragment, 1, 4>(parent);
  auto source = views[0][1];
  Fragment scratch;

  TADDS(scratch, source, scalar);
  TSUBS(scratch, source, scalar);
  TMULS(scratch, source, scalar);
  TDIVS(scratch, source, scalar);
  TMAXS(scratch, source, scalar);
  TMINS(scratch, source, scalar);

  TileArray<Fragment, 1, 4> output;
  TADDS(output[0][0], source, scalar);
  TSUBS(output[0][1], source, scalar);
  TMULS(output[0][2], source, scalar);
  TMAXS(output[0][3], source, scalar);

  TileArray<Fragment, 1, 1> division_output;
  TDIVS(division_output[0][0], source, scalar);
  Fragment division = TASSEMBLY<Fragment>(std::move(division_output));

  TileArray<Fragment, 1, 1> minimum_output;
  TMINS(minimum_output[0][0], source, scalar);
  Fragment minimum = TASSEMBLY<Fragment>(std::move(minimum_output));

  asm volatile("" : : "Tr"(scratch.data()), "Tr"(division.data()),
               "Tr"(minimum.data()));
  return TASSEMBLY<Parent>(std::move(output));
}

int main() {
  Parent parent;
  Parent result = scalar_assemble(parent, 1.0f);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
