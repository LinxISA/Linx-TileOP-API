// PTO-ISA 0.58.5+ Shared per-PE range modifier coverage.
// Shared source uses B.IOS followed immediately by B.SUBVIEW; Shared
// destination uses B.IOS followed immediately by B.ASSEMBLE.

#include <common/pto_tileop.hpp>

using namespace pto;

using SourceLocal = VecTileM32<float, 32, 1, 32, 1>;
using DestinationLocal = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using SharedDestination = SharedTile<DestinationLocal>;
using SourceGM = global_tensor<float, RowMajor<32, 1>>;
using DestinationGM = global_tensor<float, RowMajor<4, 8>>;

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

__attribute__((noinline)) void shared_destination_assemble(DestinationGM &src) {
  SharedDestination dst;
  auto assembled = range::assemble_init_last(dst);
  TLOAD(assembled, src);
}

__attribute__((noinline)) void shared_destination_assemble_ass(
    DestinationGM &src) {
  SharedDestination dst;
  // The INIT slot is written by the plain allocating TLOAD; TLOAD_ASS only
  // consumes a non-INIT (middle/last) carrier of the same parent.
  auto init = range::assemble(dst);
  TLOAD(init, src);
  auto assembled = range::assemble_last(dst);
  TLOAD_ASS(assembled, src); // Existing Shared handle is a B.IOS source.
}

__attribute__((noinline)) void shared_destination_assemble_runtime(
    DestinationGM &src, uintptr_t base_units) {
  SharedDestination dst;
  auto assembled = range::assemble_init_last<1, 3>(dst, base_units);
  TLOAD(assembled, src);
}

void use(void *) {}

int main() {
  float src_buf[32];
  float dst_buf[32];
  DestinationGM src(src_buf);
  SourceGM dst(dst_buf);
  SourceLocal local;
  shared_source_subview(dst, local);
  shared_source_subview_runtime(dst, local, 128);
  shared_destination_assemble(src);
  shared_destination_assemble_runtime(src, 4);
  use(src_buf);
  return 0;
}
