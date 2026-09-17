#include <common/pto_tileop.hpp>

using namespace pto;

using Source = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using Destination = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using GlobalSource = global_tensor<float, RowMajor<16, 16>>;

__attribute__((noinline)) void test_trowsum_ass(Source &src, Destination &dst) {
  auto assembled_dst = range::assemble(dst);
  TROWSUM_ASS(src, assembled_dst);
}

__attribute__((noinline)) void test_tload_ass(GlobalSource &src,
                                               Destination &dst) {
  // TLOAD_ASS consumes an already-associated Local Tile.  The initial TLOAD
  // establishes the association before the associated load is issued.
  TLOAD(dst, src);
  TLOAD_ASS(dst, src);
}

int main() {
  float src_data[16 * 16];
  GlobalSource global_src(src_data);
  Source src;
  Destination dst;

  test_trowsum_ass(src, dst);
  test_tload_ass(global_src, dst);

  return 0;
}