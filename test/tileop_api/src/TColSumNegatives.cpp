#include <common/pto_tileop.hpp>

using namespace pto;

using Source = Tile<Location::Vec, uint32_t, 4, 8,
                    BLayout::RowMajor, 3, 7>;
using Result = Tile<Location::Vec, uint32_t, 4, 8,
                    BLayout::RowMajor, 1, 7>;

#ifdef SHOULD_FAIL_TCOLSUM_DEST_ROWS
using Bad = Tile<Location::Vec, uint32_t, 4, 8,
                 BLayout::RowMajor, 2, 7>;
void rejected(Bad &dst, Source &src) { TCOLSUM(dst, src); }
#elif defined(SHOULD_FAIL_TCOLSUM_DEST_COLS)
using Bad = Tile<Location::Vec, uint32_t, 4, 8,
                 BLayout::RowMajor, 1, 6>;
void rejected(Bad &dst, Source &src) { TCOLSUM(dst, src); }
#elif defined(SHOULD_FAIL_TCOLSUM_DTYPE)
using Bad = Tile<Location::Vec, int32_t, 4, 8,
                 BLayout::RowMajor, 1, 7>;
void rejected(Bad &dst, Source &src) { TCOLSUM(dst, src); }
#elif defined(SHOULD_FAIL_THISTOGRAM_RETIRED)
void rejected(Result &dst, Source &src, Source &index) {
  THISTOGRAM(dst, src, index, 0);
}
#endif

int main() { return 0; }
