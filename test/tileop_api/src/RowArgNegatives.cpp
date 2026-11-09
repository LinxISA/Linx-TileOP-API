#include <common/pto_tileop.hpp>

using namespace pto;

using Source = Tile<Location::Vec, __half, 16, 16,
                    BLayout::RowMajor, 16, 16>;
using BadDestination = Tile<Location::Vec, __half, 16, 1,
                            BLayout::RowMajor, 16, 1>;

#if defined(SHOULD_FAIL_TROWARGMAX_NON_U32)
void bad_max(BadDestination &dst, Source &src) { TROWARGMAX(dst, src); }
#endif

#if defined(SHOULD_FAIL_TROWARGMIN_NON_U32)
void bad_min(BadDestination &dst, Source &src) { TROWARGMIN(dst, src); }
#endif

int main() { return 0; }
