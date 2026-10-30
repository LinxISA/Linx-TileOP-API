#include <common/pto_tileop.hpp>

#include <cstdint>

using namespace pto;

using Fragment = VecTileM32<int32_t, 32, 1>;
using Parent = VecTileM32<int32_t, 32, 8>;

template <typename Tile> void mixed_left(Parent &parent, Tile &tile) {
  auto view = TPARTVIEW<Fragment, 1, 8>(parent)[0][0];
  TileArray<Fragment, 1, 2> slots;
  TADD(slots[0][0], view, tile);
}

template <typename Tile> void mixed_right(Parent &parent, Tile &tile) {
  auto view = TPARTVIEW<Fragment, 1, 8>(parent)[0][0];
  TileArray<Fragment, 1, 2> slots;
  TADD(slots[0][0], tile, view);
}

#if defined(SHOULD_FAIL_ISSUE299_LAYOUT)
using Bad = Tile<Location::Vec, int32_t, 32, 1, BLayout::RowMajor>;
#elif defined(SHOULD_FAIL_ISSUE299_DTYPE)
using Bad = VecTileM32<uint32_t, 32, 1>;
#elif defined(SHOULD_FAIL_ISSUE299_SHAPE)
using Bad = VecTileM32<int32_t, 32, 2>;
#elif defined(SHOULD_FAIL_ISSUE299_VALID_SHAPE)
using Bad = VecTileM32<int32_t, 32, 1, 16, 1>;
#elif defined(SHOULD_FAIL_ISSUE299_LOCATION)
using Bad = CubeTileM32<int32_t, 32, 1>;
#elif defined(SHOULD_FAIL_ISSUE299_CAPACITY)
using Bad = VecTileM32<uint64_t, 32, 1>;
#endif

#if defined(SHOULD_FAIL_ISSUE299_LAYOUT) ||                                    \
    defined(SHOULD_FAIL_ISSUE299_DTYPE) ||                                     \
    defined(SHOULD_FAIL_ISSUE299_SHAPE) ||                                     \
    defined(SHOULD_FAIL_ISSUE299_VALID_SHAPE) ||                               \
    defined(SHOULD_FAIL_ISSUE299_LOCATION) ||                                  \
    defined(SHOULD_FAIL_ISSUE299_CAPACITY)
void reject_left(Parent &parent, Bad &bad) { mixed_left(parent, bad); }
void reject_right(Parent &parent, Bad &bad) { mixed_right(parent, bad); }
#endif

int main() { return 0; }
