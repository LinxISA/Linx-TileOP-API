#include <common/pto_tile_region_inline_asm.hpp>
#include <utility>
using namespace pto;
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
__attribute__((noinline)) Parent unary_subview_assemble(Parent &parent) {
  auto views = TPARTVIEW<Fragment, 1, 4>(parent);
  auto source = views[0][1];
  TileArray<Fragment, 1, 4> output;
  TABS(output[0][0], source);
  TEXP(output[0][1], source);
  TNEG(output[0][2], source);
  TRELU(output[0][3], source);
  return TASSEMBLY<Parent>(std::move(output));
}
int main() { Parent p; auto out = unary_subview_assemble(p); asm volatile("" : : "Tr"(out.data())); }
