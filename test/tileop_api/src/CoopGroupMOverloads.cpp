// Explicit-groupM cooperative Local-A/Shared-B overloads for the TMATMUL
// family (ADR-0100): every explicit groupM overload encodes LB0 with the
// runtime core-total group_M; D/C valid rows equal the per-PE A shard.
//   FP32:  TMATMUL_ACC / TMATMUL_BIAS          (CubeM16 group_M<=64,
//   MX:    TMATMUL_MX / TMATMUL_MX_ACC /        CubeM32 group_M>64)
//          TMATMUL_MX_BIAS
#include <jcore/template_asm.hpp>

using namespace pto;

namespace fp32_forms {
// FP32 basic 系列
using AL16 = Tile<Location::Left, float, 16, 16, BLayout::CubeM16>;
using AL32 = Tile<Location::Left, float, 32, 16, BLayout::CubeM32>;
using BS = SharedMatrixRight<float, 16, 16>;
using D16 = Tile<Location::Acc, float, 16, 16, BLayout::CubeM16>;
using D32 = Tile<Location::Acc, float, 32, 16, BLayout::CubeM32>;
using Bias = Tile<Location::Bias, float, 1, 16, BLayout::RowMajor>;

__attribute__((noinline)) void acc16(D16 &d, D16 &c, AL16 &a, BS &b) {
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c, a, sb, 64);
}
__attribute__((noinline)) void acc32(D32 &d, D32 &c, AL32 &a, BS &b) {
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_ACC(d, c, a, sb, 128);
}
__attribute__((noinline)) void bias16(D16 &d, AL16 &a, BS &b, Bias &bias) {
  auto sb = TMOV_L2S_INSERT(b);
  TMATMUL_BIAS(d, a, sb, bias, 64);
}



}  // namespace fp32_forms

namespace mx_forms {
template <typename T> using MA16 = Tile<Location::Left, T, 16, 32, BLayout::CubeM16>;
using SA = Tile<Location::Scaling, __fp8_e8m0, 16, 1, BLayout::RowMajor>;
template <typename T> using ShB = SharedMatrixRight<T, 32, 16>;
using ShSB = SharedMatrixRight<__fp8_e8m0, 1, 16, 1, 16>;
using GMF = global_tensor<float, RowMajor<16, 16>>;
using GM16 = global_tensor<__fp8_e4m3, RowMajor<16, 32>>;
using GMB = global_tensor<__fp8_e4m3, RowMajor<32, 16>>;
using GMSB = global_tensor<__fp8_e8m0, RowMajor<1, 16>>;
using D16 = Tile<Location::Acc, float, 16, 16, BLayout::CubeM16>;
using Bias = Tile<Location::Bias, float, 1, 16, BLayout::RowMajor>;

__attribute__((noinline)) void mx(float *out, __fp8_e4m3 *ain, __fp8_e4m3 *bin, __fp8_e8m0 *sbin) {
  GMF gm_d(out); GM16 gm_a(ain); GMB gm_b(bin); GMSB gm_sb(sbin);
  D16 d; MA16<__fp8_e4m3> a; SA sa;
  TLOAD_CUBE(d, gm_d); TLOAD_CUBE(a, gm_a);
  auto b = TLOAD<ShB<__fp8_e4m3>>(gm_b);
  auto sb = TLOAD<ShSB>(gm_sb);
  TMATMUL_MX(d, a, sa, b, sb, 64);
}


}  // namespace mx_forms


namespace options_groupm_forms {
// 用户报错形态: TMATMUL_ACC(tO, tO, tW, tV, pvOptions, kGroupM)
using TW = Tile<Location::Left, __bf16, 32, 128, BLayout::CubeM32, 32, 128>;
using TV = SharedMatrixRight<__bf16, 128, 16>;
using TO = Tile<Location::Acc, float, 32, 16, BLayout::CubeM32, 32, 16>;
constexpr size_t kGroupM = 128;
__attribute__((noinline)) void pv(TO &tO, TW &tW, TV &tV) {
  auto sv = TMOV_L2S_INSERT(tV);
  auto pvOptions = fixp::Options<FixpAttr::keep_acc()>{};
  TMATMUL_ACC(tO, tO, tW, sv, pvOptions, kGroupM);
}


}  // namespace options_groupm_forms

int main() { return 0; }
