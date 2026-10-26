#include <common/pto_tileop.hpp>

#include <cstdint>

using namespace pto;

using Fragment = VecTileM32<int32_t, 32, 1>;

#if defined(SHOULD_FAIL_ISSUE288_LAYOUT)
using BadSlot = Tile<Location::Vec, int32_t, 32, 1, BLayout::RowMajor>;
void bad_layout(Fragment &lhs, Fragment &rhs) {
  TileArray<BadSlot, 1, 2> slots;
  TADD(slots[0][0], lhs, rhs);
}
#elif defined(SHOULD_FAIL_ISSUE288_DTYPE)
using OtherDType = VecTileM32<uint32_t, 32, 1>;
void bad_dtype(Fragment &lhs, OtherDType &rhs) {
  TileArray<Fragment, 1, 2> slots;
  TADD(slots[0][0], lhs, rhs);
}
#elif defined(SHOULD_FAIL_ISSUE288_SHAPE)
using OtherShape = VecTileM32<int32_t, 32, 2>;
void bad_shape(Fragment &lhs, OtherShape &rhs) {
  TileArray<Fragment, 1, 2> slots;
  TADD(slots[0][0], lhs, rhs);
}
#elif defined(SHOULD_FAIL_ISSUE288_VALID_SHAPE)
using OtherValid = VecTileM32<int32_t, 32, 1, 16, 1>;
void bad_valid_shape(Fragment &lhs, OtherValid &rhs) {
  TileArray<Fragment, 1, 2> slots;
  TADD(slots[0][0], lhs, rhs);
}
#endif

int main() { return 0; }
