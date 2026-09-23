#include <common/pto_tileop.hpp>

using namespace pto;

using MatrixA = SharedMatrixLeft<float, 16, 16>;
using MatrixB = SharedMatrixRight<float, 16, 16>;
using SharedA = SharedTile<MatrixA>;
using SharedB = SharedTile<MatrixB>;
using Acc = CubeAccumulatorM16<float, 16, 16>;
using Aux = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

// Deliberately inconsistent Options: Attr requests no auxiliary input, but
// the concrete type still carries one. LAST_USE must reject the carried type
// even though an Attr-only check would accept it.
using BadOptions = fixp::Options<FixpAttr{}, Aux>;

#ifdef SHOULD_FAIL_LAST_USE_AUX_MATMUL
void bad_matmul(Acc &d, SharedA &a, SharedB &b, Aux &aux) {
  BadOptions options(0, 0, &aux, nullptr, nullptr, nullptr, nullptr);
  TMATMUL_LAST_USE(d, a, b, options);
}
#endif

#ifdef SHOULD_FAIL_LAST_USE_AUX_ACC
void bad_matmul_acc(Acc &d, Acc &c, SharedA &a, SharedB &b, Aux &aux) {
  BadOptions options(0, 0, &aux, nullptr, nullptr, nullptr, nullptr);
  TMATMUL_ACC_LAST_USE(d, c, a, b, options);
}
#endif

int main() { return 0; }
