#include <common/pto_tileop.hpp>

using namespace pto;

// Issue #195: the convolution composition (weight TLOAD -> TIMG2COL ->
// cooperative TMATMUL) needs (1) the weight parameter words to stay in real
// GPRs so the three-source B.IOR keeps GMBase/ShapeGPR/StartGPR even when
// StartWord folds to the constant zero, and (2) a TIMG2COL overload whose
// LB1 carries the core-total group rows at runtime while the destination
// type stays the per-PE shard.

using WeightOut = Tile<Location::Right, float, 16, 64, BLayout::RowMajor, 16, 64>;
using WeightShared = SharedTile<WeightOut>;
using WeightGM = global_tensor<float, RowMajor<16, 64>>;
using InputGM = global_tensor<float, RowMajor<8, 256>>;
using tileAShard = CubeTileM16<float, 16, 64>;
using AccC = CubeAccumulatorM16<float, 16, 16>;

__attribute__((noinline)) void weight_tload_zero_start(WeightShared &dst,
                                                       const WeightGM &src) {
  // start words default to zero: the first weight group.
  TLOAD<OIHW2NK>(dst, src, make_weight_tload_params(16, 16, 1, 1));
}

__attribute__((noinline)) void timg2col_group_rows(tileAShard &dst,
                                                   InputGM &src,
                                                   size_t groupRows) {
  TIMG2COL(dst, src, 0x0008000800040008ULL, 0, 0, groupRows);
}

__attribute__((noinline)) void conv_compose(tileAShard &tA, AccC &tC,
                                            WeightShared &tW, InputGM &gA,
                                            WeightGM &gW, size_t groupRows,
                                            size_t groupM) {
  TIMG2COL(tA, gA, 0x0008000800040008ULL, 0, 0, groupRows);
  TLOAD<OIHW2NK>(tW, gW, make_weight_tload_params(16, 16, 1, 1));
  TMATMUL(tC, tA, tW, fixp::keep_acc(), groupM);
}
