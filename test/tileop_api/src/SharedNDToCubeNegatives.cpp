#include <common/pto_tileop.hpp>
using namespace pto;

#if defined(SHOULD_FAIL_423_DTYPE)
using Cube = VecTileM32<uint32_t, 32, 8>;
using ND = Tile<Location::Vec, float, 32, 8>;
#elif defined(SHOULD_FAIL_423_COLUMNS)
using Cube = VecTileM32<uint8_t, 32, 128>;
using ND = Tile<Location::Vec, uint8_t, 32, 256>;
#elif defined(SHOULD_FAIL_423_PARENT)
using Cube = VecTileM32<uint8_t, 32, 128>;
using ND = Tile<Location::Vec, uint8_t, 64, 128>;
#elif defined(SHOULD_FAIL_423_N8)
using Cube = Tile<Location::Vec, uint32_t, 4, 8, BLayout::CubeN8>;
using ND = Tile<Location::Vec, uint32_t, 4, 8>;
#elif defined(SHOULD_FAIL_423_LAYOUT)
using Cube = VecTileM32<uint32_t, 32, 8>;
using ND = VecTileM32<uint32_t, 32, 8>;
#elif defined(SHOULD_FAIL_423_RCPE)
using Cube = VecTileM32<__fp8_rcpe6m2, 32, 128>;
using ND = Tile<Location::Vec, __fp8_rcpe6m2, 32, 128>;
#elif defined(SHOULD_FAIL_423_M16_B64)
using Cube = VecTileM16<uint64_t, 16, 8>;
using ND = Tile<Location::Vec, uint64_t, 16, 8>;
#else
using Cube = VecTileM32<uint8_t, 32, 256>;
using ND = Tile<Location::Vec, uint8_t, 64, 256>;
#endif

void test(Cube &dst, SharedTile<ND> &src) {
#if defined(SHOULD_FAIL_423_OFFSET)
  auto view = range::shared_nd_subview<64, 1>(src);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_RANGE)
  auto view = range::shared_nd_subview<64, 96>(src);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_LENGTH)
  auto view = range::shared_nd_subview<32>(src);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_SIZE_CODE)
  auto view = range::shared_nd_subview<3>(src);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_MASK)
  auto view = range::shared_nd_subview<64>(src);
  TMOV_SHARED_ND_TO_CUBE<16>(dst, view);
#elif defined(SHOULD_FAIL_423_ASSEMBLE)
  auto writer = range::assemble<64>(dst);
  TMOV_SHARED_ND_TO_CUBE(writer, src);
#elif defined(SHOULD_FAIL_423_SLOT)
  SharedTileSlot<0, ND> slot;
  auto view = range::shared_nd_subview<64>(slot);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_OLD_SUBVIEW)
  auto view = range::subview<64>(src);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_TEMPORARY)
  auto view = range::shared_nd_subview<64>(SharedTile<ND>{});
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#elif defined(SHOULD_FAIL_423_DTYPE) || defined(SHOULD_FAIL_423_COLUMNS) || \
      defined(SHOULD_FAIL_423_PARENT) || defined(SHOULD_FAIL_423_N8) || \
      defined(SHOULD_FAIL_423_LAYOUT) || defined(SHOULD_FAIL_423_RCPE) || \
      defined(SHOULD_FAIL_423_M16_B64)
  TMOV_SHARED_ND_TO_CUBE(dst, src);
#else
  auto view = range::shared_nd_subview<64>(src);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
#endif
}
