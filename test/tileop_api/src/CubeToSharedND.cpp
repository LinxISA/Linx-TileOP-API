#include <common/pto_tileop.hpp>

using namespace pto;

using CubeM16 = CubeTileM16<uint16_t, 16, 64>;
using SharedParentLocal =
    Tile<Location::Left, uint16_t, 64, 64, BLayout::RowMajor>;
using SharedParent = SharedTile<SharedParentLocal>;

static_assert(CubeM16::LogicalTileBytes == 2048);
static_assert(CubeM16::PhysicalRows == 16);
static_assert(CubeM16::PhysicalCols == 64);
static_assert(SharedParentLocal::LogicalTileBytes == 8192);

using CubeM32 = CubeTileM32<uint32_t, 32, 8>;
using SingletonNDLocal =
    Tile<Location::Left, uint32_t, 32, 8, BLayout::RowMajor>;
using SingletonND = SharedTile<SingletonNDLocal>;

// Keep the newly allocated Shared handle local to one compiler-managed SSA
// lifetime. SharedTile is not a normal cross-function object ABI.
__attribute__((noinline)) void cooperative_forms(CubeM16 &source,
                                                  uintptr_t row_offset) {
  SharedParent parent;
  auto init = range::assemble<16, 0>(parent, row_offset);
  TMOV_CUBE_TO_SHARED_ND(init, source);
  auto middle = range::assemble_middle<16, 16>(parent, row_offset);
  TMOV_CUBE_TO_SHARED_ND(middle, source);
  auto middle2 = range::assemble_middle<16, 32>(parent, row_offset);
  TMOV_CUBE_TO_SHARED_ND(middle2, source);
  auto last = range::assemble_last<16, 48>(parent, row_offset);
  TMOV_CUBE_TO_SHARED_ND(last, source);
}

__attribute__((noinline)) void singleton_form(CubeM32 &source) {
  SingletonND dst;
  TMOV_CUBE_TO_SHARED_ND<1>(dst, source);
}
