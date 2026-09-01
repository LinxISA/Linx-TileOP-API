#include <common/pto_tileop.hpp>

#include <utility>

using namespace pto;

using Source = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Fragment = TileLeft<__bf16, 32, 16>;
using Parent = TileLeft<__bf16, 32, 64>;

__attribute__((noinline)) Parent build_probability_tile(Source &source) {
  TileArray<Fragment, 1, 4> probability_fragments;

  for (int col = 0; col < 4; ++col) {
    auto destination_slot = probability_fragments[0][col];
    TCVT(destination_slot, source);
  }

  return TASSEMBLY<Parent>(std::move(probability_fragments));
}

int main() {
  Source source;
  Parent result = build_probability_tile(source);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
