// Shared->GM TSTORE (TLSU fn1 Shared form, B.IOS PE_MASK=1111) and
// TSTORE.SPART (fn14, explicit nonzero PE subset).
#include <common/pto_tileop.hpp>

using namespace pto;

using T = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<8, 256>>;
using TMin = Tile<Location::Vec, float, 1, 8, BLayout::RowMajor>;
using GMMin = global_tensor<float, RowMajor<1, 8>>;

// A 32 B valid payload occupies the minimum architectural 128 B carrier and
// therefore uses the assigned Shared SizeCode 1.
__attribute__((noinline)) void min_capacity(float *output, float *input) {
  GMMin g(output);
  GMMin src(input);
  TMin t;
  TLOAD(t, src);
  auto sh = TMOV_L2S_INSERT(t);
  TSTORE(g, sh);
}

__attribute__((noinline)) void full(float *output, float *input) {
  GM g(output);
  GM src(input);
  T t;
  TLOAD(t, src);
  auto sh = TMOV_L2S_INSERT(t);
  TSTORE(g, sh);
}
__attribute__((noinline)) void partial(float *output, float *input) {
  GM g(output);
  GM src(input);
  T t;
  TLOAD(t, src);
  auto sh = TMOV_L2S_INSERT(t);
  TSTORE_PART<12>(g, sh);  // fixed PEMode mask 1100
}
// DYNAMIC runtime valid shape (reduced valid rectangle 4x128 of 8x256):
// the bundle's B.DIM must come from the Shared tile's runtime valid shape.
__attribute__((noinline)) void dyn_valid(float *output, float *input) {
  GM g(output);
  GM src(input);
  Tile<Location::Vec, float, 8, 256, BLayout::RowMajor, DYNAMIC,
       DYNAMIC> t(4, 128);
  TLOAD(t, src);
  auto sh = TMOV_L2S_INSERT(t);
  TSTORE(g, sh);
}
