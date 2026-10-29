#include <common/pto_tileop.hpp>

using namespace pto;

using U32GM = global_tensor<uint32_t, RowMajor<8, 32>>;
using U32Row = Tile<Location::Vec, uint32_t, 8, 32, BLayout::RowMajor>;
using U32M32 = VecTileM32<uint32_t, 32, 8>;
using U32M16 = VecTileM16<uint32_t, 16, 16>;
using S64GM = global_tensor<int64_t, RowMajor<4, 4>>;
using S64Row = Tile<Location::Vec, int64_t, 4, 4, BLayout::RowMajor>;
using F32GM = global_tensor<float, RowMajor<8, 32>>;
using F32Row = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;
using SlackM32 = VecTileM32<uint32_t, 17, 3>;
using SlackM16 = VecTileM16<uint16_t, 15, 5>;

static_assert(U32M32::LogicalTileBytes == 1024);
static_assert(U32M32::PhysicalRows == 32 && U32M32::PhysicalCols == 8);
static_assert(U32M16::LogicalTileBytes == 1024);
static_assert(U32M16::PhysicalRows == 16 && U32M16::PhysicalCols == 16);
static_assert(SlackM32::LogicalTileBytes == 512);
static_assert(SlackM32::PhysicalRows == 32 && SlackM32::PhysicalCols == 4);
static_assert(SlackM32::CubeRequiredCellCount == 3 &&
              SlackM32::CubeCellCount == 4);
static_assert(SlackM16::LogicalTileBytes == 256);
static_assert(SlackM16::PhysicalRows == 16 && SlackM16::PhysicalCols == 8);

__attribute__((noinline)) void atomic_row(U32GM &gm, U32Row &src) {
  TSTORE_ATOMIC_ADD(gm, src);
  TSTORE_ATOMIC_AND(gm, src);
  TSTORE_ATOMIC_OR(gm, src);
  TSTORE_ATOMIC_XOR(gm, src);
  TSTORE_ATOMIC_INC(gm, src);
  TSTORE_ATOMIC_DEC(gm, src);
}

__attribute__((noinline)) void atomic_minmax(S64GM &gm, S64Row &src) {
  TSTORE_ATOMIC_MIN(gm, src);
  TSTORE_ATOMIC_MAX(gm, src);
}

__attribute__((noinline)) void atomic_float_add(F32GM &gm, F32Row &src) {
  TSTORE_ATOMIC_ADD(gm, src);
}

__attribute__((noinline)) void atomic_cube(U32GM &gm, U32M32 &m32,
                                           U32M16 &m16) {
  TSTORE_ATOMIC_ADD(gm, m32);
  TSTORE_ATOMIC_MAX(gm, m16);
}

__attribute__((noinline)) void atomic_cube_masked(U32GM &gm, U32M32 &source) {
  auto active = predicate::compare_scalar<CmpMode::GT>(source, 0u);
  TSTORE_ATOMIC_ADD<1>(gm, source, active);
  TSTORE_ATOMIC_MIN<2>(gm, source, active);
  TSTORE_ATOMIC_MAX<4, true>(gm, source, active);
  TSTORE_ATOMIC_AND<8>(gm, source, active);
  TSTORE_ATOMIC_OR<12, true>(gm, source, active);
  TSTORE_ATOMIC_XOR<14>(gm, source, active);
  TSTORE_ATOMIC_INC<15>(gm, source, active);
  TSTORE_ATOMIC_DEC<0>(gm, source, active);
}

__attribute__((always_inline)) inline void atomic_shared(U32GM &gm,
                                                         U32Row &local) {
  auto shared = TMOV_L2S_INSERT(local);
  TSTORE_ATOMIC_ADD<12>(gm, shared);
}

__attribute__((noinline)) void atomic_shared_form(U32GM &gm, U32Row &local) {
  atomic_shared(gm, local);
}
