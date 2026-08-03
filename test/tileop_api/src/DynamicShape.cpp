#include <common/pto_tileop.hpp>

using namespace pto;

using TileF = Tile<Location::Vec, float, 16, 32, BLayout::RowMajor, -1, -1>;
using RowF = Tile<Location::Vec, float, 16, 32, BLayout::RowMajor, -1, 1>;
using LeftF = TileLeft<float, 16, 32, -1, -1>;
using RightF = TileRight<float, 32, 16, -1, -1>;
using MatOutF = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, -1, -1>;
using GlobalF = global_tensor<float, RowMajor<-1, -1>>;

__attribute__((noinline)) void dynamic_elementwise(TileF &dst, TileF &src0,
                                                   TileF &src1) {
  TADD(dst, src0, src1);
  TSUB(dst, src0, src1);
  TMUL(dst, src0, src1);
  TDIV(dst, src0, src1);
  TMAX(dst, src0, src1);
  TMIN(dst, src0, src1);
  TABS(dst, src0);
  TEXP(dst, src0);
  TRECIP(dst, src0);
  TSQRT(dst, src0);
  TADDS(dst, src0, 1.0f);
  TMULS(dst, src0, 2.0f);
}

__attribute__((noinline)) void dynamic_reduce_broadcast(RowF &row, TileF &dst,
                                                        TileF &src) {
  TROWSUM(row, src);
  TROWMAX(row, src);
  TROWEXPANDMUL(dst, src, row);
  TROWEXPANDADD(dst, src, row);
}

__attribute__((noinline)) void dynamic_tlsu(float *input, float *output,
                                            int rows, int cols) {
  GlobalF src(input, rows, cols);
  GlobalF dst(output, rows, cols);
  TileF tile(rows, cols);
  TLOAD(tile, src);
  TSTORE(dst, tile);
}

__attribute__((noinline)) void dynamic_matmul(MatOutF &c, LeftF &a, RightF &b) {
  auto shared_a = TMOV_L2S_PUBLISH(a);
  auto shared_b = TMOV_L2S_PUBLISH(b);
  TMATMUL(c, shared_a, shared_b);
}

int main() { return 0; }
