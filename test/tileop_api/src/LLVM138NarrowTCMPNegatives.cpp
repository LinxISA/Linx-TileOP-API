#include <common/pto_tileop.hpp>

using namespace pto;

#ifndef PHYSICAL_ROWS
#define PHYSICAL_ROWS 32
#endif

using Column = Tile<Location::Vec, int32_t, PHYSICAL_ROWS, 1,
                    BLayout::RowMajor, DYNAMIC, DYNAMIC>;

#if defined(SHOULD_FAIL_PHYSICAL_SHAPE)
using Other = Tile<Location::Vec, int32_t, PHYSICAL_ROWS / 16, 16,
                   BLayout::RowMajor, DYNAMIC, DYNAMIC>;
#elif defined(SHOULD_FAIL_LAYOUT)
using Other = Tile<Location::Vec, int32_t, PHYSICAL_ROWS, 1,
                   BLayout::ColMajor, DYNAMIC, DYNAMIC>;
#elif defined(SHOULD_FAIL_VALID_SHAPE)
using Other = Tile<Location::Vec, int32_t, PHYSICAL_ROWS, 1,
                   BLayout::RowMajor, DYNAMIC, 1>;
#else
#error Select a negative TCMP case
#endif

void incompatible(Column &dst, Column &left, Other &right) {
  TCMP<CmpMode::GT>(dst, left, right);
}
