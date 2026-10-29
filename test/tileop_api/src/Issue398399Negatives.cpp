#include <common/pto_tileop.hpp>

using namespace pto;

#if defined(SHOULD_FAIL_ATOMIC_ADD_DTYPE)
using GM = global_tensor<uint16_t, RowMajor<8, 8>>;
using Source = Tile<Location::Vec, uint16_t, 8, 8>;
void test(GM &gm, Source &source) { TSTORE_ATOMIC_ADD(gm, source); }

#elif defined(SHOULD_FAIL_ATOMIC_INC_DTYPE)
using GM = global_tensor<uint64_t, RowMajor<4, 4>>;
using Source = Tile<Location::Vec, uint64_t, 4, 4>;
void test(GM &gm, Source &source) { TSTORE_ATOMIC_INC(gm, source); }

#elif defined(SHOULD_FAIL_ATOMIC_CUBE_N8)
using GM = global_tensor<uint32_t, RowMajor<4, 8>>;
using Source = Tile<Location::Vec, uint32_t, 4, 8, BLayout::CubeN8>;
void test(GM &gm, Source &source) { TSTORE_ATOMIC_ADD(gm, source); }

#elif defined(SHOULD_FAIL_CUBE_SHARED_DTYPE)
using Cube = CubeTileM32<uint32_t, 32, 8>;
using ND = Tile<Location::Left, uint16_t, 32, 16, BLayout::RowMajor>;
void test(SharedTile<ND> &dst, Cube &source) {
  TMOV_CUBE_TO_SHARED_ND<1>(dst, source);
}

#elif defined(SHOULD_FAIL_CUBE_SHARED_COLS)
using Cube = CubeTileM32<uint32_t, 32, 8>;
using ND = Tile<Location::Left, uint32_t, 16, 16, BLayout::RowMajor>;
void test(SharedTile<ND> &dst, Cube &source) {
  TMOV_CUBE_TO_SHARED_ND<1>(dst, source);
}

#elif defined(SHOULD_FAIL_CUBE_SHARED_CAPACITY)
using Cube = CubeTileM32<uint32_t, 32, 8>;
using ND = Tile<Location::Left, uint32_t, 64, 8, BLayout::RowMajor>;
void test(SharedTile<ND> &dst, Cube &source) {
  TMOV_CUBE_TO_SHARED_ND<1>(dst, source);
}

#elif defined(SHOULD_FAIL_CUBE_SHARED_WRITER_SIZE)
using Cube = CubeTileM16<uint16_t, 16, 64>;
using ND = Tile<Location::Left, uint16_t, 64, 64, BLayout::RowMajor>;
void test(SharedTile<ND> &dst, Cube &source) {
  auto writer = range::assemble<8, 0>(dst);
  TMOV_CUBE_TO_SHARED_ND(writer, source);
}

#else
using Cube = CubeTileM32<uint32_t, 32, 8>;
using ND = Tile<Location::Left, uint32_t, 32, 8, BLayout::RowMajor>;
static_assert(Cube::LogicalTileBytes == ND::LogicalTileBytes);
#endif
