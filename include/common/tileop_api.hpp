#ifndef TILEOP_API_HPP
#define TILEOP_API_HPP

#include "common/tileop_api_impl.hpp"

#ifndef __linx
template <is_tile_data_v tile_shape, typename T, int descending = 0>
void TCI(tile_shape &dst, T s) {
  TCI_Impl<tile_shape, T, descending>(dst, s);
}
#ifdef __cpu_sim__
template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD(tile_shape &dst, gm_shape &src) {
  TLOAD_Impl(dst, src);
}
template <is_global_data_v gm_shape, is_tile_data_v tile_shape>
void TSTORE(gm_shape &dst, tile_shape &src) {
  TSTORE_Impl(dst, src);
}
#endif

// TGATHER / TSCATTER: defined as one-layer inline-asm in
// jcore/template_asm.hpp (no __vec__ kernel), so no wrapper here. The
// jcore/cpu_sim *_Impl definitions remain available for callers that
// prefer the kernel-launch form.
#endif
#endif
