#include <common/pto_tileop.hpp>
using namespace pto;
using Cell = VecTileM32<uint32_t, 32, 1>;
using Parent = VecTileM32<uint16_t, 32, 8>;
void pack_init(Cell &a, Cell &b, Parent &d) {
  auto dst = range::assemble<1, 0>(d);
  TPACK(dst, a, b, 0x0202u);
}
void pack_last(Cell &a, Cell &b, Parent &d) {
  auto dst = range::assemble_last<1, 3>(d);
  TPACK_ASS(dst, a, b, 0x0202u);
}
