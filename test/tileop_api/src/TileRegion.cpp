#include <common/pto_tileop.hpp>
#include <utility>

using namespace pto;

using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using RowState =
    Tile<Location::Vec, float, 32, 8, BLayout::RowMajor, 32, 1>;
using ProbabilityFragment = TileLeft<__bf16, 32, 16>;
using ProbabilityParent = TileLeft<__bf16, 32, 64>;

__attribute__((noinline)) ProbabilityParent
tile_region_inline_asm(Parent &parent, Fragment &normalized, int index) {
  auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
  SubTileView<Parent, Fragment> source = parts[0][index];

  Fragment scaled;
  RowState row_max;
  TMULS(scaled, source, 0.125f);
  TROWMAX(row_max, source);

  TileArray<ProbabilityFragment, 1, 4> destinations;
  TileArrayOutputRef<ProbabilityFragment> destination =
      destinations[0][index];
  TCVT(destination, normalized);
  return TASSEMBLY<ProbabilityParent>(std::move(destinations));
}

int main() {
  Parent parent;
  Fragment normalized;
  auto probability = tile_region_inline_asm(parent, normalized, 0);
  asm volatile("" : : "Tr"(probability.data()));
  return 0;
}
