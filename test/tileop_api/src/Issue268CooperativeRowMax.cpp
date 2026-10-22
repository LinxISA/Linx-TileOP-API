// Issue #268: cooperative RowMaxInit and RowMaxEn both use the per-PE row
// reduction shape.  Shared-A's valid rows still describe the core-total
// group_M, so a 128-row group produces 32-row D/RowMax carriers per PE.
#include <common/pto_tileop.hpp>

using namespace pto;

using QLeft = SharedTile<SharedMatrixLeft<__bf16, 128, 64>>;
using KRight = SharedTile<SharedMatrixRight<__bf16, 128, 64>>;
using Dst = CubeTileM32<__bf16, 32, 128>;
using RowState = VecTileM32<__bf16, 32, 2, 32, 1>;
using GroupMax = VecTileM32<__bf16, 32, 4>;

void cooperative_row_max(QLeft &q, KRight &k) {
  Dst d;
  RowState row_in, row_out;
  GroupMax group_out;
  TMATMUL(d, q, k,
          fixp::bf16().row_max(row_in, row_out).group_max<32>(group_out));
}
