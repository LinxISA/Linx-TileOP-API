#include <common/pto_tileop.hpp>

// Positive TLEA fixtures (PTO-TILE-TLEA, selector 0x02E, VEC engine).
// Compiled with the Linx target compiler by test/tileop_api/compile.all.
// TLEA extends Local S32/U32/S64/U64 element indices to 64-bit byte offsets
// scaled by element_bits/8, writing a new Local S64 (signed) or U64 (unsigned)
// destination with the same logical valid shape and independent physical
// geometry. The wrapper emits the RowMajor form.

using namespace pto;

template <typename T, int Rows = 8, int Cols = 32>
using Vec = Tile<Location::Vec, T, Rows, Cols, BLayout::RowMajor>;

// Signed source -> S64 byte-offset destination (4-byte element stride).
void tlea_s32_to_s64(Vec<int32_t> &idx, Vec<int64_t> &off) {
  TLEA(off, idx, 32u);
}

// Unsigned source -> U64 byte-offset destination.
void tlea_u32_to_u64(Vec<uint32_t> &idx, Vec<uint64_t> &off) {
  TLEA(off, idx, 32u);
}

// 64-bit index forms, 8-byte element stride.
void tlea_s64_to_s64(Vec<int64_t> &idx, Vec<int64_t> &off) {
  TLEA(off, idx, 64u);
}

void tlea_u64_to_u64(Vec<uint64_t> &idx, Vec<uint64_t> &off) {
  TLEA(off, idx, 64u);
}

// Independent physical capacity: source and destination share one logical
// valid shape (8 x 32) while the destination has a wider physical column.
void tlea_independent_capacity(
    Vec<int32_t, 8, 32> &idx,
    Tile<Location::Vec, int64_t, 8, 64, BLayout::RowMajor, 8, 32> &off) {
  TLEA(off, idx, 16u);
}

// Dynamic valid shape on both operands (runtime ValidCol/ValidRow).
void tlea_dynamic_shape(
    Tile<Location::Vec, int32_t, 8, 32, BLayout::RowMajor, -1, -1> &idx,
    Tile<Location::Vec, int64_t, 8, 32, BLayout::RowMajor, -1, -1> &off) {
  TLEA(off, idx, 32u);
}

int main() { return 0; }
