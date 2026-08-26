// PTO-ISA 0.58.4 ADR-0098 Shared range modifier coverage.
// Shared source uses B.IOS followed immediately by B.SUBVIEW; Shared
// destination uses B.IOS followed immediately by B.ASSEMBLE.

#include <common/pto_tileop.hpp>

using namespace pto;

using Local = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using Shared = SharedTile<Local>;
using GM = global_tensor<float, RowMajor<4, 8>>;

__attribute__((noinline)) void shared_source_subview(GM &dst, Shared &src) {
  range::Subview<Shared, 12, 2047, 23> view(src, 23);
  TSTORE(dst, view);
}

__attribute__((noinline)) void shared_destination_assemble(GM &src,
                                                            Shared &dst) {
  range::Assemble<Shared, 12, true, false, 0, 0> assembled(dst, 0);
  TLOAD(assembled, src);
}

void use(void *) {}

int main() {
  float src_buf[4 * 8];
  float dst_buf[4 * 8];
  GM src(src_buf);
  GM dst(dst_buf);
  Shared shared;
  shared_source_subview(dst, shared);
  shared_destination_assemble(src, shared);
  use(src_buf);
  return 0;
}
