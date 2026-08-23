#include <common/pto_tileop.hpp>

using namespace pto;

using TileF = Tile<Location::Vec, float, 16, 32, BLayout::RowMajor, -1, -1>;
using RowF = Tile<Location::Vec, float, 16, 32, BLayout::RowMajor, -1, 1>;
using GlobalF = global_tensor<float, RowMajor<-1, -1>>;

__attribute__((noinline)) void dynamic_elementwise(float *output, float *input0,
                                                   float *input1, int rows,
                                                   int cols) {
  GlobalF gm_dst(output, rows, cols);
  GlobalF gm_src0(input0, rows, cols);
  GlobalF gm_src1(input1, rows, cols);
  TileF dst(rows, cols);
  TileF src0(rows, cols);
  TileF src1(rows, cols);
  TLOAD(src0, gm_src0);
  TLOAD(src1, gm_src1);
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
  TSTORE(gm_dst, dst);
}

__attribute__((noinline)) void dynamic_reduce_broadcast(float *output,
                                                        float *input, int rows,
                                                        int cols) {
  GlobalF gm_dst(output, rows, cols);
  GlobalF gm_src(input, rows, cols);
  RowF row(rows);
  TileF dst(rows, cols);
  TileF src(rows, cols);
  TLOAD(src, gm_src);
  TROWSUM(row, src);
  TROWMAX(row, src);
  TROWEXPANDMUL(dst, src, row);
  TROWEXPANDADD(dst, src, row);
  TSTORE(gm_dst, dst);
}

__attribute__((noinline)) void dynamic_tlsu(float *input, float *output,
                                            int rows, int cols) {
  GlobalF src(input, rows, cols);
  GlobalF dst(output, rows, cols);
  TileF tile(rows, cols);
  TLOAD(tile, src);
  TSTORE(dst, tile);
}

int main() { return 0; }
