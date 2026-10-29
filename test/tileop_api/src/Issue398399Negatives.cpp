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

#elif defined(SHOULD_FAIL_ATOMIC_MASK_ROWMAJOR)
using GM = global_tensor<uint32_t, RowMajor<8, 32>>;
using Source = Tile<Location::Vec, uint32_t, 8, 32>;
using Basis = VecTileM32<uint32_t, 32, 8>;
void test(GM &gm, Source &source, Basis &basis) {
  auto active = predicate::compare_scalar<CmpMode::GT>(basis, 0u);
  TSTORE_ATOMIC_ADD(gm, source, active);
}

#elif defined(SHOULD_FAIL_ATOMIC_MASK_SHAPE)
using GM = global_tensor<uint32_t, RowMajor<32, 8>>;
using Source = VecTileM32<uint32_t, 32, 8>;
using Basis = VecTileM32<uint32_t, 16, 8>;
void test(GM &gm, Source &source, Basis &basis) {
  auto active = predicate::compare_scalar<CmpMode::GT>(basis, 0u);
  TSTORE_ATOMIC_ADD(gm, source, active);
}

#elif defined(SHOULD_FAIL_ATOMIC_MASK_SHARED)
using GM = global_tensor<uint32_t, RowMajor<32, 8>>;
using Local = Tile<Location::Left, uint32_t, 32, 8>;
using Basis = VecTileM32<uint32_t, 32, 8>;
void test(GM &gm, SharedTile<Local> &source, Basis &basis) {
  auto active = predicate::compare_scalar<CmpMode::GT>(basis, 0u);
  TSTORE_ATOMIC_ADD(gm, source, active);
}

#elif defined(SHOULD_FAIL_ATOMIC_PE_MASK)
using GM = global_tensor<uint32_t, RowMajor<32, 8>>;
using Source = VecTileM32<uint32_t, 32, 8>;
void test(GM &gm, Source &source) { TSTORE_ATOMIC_ADD<3>(gm, source); }

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
