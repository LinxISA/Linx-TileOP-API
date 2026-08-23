#include <common/pto_tileop.hpp>

using namespace pto;

// Stored A is KxM and stored B is NxK. TransA/TransB materialize the logical
// MxK and KxN primaries; D closes against the effective 16x24 valid shape.
using StoredA = SharedMatrixLeft<float, 32, 16>;
using StoredB = SharedMatrixRight<float, 32, 32, 24, 32>;
using D = CubeAccumulatorM16<float, 16, 24>;
using RowMax = Tile<Location::Vec, float, 16, 8,
                    BLayout::RowMajor, 16, 1>;
using GroupMax = Tile<Location::Vec, float, 16, 8,
                      BLayout::RowMajor, 16, 3>;

void non_square_transpose(D &d, StoredA &stored_a, StoredB &stored_b,
                          RowMax &row_max, GroupMax &group_max) {
  auto shared_a = TMOV_L2S_INSERT(stored_a);
  auto shared_b = TMOV_L2S_INSERT(stored_b);
  TMATMUL(d, shared_a, shared_b,
          fixp::keep_acc()
              .row_max(row_max)
              .group_max<8>(group_max)
              .transpose_a().transpose_b());
}
