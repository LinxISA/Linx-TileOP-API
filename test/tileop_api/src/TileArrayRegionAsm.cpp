#include <common/pto_tileop.hpp>

#include <utility>

using namespace pto;

using Input = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Fragment = TileLeft<__bf16, 32, 16>;
using Parent = TileLeft<__bf16, 32, 64>;

__attribute__((noinline)) Parent assemble_four_slots(Input &input) {
  TileArray<Fragment, 1, 4> fragments;

  TCVT(fragments[0][0], input);
  TCVT(fragments[0][1], input);
  TCVT(fragments[0][2], input);
  TCVT(fragments[0][3], input);

  return TASSEMBLY<Parent>(std::move(fragments));
}

using M16Parent = CubeTileM16<float, 16, 64>;
using M16Fragment = CubeTileM16<float, 16, 16>;
using M32Parent = CubeTileM32<float, 32, 64>;
using M32Fragment = CubeTileM32<float, 32, 16>;

static_assert(M16Parent::BFractal == BLayout::CubeM16);
static_assert(M16Fragment::BFractal == BLayout::CubeM16);
static_assert(M32Parent::BFractal == BLayout::CubeM32);
static_assert(M32Fragment::BFractal == BLayout::CubeM32);
static_assert(M16Parent::LogicalTileBytes ==
              4 * M16Fragment::LogicalTileBytes);
static_assert(M32Parent::LogicalTileBytes ==
              4 * M32Fragment::LogicalTileBytes);

int main() {
  Input input;
  Parent result = assemble_four_slots(input);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
