#include <common/pto_tileop.hpp>

using namespace pto;

using ReductionResult = VecTileM32<float, 32, 128, 32, 1>;
using RowTile = VecTileM32<float, 32, 1, 32, 1>;

// Issue #160 item 8: a reduction-prefix view is a legal TFMA addend and a
// legal source for scalar TMULS and TCVT.  Each consumer must retain the
// CUBE_M32 layout and attach a one-CELL B.SUBVIEW to the viewed source.
__attribute__((noinline)) void consume_issue160_prefix(
    ReductionResult &reduction, RowTile &a, RowTile &b, RowTile &out) {
  auto prefix = TREDUCEPREFIXVIEW<RowTile>(reduction);
  TFMA(out, a, b, prefix);
  TMULS(out, prefix, 0.5f);
  TCVT(out, prefix);
  // Binary reduction-prefix consumers (issue #160 item 6): both source
  // positions must emit a CUBE B.DATR before the dimensions.
  TADD(out, a, prefix);
  TADD(out, prefix, a);
}

// Issue #160 item 4: a single 128-byte CUBE_M32 CELL is [32,1] for FP32 but
// [32,2] for BF16 and [32,4] for E8M0.  The reduction-prefix view must accept
// all three physical column counts, not just FP32's one-column CELL.
using BF16Reduction = VecTileM32<__bf16, 32, 256, 32, 1>;
using BF16Cell = VecTileM32<__bf16, 32, 2, 32, 1>;

__attribute__((noinline)) void consume_issue160_prefix_bf16(
    BF16Reduction &reduction, BF16Cell &a, BF16Cell &b, BF16Cell &out) {
  auto prefix = TREDUCEPREFIXVIEW<BF16Cell>(reduction);
  TFMA(out, a, b, prefix);
  TMULS(out, prefix, (__bf16)0.5f);
  TCVT(out, prefix);
  TADD(out, a, prefix);
  TADD(out, prefix, a);
}

using E8M0Reduction = VecTileM32<__fp8_e8m0, 32, 512, 32, 1>;
using E8M0Cell = VecTileM32<__fp8_e8m0, 32, 4, 32, 1>;

// E8M0 is a scale dtype: exercise the view construction of its [32,4] CELL and
// a legal consumer (TCVT to a BF16 CELL) rather than arithmetic overloads.
__attribute__((noinline)) void consume_issue160_prefix_e8m0(
    E8M0Reduction &reduction, BF16Cell &out) {
  auto prefix = TREDUCEPREFIXVIEW<E8M0Cell>(reduction);
  TCVT(out, prefix);
}

int main() {
  ReductionResult reduction;
  RowTile a;
  RowTile b;
  RowTile out;
  consume_issue160_prefix(reduction, a, b, out);
  asm volatile("" : : "Tr"(out.data()));

  BF16Reduction bf16_reduction;
  BF16Cell bf16_a;
  BF16Cell bf16_b;
  BF16Cell bf16_out;
  consume_issue160_prefix_bf16(bf16_reduction, bf16_a, bf16_b, bf16_out);
  asm volatile("" : : "Tr"(bf16_out.data()));

  E8M0Reduction e8m0_reduction;
  BF16Cell e8m0_out;
  consume_issue160_prefix_e8m0(e8m0_reduction, e8m0_out);
  asm volatile("" : : "Tr"(e8m0_out.data()));
  return 0;
}
