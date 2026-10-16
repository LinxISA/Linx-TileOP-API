#include <common/pto_tileop.hpp>
using namespace pto;
using Cell = VecTileM32<uint32_t, 32, 1>;
using Parent = VecTileM32<uint16_t, 32, 8>;
void pack_assemble(Cell &a, Cell &b, Parent &d) {
  Parent assembled;
  auto init = range::assemble<1, 0>(assembled);
  TPACK(init, a, b, 0x0202u);
  auto last = range::assemble_last<1, 3>(assembled);
  TPACK_ASS(last, a, b, 0x0202u);
  d = assembled;
}
