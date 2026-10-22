#include <common/pto_tileop.hpp>
using namespace pto;
using Fragment = global_tensor<float, RowMajor<32, 4>>;

// The typed Shared continuation must select the same seven-argument range
// overload while retaining an input-only B.IOS handle.
using SharedParent = SharedTile<SharedMatrixLeft<float, 32, 16>>;
__attribute__((noinline)) void assemble_shared_continuations(
    Fragment &gm, SharedParent &parent) {
  auto middle = range::assemble_middle<4, 4>(parent);
  TLOAD_ASS(middle, gm);
  auto last = range::assemble_last<4, 12>(parent);
  TLOAD_ASS(last, gm);
}
