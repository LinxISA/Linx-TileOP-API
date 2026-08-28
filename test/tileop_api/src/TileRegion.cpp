// Experimental compiler-semantic Tile region/session surface.
// This test intentionally exercises only the non-lowering type contract.
#include <common/pto_tileop.hpp>

using namespace pto;

using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

__attribute__((noinline)) void tile_region_syntax(Parent &parent, int index) {
  auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
  SubTileView<Parent, Fragment> source = parts[0][index];

  TileArray<Fragment, 1, 4> destinations;
  TileArrayOutputRef<Fragment> destination = destinations[0][index];

  (void)source;
  (void)destination;
}

int main() {
  Parent parent;
  tile_region_syntax(parent, 0);
  return 0;
}
