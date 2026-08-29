// PTO-ISA 0.58.4 ADR-0098 Shared range modifier coverage.
// Shared source uses B.IOS followed immediately by B.SUBVIEW; Shared
// destination uses B.IOS followed immediately by B.ASSEMBLE.

#include <common/pto_tileop.hpp>

using namespace pto;

using Local = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using Shared = SharedTile<Local>;
using GM = global_tensor<float, RowMajor<4, 8>>;

__attribute__((noinline)) void shared_source_subview(GM &dst, Local &local) {
  auto src = TMOV_L2S_PUBLISH(local);
  auto view = range::subview(src);
  TSTORE(dst, view);
}

__attribute__((noinline)) void shared_source_subview_runtime(
    GM &dst, Local &local, uintptr_t base) {
  auto src = TMOV_L2S_PUBLISH(local);
  auto view = range::subview(src, base);
  TSTORE(dst, view);
}

__attribute__((noinline)) void shared_destination_assemble(GM &src) {
  Shared dst;
  auto assembled = range::assemble_init_last(dst);
  TLOAD(assembled, src);
}

__attribute__((noinline)) void shared_destination_assemble_runtime(
    GM &src, uintptr_t base_units) {
  Shared dst;
  auto assembled = range::assemble_init_last<128, 3>(dst, base_units);
  TLOAD(assembled, src);
}

void use(void *) {}

int main() {
  float src_buf[4 * 8];
  float dst_buf[4 * 8];
  GM src(src_buf);
  GM dst(dst_buf);
  Local local;
  shared_source_subview(dst, local);
  shared_source_subview_runtime(dst, local, 128);
  shared_destination_assemble(src);
  shared_destination_assemble_runtime(src, 4);
  use(src_buf);
  return 0;
}
