// reinterpret_tile: zero-instruction equal-bit-width dtype reinterpret view.
// Positive cases bind the source register unchanged; negatives (width
// mismatch, Shared source, temporary binding) are compile-time rejected and
// driven from run_negatives-style -DNEG_CASE=N invocations.
#include <common/pto_tileop.hpp>

using namespace pto;

using F32T = Tile<Location::Vec, float, 4, 16, BLayout::RowMajor>;
using U32T = Tile<Location::Vec, uint32_t, 4, 16, BLayout::RowMajor>;
using BF16T = Tile<Location::Vec, __bf16, 4, 16, BLayout::RowMajor>;
using U16T = Tile<Location::Vec, uint16_t, 4, 16, BLayout::RowMajor>;
using SharedU32 = SharedTile<U32T>;

// The view forwards the source register: the consuming operation binds the
// source Tile storage with the reinterpreted dtype. TCMP takes independent
// source types, so an FP32 operation can consume a U32-carried tile.
// The view is a named lvalue: operations take non-const lvalue references,
// so a temporary reinterpret_tile(...) cannot be passed inline.
using PredT = Tile<Location::Vec, int32_t, 4, 16, BLayout::RowMajor>;
__attribute__((noinline)) void carrier_view(F32T &f, U32T &u, PredT &p) {
  auto u_as_f32 = reinterpret_tile<float>(u);
  TCMP<CmpMode::LT>(p, f, u_as_f32);
}

// TCVT also takes independent dst/src types: the U16 bit view of a BF16
// tile feeds a real conversion.
using U16Out = Tile<Location::Vec, float, 4, 16, BLayout::RowMajor>;
__attribute__((noinline)) void bits_view(BF16T &b, U16Out &out) {
  auto b_as_u16 = reinterpret_tile<uint16_t>(b);
  TCVT(out, b_as_u16);
}

#ifdef NEG_CASE
void neg(F32T &f) {
#if NEG_CASE == 1
  // Width mismatch: 32 -> 16 must be rejected.
  reinterpret_tile<uint16_t>(f);
#elif NEG_CASE == 2
  // Shared source: first phase supports Local Tiles only.
  SharedU32 sh;
  reinterpret_tile<uint32_t>(sh);
#endif
}
#endif

int main() {
  F32T f;
  U32T u;
  PredT p;
  BF16T b;
  U16Out out;
  carrier_view(f, u, p);
  bits_view(b, out);
  return 0;
}
