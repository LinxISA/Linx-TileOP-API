#include <common/pto_tileop.hpp>

using namespace pto;

using StaticTile =
    Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;

void local_tmov(StaticTile &dst, const StaticTile &src) {
  TMOV(dst, src);
}

void local_tmov_alias(StaticTile &tile) {
  TMOV(tile, tile);
}

int main() { return 0; }