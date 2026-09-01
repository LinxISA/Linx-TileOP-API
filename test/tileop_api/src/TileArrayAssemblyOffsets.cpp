#include <common/pto_tileop.hpp>
#include <utility>

using namespace pto;

using Input = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Fragment = TileLeft<__bf16, 32, 16>;
using Parent = TileLeft<__bf16, 32, 64>;

void assemble_all_slots(Input &input) {
  TileArray<Fragment, 1, 4> fragments;
  TCVT(fragments[0][0], input);
  TCVT(fragments[0][1], input);
  TCVT(fragments[0][2], input);
  TCVT(fragments[0][3], input);
  Parent result = TASSEMBLY<Parent>(std::move(fragments));
  asm volatile("" : : "Tr"(result.data()));
}

int main() { return 0; }
