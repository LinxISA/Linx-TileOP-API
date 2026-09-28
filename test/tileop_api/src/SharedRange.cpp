// PTO-ISA 0.58.5+ Shared per-PE range modifier coverage.  Shared source
// uses B.IOS followed immediately by B.SUBVIEW.  Destination B.ASSEMBLE is
// covered by the Local range fixtures; the current target API intentionally
// does not combine it with a Shared CUBE carrier.

#include <common/pto_tileop.hpp>

using namespace pto;

using SourceLocal = VecTileM32<float, 32, 1, 32, 1>;
using SourceGM = global_tensor<float, RowMajor<32, 1>>;

__attribute__((noinline)) void shared_source_subview(SourceGM &dst,
                                                      SourceLocal &local) {
  auto src = TMOV_L2S_PUBLISH(local);
  auto view = range::subview(src);
  TSTORE(dst, view);
}

__attribute__((noinline)) void shared_source_subview_runtime(
    SourceGM &dst, SourceLocal &local, uintptr_t base) {
  auto src = TMOV_L2S_PUBLISH(local);
  auto view = range::subview(src, base);
  TSTORE(dst, view);
}

void use(void *) {}

int main() {
  float src_buf[32];
  float dst_buf[32];
  SourceGM dst(dst_buf);
  SourceLocal local;
  shared_source_subview(dst, local);
  shared_source_subview_runtime(dst, local, 128);
  use(dst_buf);
  return 0;
}
