#include <common/pto_tileop.hpp>

using namespace pto;

using SelectTile = Tile<Location::Vec, uint16_t, 16, 16,
                        BLayout::RowMajor>;

void tsel_canonical(SelectTile &dst, SelectTile &mask,
                    SelectTile &true_src) {
  TSEL(dst, mask, true_src);
}
