#include <common/pto_tileop.hpp>

using namespace pto;

using WideF = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, 16, 1>;
using WideP = Tile<Location::Vec, int32_t, 16, 16, BLayout::RowMajor, 16, 1>;

void valid_partial_shapes(WideP &predicate, WideF &left, WideF &right,
                          WideF &dst) {
  TCMP<CmpMode::GT>(predicate, left, right);
  TSEL(dst, predicate, left);
}
