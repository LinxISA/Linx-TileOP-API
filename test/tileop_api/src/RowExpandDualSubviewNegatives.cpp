// Negative fixtures for issue #253: in the dual-SubTileView row-expand
// overload only src1 is a broadcast source (one-column or packed CUBE CELL
// carrier), and the matrix fragment src0 must match the destination columns.
#include <common/pto_tile_region_inline_asm.hpp>
using namespace pto;

using Score = CubeTileM32<__bf16, 32, 128>;    // matrix parent
using Group = CubeTileM32<__bf16, 32, 32>;     // matrix fragment
using Narrow = CubeTileM32<__bf16, 32, 16>;    // matrix fragment narrower than dst
using Maxima = VecTileM32<__bf16, 32, 4>;      // carrier parent, ValidCol==4
using Pair = VecTileM32<__bf16, 32, 2>;        // valid carrier fragment (ValidCol==2==CubeCellCols)

#ifdef SHOULD_FAIL_ROWEXPAND_VIEW_CUBE_EXCEEDS
// A four-column carrier fragment spans two BF16 CUBE_M32 CELLs.
void bad(Group &d, Score &score, Maxima &maxima) {
  auto scores = TPARTVIEW<Group, 1, 4>(score);
  auto wide = TPARTVIEW<Maxima, 1, 1>(maxima);
  auto src = scores[0][1];
  auto bcast = wide[0][0];
  TROWEXPANDEXPDIF<Group, Score, Group, Maxima, Maxima, 0>(d, src, bcast);
}
#elif defined(SHOULD_FAIL_ROWEXPAND_VIEW_OFFSET_CELL)
// Byte offset 4 selects a third BF16 within a two-column CELL (out of range).
void bad(Group &d, Score &score, Maxima &maxima) {
  auto scores = TPARTVIEW<Group, 1, 4>(score);
  auto groups = TPARTVIEW<Pair, 1, 2>(maxima);
  auto src = scores[0][1];
  auto bcast = groups[0][0];
  TROWEXPANDEXPDIF<Group, Score, Group, Maxima, Pair, 4>(d, src, bcast);
}
#elif defined(SHOULD_FAIL_ROWEXPAND_VIEW_MATRIX_COLS)
// The matrix fragment has 16 valid columns but the destination has 32.
void bad(Group &d, Score &score, Maxima &maxima) {
  auto scores = TPARTVIEW<Narrow, 1, 8>(score);
  auto groups = TPARTVIEW<Pair, 1, 2>(maxima);
  auto src = scores[0][1];
  auto bcast = groups[0][0];
  TROWEXPANDEXPDIF<Group, Score, Narrow, Maxima, Pair, 0>(d, src, bcast);
}
#endif

int main() { return 0; }
