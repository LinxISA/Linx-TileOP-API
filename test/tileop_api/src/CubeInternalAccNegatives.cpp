// CUBE InternalAcc negative contracts: each NEG_CASE must fail to compile.
// Driven by run_negatives-style invocations (-DNEG_CASE=N).
#include <jcore/template_asm.hpp>

using namespace pto;

using A = SharedMatrixLeft<float, 64, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using CH = Tile<Location::Acc, __half, 16, 16, BLayout::CubeM16>;

#ifdef NEG_CASE
__attribute__((noinline)) void neg(
#ifdef NEG_CASE2_DST
    CH &d,
#else
    C &d,
#endif
    C &c, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a); auto sb = TMOV_L2S_INSERT(b);
#if NEG_CASE == 1
  // CCTRL[1] hint on a non-ACC TMATMUL: must be rejected.
  TMATMUL(d, sa, sb, fixp::Options<FixpAttr{}.with_cube_ctrl(CubeCtrlInternalAccHint)>{});
#elif NEG_CASE == 2
  // CCTRL[0] raw D + f16 PreQuant (dtype-matching dst): must be rejected.
  TMATMUL_ACC(d, c, sa, sb, fixp::Options<FixpAttr::f16().with_cube_ctrl(CubeCtrlRawAccumulator)>{});
#elif NEG_CASE == 3
  // CCTRL[0] raw D + Relu: must be rejected.
  TMATMUL_ACC(d, c, sa, sb, fixp::Options<FixpAttr::keep_acc(FixpReluMode::Relu).with_cube_ctrl(CubeCtrlRawAccumulator)>{});
#endif
}
int main() { return 0; }
#endif
