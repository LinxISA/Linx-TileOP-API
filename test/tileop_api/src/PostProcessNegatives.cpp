// Negative tests: each block below must fail to compile (the assertion named
// in the message fires). Driven by run_negatives.sh which compiles this file
// once per -DSHOULD_FAIL_<N> and expects failure.
#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM32<float, 32, 32>;
using Ds8 = CubeAccumulatorM32<int8_t, 32, 32>;
using A = CubeTileM32<float, 32, 64>;
using B = CubeTileN8<float, 64, 32>;
using R = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 2>;  // bad ValidCol
using G = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 8>;  // bad for N=32/GroupN=16
using GroupA = SharedMatrixLeft<float, 16, 16>;
using GroupB = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using BadGroupC = CubeAccumulatorM16<float, 8, 16>;
using BadGroupK = SharedMatrixRight<float, 32, 16>;
using BadGroupN = SharedMatrixRight<float, 16, 32>;
using DynamicGroupA = SharedMatrixLeft<float, 16, 16, DYNAMIC, DYNAMIC>;
using DynamicGroupB = SharedMatrixRight<float, 16, 16, DYNAMIC, DYNAMIC>;
using DynamicGroupC = CubeAccumulatorM16<float, 16, 16, DYNAMIC, DYNAMIC>;
using OldA = TileLeft<float, 32, 64>;
using BadLayoutD = CubeAccumulatorM16<float, 16, 32>;
using BadLayoutA = CubeTileM32<float, 16, 64>;
using BadK = CubeTileN8<float, 32, 32>;
using BadGemvVec = CubeTileM16<float, 2, 64>;
using GemvMtx = CubeTileN8<float, 64, 32>;
using GemvDst = CubeAccumulatorM16<float, 1, 32>;
using MixedA = CubeTileM16<int8_t, 16, 32>;
using MixedB = CubeTileN8<uint16_t, 32, 16>;
using MixedD = CubeAccumulatorM16<int32_t, 16, 16>;
using UnsignedA = CubeTileM16<uint8_t, 16, 32>;
using UnsignedB = CubeTileN8<uint16_t, 32, 16>;
using UnsignedQ = CubeAccumulatorM16<int8_t, 16, 16>;
using BadDValid = CubeAccumulatorM32<float, 32, 32, 32, 16>;
using BadAcc = CubeAccumulatorM32<int32_t, 32, 32>;
using BadBias = Tile<Location::Bias, int32_t, 8, 32,
                     BLayout::RowMajor, 1, 32>;
using NegMXA = CubeTileM32<__fp8_e4m3, 32, 64>;
using NegMXB = CubeTileN8<__fp8_e4m3, 64, 32>;
using PlainMXA = CubeTileM32<__half, 32, 64>;
using PlainMXB = CubeTileN8<__bf16, 64, 32>;
using GoodScaleA = Tile<Location::Scaling, __fp8_e8m0, 32, 2,
                        BLayout::RowMajor, 32, 2>;
using GoodScaleB = Tile<Location::Scaling, __fp8_e8m0, 2, 32,
                        BLayout::RowMajor, 2, 32>;
using BadScaleType = Tile<Location::Scaling, float, 1, 32,
                          BLayout::RowMajor, 1, 2>;
using BadScaleShape = Tile<Location::Scaling, __fp8_e8m0, 4, 32,
                           BLayout::RowMajor, 1, 32>;
using TransStoredA = SharedMatrixLeft<float, 32, 16>;
using TransStoredB = SharedMatrixRight<float, 32, 32, 24, 32>;
using BadTransD = CubeAccumulatorM16<float, 16, 24, 16, 16>;

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
#if defined(SHOULD_FAIL_old_rowmajor)
  OldA old_a;
  TMATMUL(d, old_a, b);
#endif
#if defined(SHOULD_FAIL_mismatched_m_layout)
  BadLayoutD bad_d;
  BadLayoutA bad_a;
  TMATMUL(bad_d, bad_a, b);
#endif
#if defined(SHOULD_FAIL_local_k)
  BadK bad_b;
  TMATMUL(d, a, bad_b);
#endif
#if defined(SHOULD_FAIL_shared_cube_layout)
  auto bad_shared_a = TMOV_L2S_INSERT(a);
  SharedMatrixRight<float, 64, 32> ordinary_b;
  auto shared_b = TMOV_L2S_INSERT(ordinary_b);
  TMATMUL(d, bad_shared_a, shared_b);
#endif
#if defined(SHOULD_FAIL_gemv_rows)
  BadGemvVec bad_vec;
  GemvMtx matrix;
  GemvDst out;
  TGEMV(out, matrix, bad_vec);
#endif
#if defined(SHOULD_FAIL_mixed_numeric_class)
  MixedA mixed_a;
  MixedB mixed_b;
  MixedD mixed_d;
  TMATMUL(mixed_d, mixed_a, mixed_b);
#endif
#if defined(SHOULD_FAIL_unsigned_prequant)
  UnsignedA unsigned_a;
  UnsignedB unsigned_b;
  UnsignedQ quantized;
  TMATMUL(quantized, unsigned_a, unsigned_b, fixp::s8(0));
#endif
#if defined(SHOULD_FAIL_bad_d_valid_shape)
  BadDValid bad_d;
  TMATMUL(bad_d, a, b);
#endif
#if defined(SHOULD_FAIL_bad_acc_dtype)
  BadAcc bad_acc;
  TMATMUL_ACC(d, bad_acc, a, b);
#endif
#if defined(SHOULD_FAIL_bad_bias_dtype)
  BadBias bad_bias;
  TMATMUL_BIAS(d, a, b, bad_bias);
#endif
#if defined(SHOULD_FAIL_bad_mx_scale_dtype)
  NegMXA mxa;
  NegMXB mxb;
  BadScaleType bad_sa;
  BadScaleShape sb;
  TMATMUL_MX(d, mxa, bad_sa, mxb, sb);
#endif
#if defined(SHOULD_FAIL_bad_mx_scale_shape)
  NegMXA mxa;
  NegMXB mxb;
  BadScaleShape bad_sa;
  BadScaleShape bad_sb;
  TMATMUL_MX(d, mxa, bad_sa, mxb, bad_sb);
#endif
#if defined(SHOULD_FAIL_missing_mx_scale_a)
  NegMXA mxa;
  PlainMXB mxb;
  TMATMUL_MX(d, mxa, mxb);
#endif
#if defined(SHOULD_FAIL_missing_mx_scale_b)
  PlainMXA mxa;
  NegMXB mxb;
  TMATMUL_MX(d, mxa, mxb);
#endif
#if defined(SHOULD_FAIL_extra_mx_scale_a)
  PlainMXA mxa;
  PlainMXB mxb;
  GoodScaleA scale_a;
  TMATMUL_MX(d, mxa, scale_a, mxb);
#endif
#if defined(SHOULD_FAIL_extra_mx_scale_b)
  PlainMXA mxa;
  PlainMXB mxb;
  GoodScaleB scale_b;
  TMATMUL_MX(d, mxa, mxb, scale_b);
#endif
#if defined(SHOULD_FAIL_bad_transpose_d)
  TransStoredA stored_a;
  TransStoredB stored_b;
  BadTransD bad_d;
  auto shared_a = TMOV_L2S_INSERT(stored_a);
  auto shared_b = TMOV_L2S_INSERT(stored_b);
  TMATMUL(bad_d, shared_a, shared_b,
          fixp::keep_acc().transpose_a().transpose_b());
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
  DynamicGroupA group_a(16, 16);
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
