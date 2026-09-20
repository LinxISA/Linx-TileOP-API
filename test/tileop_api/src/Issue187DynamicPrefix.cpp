#include <common/pto_tileop.hpp>

using namespace pto;

// Issue #187: a CUBE_M32 tile with a dynamic valid row (ValidRow == -1)
// consumed through the binary reduction-prefix emitters must send the
// ValidRow dimension through the "B.DIM RegSrc, uimm17, ->LB1" register
// form; the unsigned uimm17 immediate form cannot carry -1.
using DynCell = VecTileM32<float, 32, 1, -1, 1>;
using DynReduction = VecTileM32<float, 32, 128, -1, 1>;
using DynRow = VecTileM32<float, 32, 1, -1, 1>;

__attribute__((noinline)) void consume_dyn_prefix_tile_src(
    DynReduction &reduction, DynRow &a, DynRow &b, DynCell &out) {
  auto prefix = TREDUCEPREFIXVIEW<DynCell>(reduction);
  TADD(out, a, prefix);
  TADD(out, prefix, a);
}
