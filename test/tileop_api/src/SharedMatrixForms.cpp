// Shared/Local storage-form test: Local-A/Shared-B keeps ordinary M, while
// Shared-A/Shared-B preserves the same logical M/K and K/N rectangles.
// MX scale tiles follow their matrix's storage.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = CubeTileM16<float, 16, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using SA = Tile<Location::Scaling, float, 16, 16, BLayout::RowMajor>;
using SB = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;

using GroupA = SharedMatrixLeft<float, 16, 16>;
using GroupB = SharedMatrixRight<float, 16, 16>;
using GroupAScale = SharedMatrixLeft<float, 16, 16>;
using GroupBScale = SharedMatrixRight<float, 16, 16>;
using MXA = SharedMatrixLeft<__fp8_e4m3, 16, 16>;
using MXB = SharedMatrixRight<__fp8_e4m3, 16, 16>;
using MXSA = SharedMatrixLeft<__fp8_e8m0, 16, 8, 16, 1>;
using MXSB = SharedMatrixRight<__fp8_e8m0, 8, 16, 1, 16>;

void shared_b_forms(C &d, C &c, A &a, B &b, SA &sa, SB &sb) {
  auto keep = fixp::Options<FixpAttr::keep_acc()>{};
  auto shared_b = TMOV_L2S_INSERT(b);
  auto shared_sb = TMOV_L2S_INSERT(sb);

  TMATMUL(d, a, shared_b);
  TMATMUL_ACC(d, c, a, shared_b);
  TMATMUL_BIAS(d, a, shared_b, c);
  (void)sa;
  (void)shared_sb;
}

void group_forms(C &d, C &c, GroupA &a, GroupB &b,
                 GroupAScale &sa, GroupBScale &sb) {
  auto keep = fixp::Options<FixpAttr::keep_acc()>{};
  auto shared_a = TMOV_L2S_INSERT(a);
  auto shared_b = TMOV_L2S_INSERT(b);
  auto shared_sa = TMOV_L2S_INSERT(sa);
  auto shared_sb = TMOV_L2S_INSERT(sb);

  TMATMUL(d, shared_a, shared_b);
  TMATMUL_ACC(d, c, shared_a, shared_b);
  TMATMUL_BIAS(d, shared_a, shared_b, c);
  TMATMUL(d, shared_a, shared_b, keep);
  TMATMUL_ACC(d, c, shared_a, shared_b, keep);
  TMATMUL_BIAS(d, shared_a, shared_b, c, keep);
  (void)sa;
  (void)sb;
  (void)shared_sa;
  (void)shared_sb;
}

void shared_mx_forms(C &d, C &c, MXA &a, MXB &b, MXSA &sa, MXSB &sb) {
  auto shared_a = TMOV_L2S_INSERT(a);
  auto shared_b = TMOV_L2S_INSERT(b);
  auto shared_sa = TMOV_L2S_INSERT(sa);
  auto shared_sb = TMOV_L2S_INSERT(sb);
  TMATMUL_MX(d, shared_a, shared_sa, shared_b, shared_sb);
  TMATMUL_MX_ACC(d, c, shared_a, shared_sa, shared_b, shared_sb);
  TMATMUL_MX_BIAS(d, shared_a, shared_sa, shared_b, shared_sb, c);
}

void use(void *) {}

int main() {
  static C d, c;
  static A a;
  static B b;
  static SA sa;
  static SB sb;
  static GroupA group_a;
  static GroupB group_b;
  static GroupAScale group_sa;
  static GroupBScale group_sb;
  static MXA mxa;
  static MXB mxb;
  static MXSA mxsa;
  static MXSB mxsb;
  shared_b_forms(d, c, a, b, sa, sb);
  group_forms(d, c, group_a, group_b, group_sa, group_sb);
  shared_mx_forms(d, c, mxa, mxb, mxsa, mxsb);
  use(&d);
  return 0;
}
