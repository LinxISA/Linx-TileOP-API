#include <common/pto_tile_region_inline_asm.hpp>
#include <utility>
using namespace pto;
using SourceParent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using SourceFragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using DestinationFragment = Tile<Location::Vec, __bf16, 32, 16, BLayout::RowMajor>;
using DestinationParent = Tile<Location::Vec, __bf16, 32, 64, BLayout::RowMajor>;
__attribute__((noinline)) DestinationParent convert_subviews(SourceParent &parent) {
  auto views = TPARTVIEW<SourceFragment, 1, 4>(parent);
  TileArray<DestinationFragment, 1, 4> output;
  auto source0 = views[0][0];
  auto source1 = views[0][1];
  auto source2 = views[0][2];
  auto source3 = views[0][3];
  TCVT(output[0][0], source0);
  TCVT(output[0][1], source1);
  TCVT(output[0][2], source2);
  TCVT(output[0][3], source3);
  return TASSEMBLY<DestinationParent>(std::move(output));
}
int main() { SourceParent p; auto out = convert_subviews(p); asm volatile("" : : "Tr"(out.data())); }
