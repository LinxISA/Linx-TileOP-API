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

template <int = 0>
void zero_scale(D &d, D &c, Bias &bias,
                MA<__half> &f16a, MB<__bf16> &bf16b,
                MA<__bf16> &bf16a, MB<__half> &f16b) {
  TMATMUL_MX(d, f16a, bf16b);
  TMATMUL_MX_ACC(d, c, f16a, bf16b);
  TMATMUL_MX_BIAS(d, f16a, bf16b, bias);
  TMATMUL_MX(d, bf16a, f16b);
  TMATMUL_MX(d, f16a, bf16b, fixp::keep_acc());
}

template <int = 0>
void one_scale_each_side(D &d, D &c, Bias &bias, SA &sa, SB &sb,
                         MA<__fp8_e4m3> &e4a, MB<__half> &f16b,
                         MA<__bf16> &bf16a, MB<__fp8_e5m2> &e5b) {
  TMATMUL_MX(d, e4a, sa, f16b);
  TMATMUL_MX_ACC(d, c, e4a, sa, f16b);
  TMATMUL_MX_BIAS(d, e4a, sa, f16b, bias);
  TMATMUL_MX(d, e4a, sa, f16b, fixp::keep_acc());
  TMATMUL_MX(d, bf16a, e5b, sb);
  TMATMUL_MX_ACC(d, c, bf16a, e5b, sb);
  TMATMUL_MX_BIAS(d, bf16a, e5b, sb, bias);
  TMATMUL_MX(d, bf16a, e5b, sb, fixp::keep_acc());
}

template <int = 0>
void scaled_types_both_sides(
    D &d, SA &sa, SB &sb,
    MA<__fp8_e4m3> &e4a, MB<__fp8_e5m2> &e5b,
    MA<__fp8_e5m2> &e5a, MB<__fp8_e4m3> &e4b,
    MA<__fp4_e2m1x2> &e2a, MB<__fp4_e1m2x2> &e1b,
    MA<__fp4_e1m2x2> &e1a, MB<__fp4_e2m1x2> &e2b) {
  TMATMUL_MX(d, e4a, sa, e5b, sb);
  TMATMUL_MX(d, e5a, sa, e4b, sb);
  TMATMUL_MX(d, e2a, sa, e1b, sb);
  TMATMUL_MX(d, e1a, sa, e2b, sb);
}

using GV_D = CubeAccumulatorM16<float, 1, 16>;
using GV_SA = Tile<Location::Scaling, __fp8_e8m0, 16, 8,
                   BLayout::RowMajor, 1, 1>;
using GV_SB = Tile<Location::Scaling, __fp8_e8m0, 8, 16,
                   BLayout::RowMajor, 1, 16>;
template <typename T> using GVA = CubeTileM16<T, 1, 32>;
template <typename T> using GVB = CubeTileN8<T, 32, 16>;

template <int = 0>
void gemv_scale_variants(GV_D &d, GV_D &c, Bias &bias,
                         GV_SA &sa, GV_SB &sb,
                         GVA<__half> &f16a, GVB<__bf16> &bf16b,
                         GVA<__fp8_e4m3> &e4a, GVB<__half> &f16b,
                         GVA<__bf16> &bf16a, GVB<__fp8_e5m2> &e5b,
                         GVB<__fp8_e5m2> &scaled_b) {
  TGEMV_MX(d, bf16b, f16a);
  TGEMV_MX_ACC(d, c, bf16b, f16a);
  TGEMV_MX_BIAS(d, bf16b, f16a, bias);
  TGEMV_MX(d, bf16b, f16a, fixp::keep_acc());
  TGEMV_MX(d, f16b, e4a, sa);
  TGEMV_MX_ACC(d, c, f16b, e4a, sa);
  TGEMV_MX_BIAS(d, f16b, e4a, sa, bias);
  TGEMV_MX(d, f16b, e4a, sa, fixp::keep_acc());
  TGEMV_MX(d, e5b, sb, bf16a);
  TGEMV_MX_ACC(d, c, e5b, sb, bf16a);
  TGEMV_MX_BIAS(d, e5b, sb, bf16a, bias);
  TGEMV_MX(d, e5b, sb, bf16a, fixp::keep_acc());
  TGEMV_MX(d, scaled_b, sb, e4a, sa);
}

template <typename T, int Rows, int Cols>
using GM = global_tensor<T, RowMajor<Rows, Cols>>;

using F16A = MA<__half>;
using BF16A = MA<__bf16>;
using E4A = MA<__fp8_e4m3>;
using F16B = MB<__half>;
using BF16B = MB<__bf16>;
using E5B = MB<__fp8_e5m2>;

__attribute__((noinline)) void carrier_zero_scale(
    float *output, __half *a_input, __bf16 *b_input) {
  GM<float, 16, 16> gm_d(output);
  GM<__half, 16, 32> gm_a(a_input);
  GM<__bf16, 32, 16> gm_b(b_input);
  D d;
  F16A a;
  BF16B b;
  TLOAD_CUBE(a, gm_a);
  TLOAD_CUBE(b, gm_b);
  TMATMUL_MX(d, a, b);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_scale_a(
    float *output, __fp8_e4m3 *a_input, __fp8_e8m0 *scale_input,
    __half *b_input) {
  GM<float, 16, 16> gm_d(output);
  GM<__fp8_e4m3, 16, 32> gm_a(a_input);
  GM<__fp8_e8m0, 16, 8> gm_sa(scale_input);
  GM<__half, 32, 16> gm_b(b_input);
  D d;
  E4A a;
  SA sa;
  F16B b;
  TLOAD_CUBE(a, gm_a);
  TLOAD(sa, gm_sa);
  TLOAD_CUBE(b, gm_b);
  TMATMUL_MX(d, a, sa, b);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_scale_b(
    float *output, __bf16 *a_input, __fp8_e5m2 *b_input,
    __fp8_e8m0 *scale_input) {
  GM<float, 16, 16> gm_d(output);
  GM<__bf16, 16, 32> gm_a(a_input);
  GM<__fp8_e5m2, 32, 16> gm_b(b_input);
  GM<__fp8_e8m0, 8, 16> gm_sb(scale_input);
  D d;
  BF16A a;
  E5B b;
  SB sb;
  TLOAD_CUBE(a, gm_a);
  TLOAD_CUBE(b, gm_b);
  TLOAD(sb, gm_sb);
  TMATMUL_MX(d, a, b, sb);
  TSTORE_CUBE(gm_d, d);
}

__attribute__((noinline)) void carrier_both_scales(
    float *output, __fp8_e4m3 *a_input, __fp8_e8m0 *scale_a_input,
    __fp8_e5m2 *b_input, __fp8_e8m0 *scale_b_input) {
  GM<float, 16, 16> gm_d(output);
  GM<__fp8_e4m3, 16, 32> gm_a(a_input);
  GM<__fp8_e8m0, 16, 8> gm_sa(scale_a_input);
  GM<__fp8_e5m2, 32, 16> gm_b(b_input);
  GM<__fp8_e8m0, 8, 16> gm_sb(scale_b_input);
  D d;
  E4A a;
  SA sa;
  E5B b;
  SB sb;
  TLOAD_CUBE(a, gm_a);
  TLOAD(sa, gm_sa);
  TLOAD_CUBE(b, gm_b);
  TLOAD(sb, gm_sb);
  TMATMUL_MX(d, a, sa, b, sb);
  TSTORE_CUBE(gm_d, d);
}
