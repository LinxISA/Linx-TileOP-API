// CUBE InternalAcc (PTO-ISA 0.58.6 spec#236) regression: CCTRL encoding and
// legality. Positive cases lower the four CCTRL values into the B.DATR
// PadValueOrByteId[1:0] union field; illegal combinations must be rejected
// at compile time (see the paired negative runner).
#include <jcore/template_asm.hpp>

using namespace pto;

using A = SharedMatrixLeft<float, 64, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using Opt = fixp::Options<FixpAttr{}>;

// CCTRL = 0 (None)    -> B.DATR ..., Zero, ...
__attribute__((noinline)) void acc_none(C &d, C &c, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a); auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c, sa, sb, Opt{});
}
// CCTRL = 2 (InternalAccHint, ACC-only transparent cache hint) -> Min
__attribute__((noinline)) void acc_hint(C &d, C &c, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a); auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c, sa, sb, Opt{}.acc_hint());
}
// CCTRL = 1 (RawAccumulator, raw accumulator-type D) -> Max
__attribute__((noinline)) void acc_raw(C &d, C &c, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a); auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c, sa, sb, Opt{}.raw_acc());
}
// CCTRL = 3 (RawAccumulator | InternalAccHint) -> Null
__attribute__((noinline)) void acc_raw_hint(C &d, C &c, A &a, B &b) {
  auto sa = TMOV_L2S_INSERT(a); auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c, sa, sb, Opt{}.raw_acc().acc_hint());
}

int main() { return 0; }
