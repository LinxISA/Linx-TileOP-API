// Negative fixtures for issue #251: the relaxed row-expand broadcast-source
// check accepts a one-column carrier or a packed CUBE CELL carrier
// (ValidCol <= CubeCellCols), but must still reject everything else.
#include <jcore/template_asm.hpp>
using namespace pto;

using M32Mat = VecTileM32<__half, 32, 32>;         // Score matrix
using M32Pair = VecTileM32<__half, 32, 2>;         // valid #251 carrier (ValidCol==2==CubeCellCols)
using M32Wide = VecTileM32<__half, 32, 3>;         // ValidCol==3 > CubeCellCols==2
using RowMulti = Tile<Location::Vec, __half, 32, 2, BLayout::RowMajor, 32, 2>;  // not CUBE, ValidCol==2

#ifdef SHOULD_FAIL_ROWEXPAND_ROWMAJOR_MULTI
// A RowMajor source cannot pack logical columns into a CELL, so ValidCol>1 is
// not a row-broadcast carrier even at offset 0.
void bad(M32Mat &d, M32Mat &s0, RowMulti &b) {
  TROWEXPANDEXPDIF<M32Mat, M32Mat, RowMulti, 0>(d, s0, b);
}
#elif defined(SHOULD_FAIL_ROWEXPAND_CUBE_EXCEEDS)
// ValidCol==3 exceeds the two logical columns a half CUBE_M32 CELL can carry.
void bad(M32Mat &d, M32Mat &s0, M32Wide &b) {
  TROWEXPANDEXPDIF<M32Mat, M32Mat, M32Wide, 0>(d, s0, b);
}
#elif defined(SHOULD_FAIL_ROWEXPAND_OFFSET_CELL)
// Byte offset 4 selects a third half within a two-column CELL (out of range).
void bad(M32Mat &d, M32Mat &s0, M32Pair &b) {
  TROWEXPANDEXPDIF<M32Mat, M32Mat, M32Pair, 4>(d, s0, b);
}
#endif

int main() { return 0; }
