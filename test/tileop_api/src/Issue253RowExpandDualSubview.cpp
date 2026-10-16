// Issue #253: the dual-SubTileView TROWEXPAND* overload takes a matrix fragment
// as src0 and a row-broadcast carrier fragment as src1.  Only src1 is a
// broadcast source, and each B.SUBVIEW carries its own fragment size.
#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using Score = CubeTileM32<__bf16, 32, 128>;  // four 32-column score groups
using Group = CubeTileM32<__bf16, 32, 32>;   // 2KB matrix fragment
using Maxima = VecTileM32<__bf16, 32, 4>;    // two packed GroupMax CELL pairs
using Pair = VecTileM32<__bf16, 32, 2>;      // 128B carrier, ValidCol == CubeCellCols

__attribute__((noinline)) void dual_view_slot0(
    Group &dst, Score &score, Maxima &maxima) {
  auto scores = TPARTVIEW<Group, 1, 4>(score);
  auto groups = TPARTVIEW<Pair, 1, 2>(maxima);
  auto src = scores[0][1];
  auto bcast = groups[0][0];

  TROWEXPANDADD<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
  TROWEXPANDSUB<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
  TROWEXPANDMUL<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
  TROWEXPANDDIV<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
  TROWEXPANDMAX<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
  TROWEXPANDMIN<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
  TROWEXPANDEXPDIF<Group, Score, Group, Maxima, Pair, 0>(dst, src, bcast);
}

// Byte offset 2 selects the second BF16 in the CELL.
__attribute__((noinline)) void dual_view_slot1(
    Group &dst, Score &score, Maxima &maxima) {
  auto scores = TPARTVIEW<Group, 1, 4>(score);
  auto groups = TPARTVIEW<Pair, 1, 2>(maxima);
  auto src = scores[0][1];
  auto bcast = groups[0][1];

  TROWEXPANDADD<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
  TROWEXPANDSUB<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
  TROWEXPANDMUL<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
  TROWEXPANDDIV<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
  TROWEXPANDMAX<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
  TROWEXPANDMIN<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
  TROWEXPANDEXPDIF<Group, Score, Group, Maxima, Pair, 2>(dst, src, bcast);
}

int main() { return 0; }
