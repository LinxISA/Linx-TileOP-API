// Negative tests: each block below must fail to compile (the assertion named
// in the message fires). Driven by run_negatives.sh which compiles this file
// once per -DSHOULD_FAIL_<N> and expects failure.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using Ds8 = Tile<Location::Vec, int8_t, 32, 32, BLayout::RowMajor>;
using A = TileLeft<float, 32, 64>;
using B = TileRight<float, 64, 32>;
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 2>;  // bad ValidCol
using G = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 8>;  // bad for N=32/GroupN=16
using GroupA = TileLeft<float, 64, 16>;
using GroupB = TileRight<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using BadGroupC = Tile<Location::Vec, float, 8, 16, BLayout::RowMajor>;
using BadGroupK = TileRight<float, 32, 16>;
using BadGroupN = TileRight<float, 16, 32>;
using DynamicGroupA = TileLeft<float, 64, 16, DYNAMIC, DYNAMIC>;
using DynamicGroupB = TileRight<float, 16, 16, DYNAMIC, DYNAMIC>;
using DynamicGroupC = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor,
                           DYNAMIC, DYNAMIC>;

void fail_cases(D &d, Ds8 &d8, A &a, B &b, R &r, G &g) {
#if defined(SHOULD_FAIL_dtype)
  // QF322S8Pre requires an S8 destination, not FP32.
  TMATMUL(d, a, b, fixp::s8(0x7));
#endif
#if defined(SHOULD_FAIL_maxabs_no_max)
  // max_abs() without RowMax or GroupMax is rejected by the builder.
  TMATMUL(d, a, b, fixp::keep_acc().max_abs());
#endif
#if defined(SHOULD_FAIL_rowmax_shape)
  // RowMaxOut ValidCol must be 1.
  TMATMUL(d, a, b, fixp::keep_acc().row_max(r));
#endif
#if defined(SHOULD_FAIL_groupmax_shape)
  // GroupMaxOut ValidCol must be ceil(N/GroupN); N=32,GroupN=16 -> 2, not 8.
  TMATMUL(d, a, b, fixp::keep_acc().group_max<16>(g));
#endif
#if defined(SHOULD_FAIL_lone_shared_a)
  // Shared A without Shared B is rejected (single binder = Shared-Right).
  auto sa = TMOV_L2S_INSERT(a);
  TMATMUL(d, sa, b);
#endif
#if defined(SHOULD_FAIL_local_transpose)
  // FPATR transpose is a cooperative Shared materialization control.
  TMATMUL(d, a, b, fixp::keep_acc().transpose_a());
#endif
#if defined(SHOULD_FAIL_group_shape)
  GroupA group_a;
  GroupB group_b;
  BadGroupC bad_c;
  auto sa = TMOV_L2S_INSERT(group_a);
  auto sb = TMOV_L2S_INSERT(group_b);
  TMATMUL(bad_c, sa, sb);
#endif
#if defined(SHOULD_FAIL_group_k)
  GroupA group_a;
  BadGroupK group_b;
  C group_c;
  auto sa = TMOV_L2S_INSERT(group_a);
  auto sb = TMOV_L2S_INSERT(group_b);
  TMATMUL(group_c, sa, sb);
#endif
#if defined(SHOULD_FAIL_group_n)
  GroupA group_a;
  BadGroupN group_b;
  C group_c;
  auto sa = TMOV_L2S_INSERT(group_a);
  auto sb = TMOV_L2S_INSERT(group_b);
  TMATMUL(group_c, sa, sb);
#endif
#if defined(SHOULD_FAIL_group_dynamic)
  DynamicGroupA group_a(64, 16);
  DynamicGroupB group_b(16, 16);
  DynamicGroupC group_c(16, 16);
  auto sa = TMOV_L2S_INSERT(group_a);
  auto sb = TMOV_L2S_INSERT(group_b);
  TMATMUL(group_c, sa, sb);
#endif
}

void use(void *) {}

int main() {
  static D d;
  static Ds8 d8;
  static A a;
  static B b;
  static R r;
  static G g;
  fail_cases(d, d8, a, b, r, g);
  use(&d);
  return 0;
}
