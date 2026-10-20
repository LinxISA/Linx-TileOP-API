#include <common/pto_tileop.hpp>

// Negative TLEA fixtures: each -DSHOULD_FAIL_TLEA_<case> must be rejected at
// compile time by the intended static_assert (or concept), not by an
// unrelated parse/toolchain error. Driven by test/tileop_api/run_negatives.sh.

using namespace pto;

template <typename T, int Rows = 8, int Cols = 32,
          BLayout L = BLayout::RowMajor>
using Vec = Tile<Location::Vec, T, Rows, Cols, L>;

#if defined(SHOULD_FAIL_TLEA_BAD_DTYPE)
// FP32 is not an S32/U32/S64/U64 element index.
void reject() { Vec<float> idx; Vec<int64_t> off; TLEA(off, idx, 32u); }
#elif defined(SHOULD_FAIL_TLEA_DEST_32BIT)
// A 32-bit destination cannot hold a 64-bit byte offset.
void reject() { Vec<int32_t> idx; Vec<int32_t> off; TLEA(off, idx, 32u); }
#elif defined(SHOULD_FAIL_TLEA_SIGNED_TO_U64)
// Signed source must pair with an S64 destination, not U64.
void reject() { Vec<int32_t> idx; Vec<uint64_t> off; TLEA(off, idx, 32u); }
#elif defined(SHOULD_FAIL_TLEA_UNSIGNED_TO_S64)
// Unsigned source must pair with a U64 destination, not S64.
void reject() { Vec<uint32_t> idx; Vec<int64_t> off; TLEA(off, idx, 32u); }
#elif defined(SHOULD_FAIL_TLEA_SHAPE_COL)
// Source and destination must share the same logical ValidCol.
void reject() {
  Vec<int32_t, 8, 32> idx;
  Tile<Location::Vec, int64_t, 8, 16, BLayout::RowMajor> off;
  TLEA(off, idx, 32u);
}
#elif defined(SHOULD_FAIL_TLEA_SHAPE_ROW)
// Source and destination must share the same logical ValidRow.
void reject() {
  Vec<int32_t, 8, 32> idx;
  Tile<Location::Vec, int64_t, 4, 32, BLayout::RowMajor> off;
  TLEA(off, idx, 32u);
}
#elif defined(SHOULD_FAIL_TLEA_CUBE_M32)
// CUBE_M32 is a legal ISA layout but is not yet implemented by this wrapper.
void reject() {
  Tile<Location::Vec, int32_t, 32, 32, BLayout::CubeM32> idx, off;
  TLEA(off, idx, 32u);
}
#elif defined(SHOULD_FAIL_TLEA_LOCATION)
// Source and destination must be persistent Local (Vec) tiles.
void reject() {
  Tile<Location::Left, int32_t, 8, 32, BLayout::RowMajor> idx;
  Vec<int64_t> off;
  TLEA(off, idx, 32u);
}
#endif

int main() { return 0; }
