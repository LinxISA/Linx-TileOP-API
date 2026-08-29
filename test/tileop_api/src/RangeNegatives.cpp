#include <common/pto_tileop.hpp>

using namespace pto;

using Local = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<4, 8>>;

#if defined(SHOULD_FAIL_SUBVIEW_DEST)
void subview_cannot_be_tload_destination(GM &src, Local &dst) {
  range::Subview<Local, 1> view(dst);
  TLOAD(view, src);
}
#endif

#if defined(SHOULD_FAIL_ASSEMBLE_SOURCE)
void assemble_cannot_be_tstore_source(GM &dst, Local &src) {
  range::Assemble<Local, 1> view(src);
  TSTORE(dst, view);
}
#endif

#if defined(SHOULD_FAIL_SUBVIEW_LENGTH)
void subview_length_cannot_exceed_parent(GM &dst, Local &src) {
  auto view = range::subview<256>(src);
  TSTORE(dst, view);
}
#endif

int main() { return 0; }
