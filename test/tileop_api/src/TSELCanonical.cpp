#include <common/pto_tileop.hpp>

using namespace pto;

using StaticSelectTile =
    Tile<Location::Vec, uint16_t, 16, 16, BLayout::RowMajor>;
using DynamicSelectTile =
    Tile<Location::Vec, uint16_t, 16, 16, BLayout::RowMajor, -1, -1>;

__attribute__((noinline)) void tsel_static(StaticSelectTile &dst,
                                           StaticSelectTile &mask,
                                           StaticSelectTile &true_src) {
  TSEL(dst, mask, true_src);
}

__attribute__((noinline)) void tsel_dynamic(DynamicSelectTile &dst,
                                            DynamicSelectTile &mask,
                                            DynamicSelectTile &true_src) {
  TSEL(dst, mask, true_src);
}
