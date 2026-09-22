#include <common/pto_tileop.hpp>

using namespace pto;

using Source = CubeTileM32<__bf16, 32, 32>;
using Destination = CubeTileM32<__fp4_e2m1x2, 32, 32>;

void convert_cube_tile_array(TileArray<Destination, 1, 1> &destination,
                             Source &source) {
  TCVT(destination[0][0], source);
}
