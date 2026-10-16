#include <common/pto_tileop.hpp>
using namespace pto;
using Wide = VecTileM32<uint32_t, 32, 4>;
using Cell = VecTileM32<uint32_t, 32, 1>;
using ReduceWide = VecTileM32<__bf16, 32, 32, 32, 1>;
using PrefixCell = VecTileM32<__bf16, 32, 2, 32, 1>;
using PackedWord = VecTileM32<uint32_t, 32, 1, 32, 1>;
void pack_subviews(Wide &a, Wide &b, Cell &d) {
  auto av = TPARTVIEW<Cell, 1, 4>(a)[0][1];
  auto bv = TPARTVIEW<Cell, 1, 4>(b)[0][2];
  TPACK(d, av, bv, 0x0202u);
}
void pack_prefix_views(ReduceWide &a, ReduceWide &b, PackedWord &d) {
  auto av = TREDUCEPREFIXVIEW<PrefixCell>(a);
  auto bv = TREDUCEPREFIXVIEW<PrefixCell>(b);
  TPACK(d, av, bv, 0x0202u);
}
