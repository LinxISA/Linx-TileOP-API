// Object-level lifetime coverage. An OPEN assembly parent may not escape
// through a Tile reference or ordinary Tile store before LAST.
#include <common/pto_tileop.hpp>

using namespace pto;
using Part = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using Parent = Tile<Location::Vec, float, 8, 8, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<4, 8>>;

#if defined(TEST_OPEN_ESCAPE)
__attribute__((noinline)) void open_escape(GM &src, Parent &out) {
  auto init = range::assemble(out);
  TLOAD(init, src);
}
#elif defined(TEST_OPEN_STORE)
__attribute__((noinline)) void open_store_before_last(GM &src, float *dst) {
  Parent parent;
  auto init = range::assemble(parent);
  TLOAD(init, src);
  global_tensor<float, RowMajor<8, 8>> output(dst);
  TSTORE(output, parent);
  auto last = range::assemble_last(parent);
  TLOAD_ASS(last, src);
}
#else
__attribute__((noinline)) void closed_load_session(GM &src, Parent &out) {
  Parent parent;
  auto init = range::assemble(parent);
  TLOAD(init, src);
  auto last = range::assemble_last(parent);
  TLOAD_ASS(last, src);
  TMOV(out, parent);
}

__attribute__((noinline)) void closed_tadd_session(Part &a, Part &b, Part &out) {
  Part parent;
  auto init = range::assemble(parent);
  TADD(init, a, b);
  auto middle = range::assemble_middle(parent);
  TADD_ASS(middle, a, b);
  auto last = range::assemble_last(parent);
  TADD_ASS(last, a, b);
  TMOV(out, parent);
}
#endif
