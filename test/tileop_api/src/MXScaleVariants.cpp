#include <common/pto_tileop.hpp>

using namespace pto;

using D = CubeAccumulatorM16<float, 16, 16>;
using Bias = Tile<Location::Bias, float, 8, 16,
                  BLayout::RowMajor, 1, 16>;
using SA = Tile<Location::Scaling, __fp8_e8m0, 16, 8,
                BLayout::RowMajor, 16, 1>;
using SB = Tile<Location::Scaling, __fp8_e8m0, 8, 16,
                BLayout::RowMajor, 1, 16>;

template <typename T> using MA = CubeTileM16<T, 16, 32>;
template <typename T> using MB = CubeTileN8<T, 32, 16>;

using GV_D = CubeAccumulatorM16<float, 1, 16>;
using GV_SA = Tile<Location::Scaling, __fp8_e8m0, 16, 8,
                   BLayout::RowMajor, 1, 1>;
using GV_SB = Tile<Location::Scaling, __fp8_e8m0, 8, 16,
                   BLayout::RowMajor, 1, 16>;
template <typename T> using GVA = CubeTileM16<T, 1, 32>;
template <typename T> using GVB = CubeTileN8<T, 32, 16>;

template <typename T, int Rows, int Cols>
using GM = global_tensor<T, RowMajor<Rows, Cols>>;

using F16A = MA<__half>;
using BF16A = MA<__bf16>;
using E4A = MA<__fp8_e4m3>;
using E2A = MA<__fp4_e2m1x2>;
using E1A = MA<__fp4_e1m2x2>;
using F16B = MB<__half>;
using BF16B = MB<__bf16>;
using E5B = MB<__fp8_e5m2>;
using E2B = MB<__fp4_e2m1x2>;
using E1B = MB<__fp4_e1m2x2>;
template <typename T> using SharedA = SharedMatrixLeft<T, 16, 32>;
template <typename T> using SharedB = SharedMatrixRight<T, 32, 16>;
using SharedSA = SharedMatrixLeft<__fp8_e8m0, 16, 8, 16, 1>;
using SharedSB = SharedMatrixRight<__fp8_e8m0, 8, 16, 1, 16>;

__attribute__((noinline)) void carrier_zero_scale(
    float *output, float *c_input, float *bias_input,
    __half *a_input, __bf16 *b_input) {
  GM<float, 16, 16> gm_d(output);
  GM<float, 16, 16> gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__half, 16, 32> gm_a(a_input);
  GM<__bf16, 32, 16> gm_b(b_input);
  D d, c;
  Bias bias;
  F16A a;
  BF16B b;
  TLOAD_CUBE(c, gm_c);
  TLOAD(bias, gm_bias);
  TLOAD_CUBE(a, gm_a);
  TLOAD_CUBE(b, gm_b);
  TMATMUL_MX(d, a, b);
  TMATMUL_MX_ACC(d, c, a, b);
  TMATMUL_MX_BIAS(d, a, b, bias);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_scale_a(
    float *output, float *c_input, float *bias_input,
    __fp8_e4m3 *a_input, __fp8_e8m0 *scale_input,
    __half *b_input) {
  GM<float, 16, 16> gm_d(output);
  GM<float, 16, 16> gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__fp8_e4m3, 16, 32> gm_a(a_input);
  GM<__fp8_e8m0, 16, 8> gm_sa(scale_input);
  GM<__half, 32, 16> gm_b(b_input);
  D d, c;
  Bias bias;
  E4A a;
  SA sa;
  F16B b;
  TLOAD_CUBE(c, gm_c);
  TLOAD(bias, gm_bias);
  TLOAD_CUBE(a, gm_a);
  TLOAD(sa, gm_sa);
  TLOAD_CUBE(b, gm_b);
  TMATMUL_MX(d, a, sa, b);
  TMATMUL_MX_ACC(d, c, a, sa, b);
  TMATMUL_MX_BIAS(d, a, sa, b, bias);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_scale_b(
    float *output, float *c_input, float *bias_input,
    __bf16 *a_input, __fp8_e5m2 *b_input,
    __fp8_e8m0 *scale_input) {
  GM<float, 16, 16> gm_d(output);
  GM<float, 16, 16> gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__bf16, 16, 32> gm_a(a_input);
  GM<__fp8_e5m2, 32, 16> gm_b(b_input);
  GM<__fp8_e8m0, 8, 16> gm_sb(scale_input);
  D d, c;
  Bias bias;
  BF16A a;
  E5B b;
  SB sb;
  TLOAD_CUBE(c, gm_c);
  TLOAD(bias, gm_bias);
  TLOAD_CUBE(a, gm_a);
  TLOAD_CUBE(b, gm_b);
  TLOAD(sb, gm_sb);
  TMATMUL_MX(d, a, b, sb);
  TMATMUL_MX_ACC(d, c, a, b, sb);
  TMATMUL_MX_BIAS(d, a, b, sb, bias);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_both_scales(
    float *output, float *c_input, float *bias_input,
    __fp8_e4m3 *a_input, __fp8_e8m0 *scale_a_input,
    __fp8_e5m2 *b_input, __fp8_e8m0 *scale_b_input) {
  GM<float, 16, 16> gm_d(output);
  GM<float, 16, 16> gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__fp8_e4m3, 16, 32> gm_a(a_input);
  GM<__fp8_e8m0, 16, 8> gm_sa(scale_a_input);
  GM<__fp8_e5m2, 32, 16> gm_b(b_input);
  GM<__fp8_e8m0, 8, 16> gm_sb(scale_b_input);
  D d, c;
  Bias bias;
  E4A a;
  SA sa;
  E5B b;
  SB sb;
  TLOAD_CUBE(c, gm_c);
  TLOAD(bias, gm_bias);
  TLOAD_CUBE(a, gm_a);
  TLOAD(sa, gm_sa);
  TLOAD_CUBE(b, gm_b);
  TLOAD(sb, gm_sb);
  TMATMUL_MX(d, a, sa, b, sb);
  TMATMUL_MX_ACC(d, c, a, sa, b, sb);
  TMATMUL_MX_BIAS(d, a, sa, b, sb, bias);
  TSTORE_CUBE(gm_d, d);
}

#define SHARED_MX_COMMON_SETUP()                                                \
  GM<float, 16, 16> gm_d(output), gm_c(c_input);                              \
  GM<float, 8, 16> gm_bias(bias_input);                                       \
  D d, c; Bias bias;                                                           \
  TLOAD_CUBE(c, gm_c); TLOAD(bias, gm_bias)

__attribute__((noinline)) void carrier_shared_zero_scale(
    float *output, float *c_input, float *bias_input,
    __half *a_input, __bf16 *b_input) {
  SHARED_MX_COMMON_SETUP();
  GM<__half, 16, 32> gm_a(a_input); GM<__bf16, 32, 16> gm_b(b_input);
  auto a = TLOAD<SharedA<__half>>(gm_a);
  auto b = TLOAD<SharedB<__bf16>>(gm_b);
  pto_matmul_detail::NoScaleOperand none; auto opt = fixp::keep_acc();
  TMATMUL_MX<0>(d, a, none, b, none, opt);
  TMATMUL_MX_ACC<0>(d, c, a, none, b, none, opt);
  TMATMUL_MX_BIAS<0>(d, a, none, b, none, bias, opt); TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_shared_scale_a(
    float *output, float *c_input, float *bias_input,
    __fp8_e4m3 *a_input, __fp8_e8m0 *sa_input, __half *b_input) {
  SHARED_MX_COMMON_SETUP();
  GM<__fp8_e4m3, 16, 32> gm_a(a_input); GM<__fp8_e8m0, 16, 8> gm_sa(sa_input);
  GM<__half, 32, 16> gm_b(b_input);
  auto a = TLOAD<SharedA<__fp8_e4m3>>(gm_a); auto sa = TLOAD<SharedSA>(gm_sa);
  auto b = TLOAD<SharedB<__half>>(gm_b);
  pto_matmul_detail::NoScaleOperand none; auto opt = fixp::keep_acc();
  TMATMUL_MX<1>(d, a, sa, b, none, opt);
  TMATMUL_MX_ACC<1>(d, c, a, sa, b, none, opt);
  TMATMUL_MX_BIAS<1>(d, a, sa, b, none, bias, opt); TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_shared_scale_b(
    float *output, float *c_input, float *bias_input,
    __bf16 *a_input, __fp8_e5m2 *b_input, __fp8_e8m0 *sb_input) {
  SHARED_MX_COMMON_SETUP();
  GM<__bf16, 16, 32> gm_a(a_input); GM<__fp8_e5m2, 32, 16> gm_b(b_input);
  GM<__fp8_e8m0, 8, 16> gm_sb(sb_input);
  auto a = TLOAD<SharedA<__bf16>>(gm_a); auto b = TLOAD<SharedB<__fp8_e5m2>>(gm_b);
  auto sb = TLOAD<SharedSB>(gm_sb);
  pto_matmul_detail::NoScaleOperand none; auto opt = fixp::keep_acc();
  TMATMUL_MX<2>(d, a, none, b, sb, opt);
  TMATMUL_MX_ACC<2>(d, c, a, none, b, sb, opt);
  TMATMUL_MX_BIAS<2>(d, a, none, b, sb, bias, opt); TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_shared_both_scales(
    float *output, float *c_input, float *bias_input,
    __fp8_e4m3 *a_input, __fp8_e8m0 *sa_input,
    __fp8_e5m2 *b_input, __fp8_e8m0 *sb_input) {
  SHARED_MX_COMMON_SETUP();
  GM<__fp8_e4m3, 16, 32> gm_a(a_input); GM<__fp8_e8m0, 16, 8> gm_sa(sa_input);
  GM<__fp8_e5m2, 32, 16> gm_b(b_input); GM<__fp8_e8m0, 8, 16> gm_sb(sb_input);
  auto a = TLOAD<SharedA<__fp8_e4m3>>(gm_a); auto sa = TLOAD<SharedSA>(gm_sa);
  auto b = TLOAD<SharedB<__fp8_e5m2>>(gm_b); auto sb = TLOAD<SharedSB>(gm_sb);
  auto opt = fixp::keep_acc();
  TMATMUL_MX<3>(d, a, sa, b, sb, opt);
  TMATMUL_MX_ACC<3>(d, c, a, sa, b, sb, opt);
  TMATMUL_MX_BIAS<3>(d, a, sa, b, sb, bias, opt); TSTORE_CUBE(gm_d, d);
}

#undef SHARED_MX_COMMON_SETUP

__attribute__((noinline)) void carrier_gemv_zero_scale(
    float *output, float *c_input, float *bias_input,
    __half *vec_input, __bf16 *mtx_input) {
  GM<float, 1, 16> gm_d(output), gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__half, 1, 32> gm_vec(vec_input);
  GM<__bf16, 32, 16> gm_mtx(mtx_input);
  GV_D d, c; Bias bias; GVA<__half> vec; GVB<__bf16> mtx;
  TLOAD_CUBE(c, gm_c); TLOAD(bias, gm_bias);
  TLOAD_CUBE(vec, gm_vec); TLOAD_CUBE(mtx, gm_mtx);
  TGEMV_MX(d, mtx, vec);
  TGEMV_MX_ACC(d, c, mtx, vec);
  TGEMV_MX_BIAS(d, mtx, vec, bias);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_gemv_scale_a(
    float *output, float *c_input, float *bias_input,
    __fp8_e4m3 *vec_input, __fp8_e8m0 *scale_input,
    __half *mtx_input) {
  GM<float, 1, 16> gm_d(output), gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__fp8_e4m3, 1, 32> gm_vec(vec_input);
  GM<__fp8_e8m0, 16, 8> gm_scale(scale_input);
  GM<__half, 32, 16> gm_mtx(mtx_input);
  GV_D d, c; Bias bias; GVA<__fp8_e4m3> vec; GV_SA scale; GVB<__half> mtx;
  TLOAD_CUBE(c, gm_c); TLOAD(bias, gm_bias);
  TLOAD_CUBE(vec, gm_vec); TLOAD(scale, gm_scale); TLOAD_CUBE(mtx, gm_mtx);
  TGEMV_MX(d, mtx, vec, scale);
  TGEMV_MX_ACC(d, c, mtx, vec, scale);
  TGEMV_MX_BIAS(d, mtx, vec, scale, bias);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_gemv_scale_b(
    float *output, float *c_input, float *bias_input,
    __bf16 *vec_input, __fp8_e5m2 *mtx_input,
    __fp8_e8m0 *scale_input) {
  GM<float, 1, 16> gm_d(output), gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__bf16, 1, 32> gm_vec(vec_input);
  GM<__fp8_e5m2, 32, 16> gm_mtx(mtx_input);
  GM<__fp8_e8m0, 8, 16> gm_scale(scale_input);
  GV_D d, c; Bias bias; GVA<__bf16> vec; GVB<__fp8_e5m2> mtx; GV_SB scale;
  TLOAD_CUBE(c, gm_c); TLOAD(bias, gm_bias);
  TLOAD_CUBE(vec, gm_vec); TLOAD_CUBE(mtx, gm_mtx); TLOAD(scale, gm_scale);
  TGEMV_MX(d, mtx, scale, vec);
  TGEMV_MX_ACC(d, c, mtx, scale, vec);
  TGEMV_MX_BIAS(d, mtx, scale, vec, bias);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_gemv_both_scales(
    float *output, float *c_input, float *bias_input,
    __fp8_e4m3 *vec_input, __fp8_e8m0 *scale_vec_input,
    __fp8_e5m2 *mtx_input, __fp8_e8m0 *scale_mtx_input) {
  GM<float, 1, 16> gm_d(output), gm_c(c_input);
  GM<float, 8, 16> gm_bias(bias_input);
  GM<__fp8_e4m3, 1, 32> gm_vec(vec_input);
  GM<__fp8_e8m0, 16, 8> gm_sv(scale_vec_input);
  GM<__fp8_e5m2, 32, 16> gm_mtx(mtx_input);
  GM<__fp8_e8m0, 8, 16> gm_sm(scale_mtx_input);
  GV_D d, c; Bias bias; GVA<__fp8_e4m3> vec; GV_SA sv;
  GVB<__fp8_e5m2> mtx; GV_SB sm;
  TLOAD_CUBE(c, gm_c); TLOAD(bias, gm_bias);
  TLOAD_CUBE(vec, gm_vec); TLOAD(sv, gm_sv);
  TLOAD_CUBE(mtx, gm_mtx); TLOAD(sm, gm_sm);
  TGEMV_MX(d, mtx, sm, vec, sv);
  TGEMV_MX_ACC(d, c, mtx, sm, vec, sv);
  TGEMV_MX_BIAS(d, mtx, sm, vec, sv, bias);
  TSTORE_CUBE(gm_d, d);
}

using D8 = CubeAccumulatorM16<int8_t, 16, 16>;
using Param = Tile<Location::Vec, uint64_t, 2, 16,
                   BLayout::RowMajor, 1, 16>;
using RowMax = Tile<Location::Vec, float, 16, 8,
                    BLayout::RowMajor, 16, 1>;

__attribute__((noinline)) void carrier_postprocess_all_sources(
    int8_t *output, __fp8_e4m3 *a_input, __fp8_e8m0 *scale_a_input,
    __fp8_e5m2 *b_input, __fp8_e8m0 *scale_b_input,
    uint64_t *quant_input, uint64_t *prelu_input,
    float *row_in_input, float *row_out_input) {
  GM<int8_t, 16, 16> gm_d(output);
  GM<__fp8_e4m3, 16, 32> gm_a(a_input);
  GM<__fp8_e8m0, 16, 8> gm_sa(scale_a_input);
  GM<__fp8_e5m2, 32, 16> gm_b(b_input);
  GM<__fp8_e8m0, 8, 16> gm_sb(scale_b_input);
  GM<uint64_t, 2, 16> gm_q(quant_input), gm_p(prelu_input);
  GM<float, 16, 8> gm_ri(row_in_input), gm_ro(row_out_input);
  D8 d; E4A a; SA sa; E5B b; SB sb; Param q, p; RowMax ri, ro;
  TLOAD_CUBE(a, gm_a); TLOAD(sa, gm_sa);
  TLOAD_CUBE(b, gm_b); TLOAD(sb, gm_sb);
  TLOAD(q, gm_q); TLOAD(p, gm_p); TLOAD(ri, gm_ri); TLOAD(ro, gm_ro);
  TMATMUL_MX(d, a, sa, b, sb,
             fixp::s8(q).prelu(p).row_max(ri, ro));
  TSTORE_CUBE(gm_d, d);
}
