#include <common/pto_tileop.hpp>

using namespace pto;

using Local = CubeTileM16<float, 16, 16>;
using GM = global_tensor<float, RowMajor<16, 16>>;

#if defined(SHOULD_FAIL_TADD_ASS_INIT) || defined(SHOULD_FAIL_TADD_MIDDLE) || \
    defined(SHOULD_FAIL_TADD_ASSEMBLE_DTYPE) ||                              \
    defined(SHOULD_FAIL_TADD_ASSEMBLE_SHAPE)
using VecTile = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
void reject_tadd_range(VecTile &a, VecTile &b, VecTile &d) {
  auto init = range::assemble(d);
#if defined(SHOULD_FAIL_TADD_ASS_INIT)
  TADD_ASS(init, a, b);
#elif defined(SHOULD_FAIL_TADD_MIDDLE)
  auto middle = range::assemble_middle(d);
  TADD(middle, a, b);
#elif defined(SHOULD_FAIL_TADD_ASSEMBLE_DTYPE)
  Tile<Location::Vec, int32_t, 4, 8, BLayout::RowMajor> wrong_dtype;
  TADD(init, a, wrong_dtype);
#else
  Tile<Location::Vec, float, 4, 4, BLayout::RowMajor> wrong_destination;
  auto wrong_shape = range::assemble(wrong_destination);
  TADD(wrong_shape, a, b);
#endif
}
#endif

#if defined(SHOULD_FAIL_TLOAD_ASS_TYPED_INIT)
void reject_typed_local_init(GM &src) {
  Tile<Location::Vec, float, 16, 16, BLayout::RowMajor> parent;
  auto init = range::assemble<1>(parent);
  TLOAD_ASS(init, src);
}
#endif

#if defined(SHOULD_FAIL_TLOAD_ASS_SHARED_TYPED_INIT)
void reject_typed_shared_init(GM &src) {
  SharedTile<SharedMatrixLeft<float, 16, 16>> parent;
  auto init = range::assemble<1>(parent);
  TLOAD_ASS(init, src);
}
#endif

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
  auto view = range::subview<2048>(src);
  TSTORE(dst, view);
}
#endif

int main() { return 0; }
