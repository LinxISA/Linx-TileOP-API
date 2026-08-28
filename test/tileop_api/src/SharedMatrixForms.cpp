// Shared/Local storage-form test: Local-A/Shared-B keeps ordinary M, while
// Shared-A/Shared-B preserves the same logical M/K and K/N rectangles.
// PTO keeps MX scale operands in Local storage even when A/B are Shared.
#include <common/pto_tileop.hpp>

using namespace pto;

using A = CubeTileM16<float, 16, 16>;
using B = SharedMatrixRight<float, 16, 16>;
using SA = Tile<Location::Scaling, float, 16, 16, BLayout::RowMajor>;
using SB = SharedMatrixRight<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using Bias = Tile<Location::Bias, float, 8, 16,
                  BLayout::RowMajor, 1, 16>;

using GroupA = SharedMatrixLeft<float, 16, 16>;
using GroupB = SharedMatrixRight<float, 16, 16>;
using GroupAScale = SharedMatrixLeft<float, 16, 16>;
using GroupBScale = SharedMatrixRight<float, 16, 16>;
using MXA = SharedMatrixLeft<__fp8_e4m3, 16, 16>;
using MXB = SharedMatrixRight<__fp8_e4m3, 16, 16>;
using MXSA = Tile<Location::Scaling, __fp8_e8m0, 16, 8,
                  BLayout::RowMajor, 16, 1>;
using MXSB = Tile<Location::Scaling, __fp8_e8m0, 8, 16,
                  BLayout::ColMajor, 1, 16>;

void shared_b_forms(C &d, C &c, Bias &bias, A &a, B &b, SA &sa, SB &sb) {
  auto keep = fixp::Options<FixpAttr::keep_acc()>{};
  auto shared_b = TMOV_L2S_INSERT(b);

  TMATMUL(d, a, shared_b);
  TMATMUL_ACC(d, c, a, shared_b);
  TMATMUL_BIAS(d, a, shared_b, bias);
  (void)sa;
  (void)sb;
}

void group_forms(C &d, C &c, Bias &bias, GroupA &a, GroupB &b,
                 GroupAScale &sa, GroupBScale &sb) {
  auto keep = fixp::Options<FixpAttr::keep_acc()>{};
  auto shared_a = TMOV_L2S_INSERT(a);
  auto shared_b = TMOV_L2S_INSERT(b);

  TMATMUL(d, shared_a, shared_b);
  TMATMUL_ACC(d, c, shared_a, shared_b);
  TMATMUL_BIAS(d, shared_a, shared_b, bias);
  TMATMUL(d, shared_a, shared_b, keep);
  TMATMUL_ACC(d, c, shared_a, shared_b, keep);
  TMATMUL_BIAS(d, shared_a, shared_b, bias, keep);
  (void)sa;
  (void)sb;
}

int main() { return 0; }
