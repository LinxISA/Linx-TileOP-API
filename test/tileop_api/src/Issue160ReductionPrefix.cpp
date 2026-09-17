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
}

int main() {
  ReductionResult reduction;
  RowTile a;
  RowTile b;
  RowTile out;
  consume_issue160_prefix(reduction, a, b, out);
  asm volatile("" : : "Tr"(out.data()));
  return 0;
}
