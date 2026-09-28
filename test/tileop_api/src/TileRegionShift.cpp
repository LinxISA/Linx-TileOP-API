#include <common/pto_tileop.hpp>
#include <common/pto_tile_region_inline_asm.hpp>

#include <utility>

using namespace pto;

using SourceParent = CubeTileM32<int32_t, 32, 64>;
using SourceFragment = CubeTileM32<int32_t, 32, 16>;
using Result = Tile<Location::Vec, int32_t, 32, 16, BLayout::RowMajor>;
using ResultParent = Tile<Location::Vec, int32_t, 32, 64, BLayout::RowMajor>;

__attribute__((noinline)) void shift_subview(SourceParent &parent,
                                             SourceFragment &amount,
                                             Result &result) {
  auto views = TPARTVIEW<SourceFragment, 1, 4>(parent);
  auto source = views[0][1];

  TSHL(result, source, amount);
  TSHR(result, source, amount);
}

__attribute__((noinline)) ResultParent assemble_shift_subviews(
    SourceParent &parent, SourceParent &amount_parent) {
  auto views = TPARTVIEW<SourceFragment, 1, 4>(parent);
  auto amount_views = TPARTVIEW<SourceFragment, 1, 4>(amount_parent);
  TileArray<Result, 1, 4> output;
  auto amount0 = amount_views[0][0];
  auto amount1 = amount_views[0][1];
  auto amount2 = amount_views[0][2];
  auto amount3 = amount_views[0][3];
  auto source0 = views[0][0];
  auto source1 = views[0][1];
  auto source2 = views[0][2];
  auto source3 = views[0][3];

  // The assemble shift overload consumes CUBE sources but writes RowMajor
  // slots, matching the B.ASSEMBLE contract used by the other binary ops.
  TSHL(output[0][0], source0, amount0);
  TSHR(output[0][1], source1, amount1);
  TSHL(output[0][2], source2, amount2);
  TSHR(output[0][3], source3, amount3);

  return TASSEMBLY<ResultParent>(std::move(output));
}

int main() {
  SourceParent parent;
  SourceParent amount_parent;
  SourceFragment amount;
  Result result;
  shift_subview(parent, amount, result);
  auto assembled = assemble_shift_subviews(parent, amount_parent);
  asm volatile("" : : "Tr"(result.data()), "Tr"(assembled.data()));
  return 0;
}