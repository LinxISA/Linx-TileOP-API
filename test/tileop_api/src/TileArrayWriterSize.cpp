#include <common/pto_tileop.hpp>

using namespace pto;

// Four 512-byte writers fill one 2 KiB parent.  The B.ASSEMBLE final field
// describes each 512-byte writer (SizeCode 3), while the INIT destination
// binder describes the 2 KiB parent allocation.
using Parent = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor, 32, 16>;
using Fragment = global_tensor<float, RowMajor<32, 4>>;

__attribute__((noinline)) void assemble_writer_size(
    Fragment &gm0, Fragment &gm1, Fragment &gm2, Fragment &gm3,
    Parent &parent) {
  auto first = range::assemble<4, 0>(parent);
  TLOAD(first, gm0);

  auto middle0 = range::assemble_middle<4, 4>(parent);
  TLOAD_ASS(middle0, gm1);

  auto middle1 = range::assemble_middle<4, 8>(parent);
  TLOAD_ASS(middle1, gm2);

  auto last = range::assemble_last<4, 12>(parent);
  TLOAD_ASS(last, gm3);
}

__attribute__((noinline)) void assemble_single_writer(
    Fragment &gm, Parent &parent) {
  auto only = range::assemble_init_last<4, 0>(parent);
  TLOAD(only, gm);
}
