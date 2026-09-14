// Issue #129 / #28: common consumer macros must not rewrite inline-asm labels.
// Instantiate the affected API with the same -DDType=__half macro used by
// downstream kernels.  The destination is assembled, as required by _ASS.
#include <common/pto_tileop.hpp>

using namespace pto;

using TileT = Tile<Location::Vec, float, 16, 16>;

__attribute__((noinline)) void tcvt_ass_macro_regression(TileT &src,
                                                          TileT &storage) {
  auto dst = range::assemble(storage);
  TCVT_ASS(dst, src);
}

int main() {
  TileT src, storage;
  tcvt_ass_macro_regression(src, storage);
  return 0;
}
