#include <common/pto_tileop.hpp>

#include <utility>

using namespace pto;

// Eight 128B writers form one 1KB parent.  The writer extent and parent
// capacity are intentionally different contracts (PTO-ISA #265).
using Writer = CubeTileM32<float, 32, 1>;
using Parent = CubeTileM32<float, 32, 8>;
using WholeSource = Tile<Location::Vec, float, 32, 8, BLayout::RowMajor>;
static_assert(Writer::LogicalTileBytes == 128);
static_assert(Parent::LogicalTileBytes == 1024);

__attribute__((noinline)) Parent assemble_b128(Writer &source) {
  TileArray<Writer, 1, 8> writers;
  // TCVT has the TileArrayOutputRef overload and is deliberately used here
  // as the 128B writer.  The important contract under test is the writer
  // extent versus the assembled parent capacity, not the unary operation.
  TCVT(writers[0][0], source);
  TCVT(writers[0][1], source);
  TCVT(writers[0][2], source);
  TCVT(writers[0][3], source);
  TCVT(writers[0][4], source);
  TCVT(writers[0][5], source);
  TCVT(writers[0][6], source);
  TCVT(writers[0][7], source);
  return TASSEMBLY<Parent>(std::move(writers));
}

// Whole-parent form: the reduction consumes all 32 rows and 8 physical
// columns.  This is the form required when RMS Norm B128 covers the parent.
__attribute__((noinline)) void whole_parent_rowsum(Parent &parent) {
  using Result = Tile<Location::Vec, float, 32, 8, BLayout::RowMajor, 32, 1>;
  SubTileView<Parent, WholeSource> whole(parent, 0, 0, 1);
  Result result;
  TROWSUM(result, whole);
  asm volatile("" : : "Tr"(result.data()));
}

// Bounded-view form: the parent remains [32,8], while the source view has an
// explicit logical [8,8] valid rectangle.  B.SUBVIEW supplies the range; it
// does not silently truncate the parent geometry.
using Bounded = Tile<Location::Vec, float, 8, 8, BLayout::RowMajor, 8, 8>;
using BoundedResult = Tile<Location::Vec, float, 8, 8, BLayout::RowMajor, 8, 1>;

__attribute__((noinline)) void bounded_rowsum(Parent &parent) {
  SubTileView<Parent, Bounded> bounded(parent, 0, 0, 1);
  BoundedResult result;
  TROWSUM(result, bounded);
  asm volatile("" : : "Tr"(result.data()));
}

int main() {
  Writer source;
  Parent parent = assemble_b128(source);
  whole_parent_rowsum(parent);
  bounded_rowsum(parent);
  return 0;
}