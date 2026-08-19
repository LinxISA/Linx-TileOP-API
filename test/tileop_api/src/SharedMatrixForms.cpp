// Shared/Local storage-form test: Local-A/Shared-B keeps ordinary M, while
// Shared-A/Shared-B uses the fixed 4-PE Group contract and a per-PE local C.
// MX scale tiles follow their matrix's storage.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = TileLeft<float, 16, 16>;
using B = TileRight<float, 16, 16>;
using S = TileLeft<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

using GroupA = TileLeft<float, 64, 16>;
using GroupB = TileRight<float, 16, 16>;
using GroupAScale = TileLeft<float, 64, 16>;
using GroupBScale = TileRight<float, 16, 16>;

void shared_b_forms(C &d, C &c, A &a, B &b, S &sa, S &sb) {
  auto keep = fixp::Options<FixpAttr::keep_acc()>{};
  auto shared_b = TMOV_L2S_INSERT(b);
  auto shared_sb = TMOV_L2S_INSERT(sb);

  TMATMUL(d, a, shared_b);
  TMATMUL_ACC(d, c, a, shared_b);
  TMATMUL_BIAS(d, a, shared_b, c);
  TMATMUL_MX(d, a, sa, shared_b, shared_sb, keep);
  TMATMUL_MX_ACC(d, c, a, sa, shared_b, shared_sb, keep);
  TMATMUL_MX_BIAS(d, a, sa, shared_b, shared_sb, c, keep);
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
  TMATMUL_MX(d, shared_a, sa, shared_b, sb);
  TMATMUL_MX_ACC(d, c, shared_a, sa, shared_b, sb);
  TMATMUL_MX_BIAS(d, shared_a, sa, shared_b, sb, c);
  TMATMUL_MX(d, shared_a, shared_sa, shared_b, shared_sb, keep);
  TMATMUL_MX_ACC(d, c, shared_a, shared_sa, shared_b, shared_sb, keep);
  TMATMUL_MX_BIAS(d, shared_a, shared_sa, shared_b, shared_sb, c, keep);
}

void use(void *) {}

int main() {
  static C d, c;
  static A a;
  static B b;
  static S sa, sb;
  static GroupA group_a;
  static GroupB group_b;
  static GroupAScale group_sa;
  static GroupBScale group_sb;
  shared_b_forms(d, c, a, b, sa, sb);
  group_forms(d, c, group_a, group_b, group_sa, group_sb);
  use(&d);
  return 0;
}
