// PTO #311 / issue #147: a wide CUBE row-reduction destination's first CELL
// is the logical [M,1] result. TREDUCEPREFIXVIEW borrows it zero-copy, and
// every TEPL source family below consumes it with a source-selecting
// B.SUBVIEW attached to the owning B.IOT's range group.

#include <jcore/template_asm.hpp>
#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using Reduction = VecTileM32<float, 32, 128, 32, 1>;
using Row = VecTileM32<float, 32, 1, 32, 1>;
using Matrix = VecTileM32<float, 32, 32>;

__attribute__((noinline)) void tfma_prefix_addend(Row &newSum, Matrix &lhs,
                                                  Matrix &rhs,
                                                  Reduction &localSumWide) {
  auto prefix = TREDUCEPREFIXVIEW<Row>(localSumWide);
  TFMA(newSum, lhs, rhs, prefix);  // B.SUBVIEW 0 on the src2 binder
}

__attribute__((noinline)) void tmuls_prefix(Row &scale, Reduction &amaxWide) {
  auto prefix = TREDUCEPREFIXVIEW<Row>(amaxWide);
  TMULS(scale, prefix, 0.25f);  // B.SUBVIEW 0 on the src binder
}

__attribute__((noinline)) void trecip_prefix(Row &inv, Reduction &sumWide) {
  auto prefix = TREDUCEPREFIXVIEW<Row>(sumWide);
  TRECIP(inv, prefix);  // B.SUBVIEW 0 on the src binder
}

__attribute__((noinline)) void tcvt_prefix(VecTileM32<__half, 32, 1, 32, 1> &dst,
                                           Reduction &srcWide) {
  auto prefix = TREDUCEPREFIXVIEW<Row>(srcWide);
  TCVT(dst, prefix);  // B.SUBVIEW 0 on the src binder
}

__attribute__((noinline)) void trowexpandmul_prefix(Matrix &dst, Matrix &base,
                                                    Reduction &sumWide) {
  auto prefix = TREDUCEPREFIXVIEW<Row>(sumWide);
  TROWEXPANDMUL(dst, base, prefix);  // B.SUBVIEW 1 on the shared B.IOT
}

int main() {
  Reduction reduction;
  Row row, row2;
  Matrix matrix;
  tfma_prefix_addend(row, matrix, matrix, reduction);
  tmuls_prefix(row, reduction);
  trecip_prefix(row, reduction);
  VecTileM32<__half, 32, 1, 32, 1> halfRow;
  tcvt_prefix(halfRow, reduction);
  trowexpandmul_prefix(matrix, matrix, reduction);
  asm volatile("" : : "Tr"(row.data()), "Tr"(matrix.data()));
  return 0;
}
