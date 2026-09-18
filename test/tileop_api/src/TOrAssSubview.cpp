#include <common/pto_tileop.hpp>

#include <cstdint>

using namespace pto;

using SourceParent = CubeTileM32<uint32_t, 32, 64>;
using SourceFragment = CubeTileM32<uint32_t, 32, 16>;
using Destination =
    Tile<Location::Vec, uint32_t, 32, 16, BLayout::RowMajor>;

// The assembly session is opened by the caller. TOR_ASS appends its result to
// the LAST slot while both TPARTVIEW sources retain their parent-relative
// ranges through B.SUBVIEW.
__attribute__((noinline)) void tor_ass_subview(SourceParent &lhs_parent,
                                               SourceParent &rhs_parent,
                                               Destination &destination) {
  auto lhs_parts = TPARTVIEW<SourceFragment, 1, 4>(lhs_parent);
  auto rhs_parts = TPARTVIEW<SourceFragment, 1, 4>(rhs_parent);
  auto lhs = lhs_parts[0][1];
  auto rhs = rhs_parts[0][2];
  auto assembled = range::assemble_last(destination);
  TOR_ASS(assembled, lhs, rhs);
}

__attribute__((noinline)) void tor_ass_explicit_range(SourceParent &lhs_parent,
                                                      SourceParent &rhs_parent,
                                                      Destination &destination) {
  auto lhs = range::subview<4, 4>(lhs_parent);
  auto rhs = range::subview<4, 8>(rhs_parent);
  auto assembled = range::assemble_last(destination);
  TOR_ASS(assembled, lhs, rhs);
}

int main() {
  SourceParent lhs;
  SourceParent rhs;
  Destination destination;
  tor_ass_subview(lhs, rhs, destination);
  tor_ass_explicit_range(lhs, rhs, destination);
  return 0;
}
