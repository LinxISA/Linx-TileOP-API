// Shared/Local storage-form test: the Shared-capable TMATMUL variants are
// exercised with Local-A/Shared-B and Shared-A/Shared-B. MX scale tiles
// follow their matrix's storage (handoff Sec 1.5); the options overloads
// carry the Shared scale, the no-options overloads keep Local scale.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = TileLeft<float, 16, 16>;
using B = TileRight<float, 16, 16>;
using S = TileLeft<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

void shared_forms(C &d, C &c, A &a, B &b, S &sa, S &sb) {
  auto keep = fixp::Options<FixpAttr::keep_acc()>{};
  auto shared_b = TMOV_L2S_INSERT(b);
  auto shared_a = TMOV_L2S_INSERT(a);
  auto shared_sa = TMOV_L2S_INSERT(sa);
  auto shared_sb = TMOV_L2S_INSERT(sb);

  // Local A + Shared B (single-binder stream).
  TMATMUL(d, a, shared_b);
  TMATMUL_ACC(d, c, a, shared_b);
  TMATMUL_BIAS(d, a, shared_b, c);
  TMATMUL_MX(d, a, sa, shared_b, shared_sb, keep);      // Shared B+ScaleB
  TMATMUL_MX_ACC(d, c, a, sa, shared_b, shared_sb, keep);
  TMATMUL_MX_BIAS(d, a, sa, shared_b, shared_sb, c, keep);

  // Shared A + Shared B (two-binder stream; scale follows storage).
  TMATMUL(d, shared_a, shared_b);
  TMATMUL_ACC(d, c, shared_a, shared_b);
  TMATMUL_BIAS(d, shared_a, shared_b, c);
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
  shared_forms(d, c, a, b, sa, sb);
  use(&d);
  return 0;
}
