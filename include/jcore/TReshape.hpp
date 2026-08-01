#ifndef TRESHAPE_HPP
#define TRESHAPE_HPP

#include "common/pto_tile.hpp"

using namespace pto;

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TRESHAPE_Impl(tile_shape_out &tile_out, tile_shape_in &tile_in) {
  static_assert(tile_shape_in::ValidRow != DYNAMIC && tile_shape_in::ValidCol != DYNAMIC &&
                tile_shape_out::ValidRow != DYNAMIC && tile_shape_out::ValidCol != DYNAMIC,
              "TODO: Support tile dynamic shape!");
  tile_out.data() = tile_in.data();
}

#endif