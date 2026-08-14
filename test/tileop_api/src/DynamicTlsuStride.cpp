#include <common/pto_tileop.hpp>

using namespace pto;

using TileF =
    Tile<Location::Vec, float, 2, 1024, BLayout::RowMajor, -1, -1>;
using GlobalF = global_tensor<float, RowMajor<-1, -1>>;

__attribute__((noinline)) void dynamic_tlsu_stride(float *input, float *output,
                                                   int rows, int input_cols,
                                                   int output_cols) {
  GlobalF src(input, rows, input_cols);
  GlobalF dst(output, rows, output_cols);
  TileF tile(rows, input_cols);
  TLOAD(tile, src);
  TSTORE(dst, tile);
}

int main() { return 0; }
