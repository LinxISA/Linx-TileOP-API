#ifndef TILEOP_API_HPP
#define TILEOP_API_HPP

#include "common/tileop_api_impl.hpp"

#ifndef __linx
template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_B,
              is_tile_data_v tile_shape_C>
void MATMACC(tile_shape_C &dst, tile_shape_A &src0, tile_shape_B &src1) {
  MATMACC_Impl(dst, src0, src1);
}
template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_B,
          is_tile_data_v tile_shape_C>
void MATMUL(tile_shape_C &dst, tile_shape_A &src0, tile_shape_B &src1) {
  MATMUL_Impl(dst, src0, src1);
}

template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_AX,
          is_tile_data_v tile_shape_B, is_tile_data_v tile_shape_BX,
          is_tile_data_v tile_shape_C>
void MATMULMX(tile_shape_C &dst, tile_shape_A &src0, tile_shape_AX &src0_scale,
             tile_shape_B &src1, tile_shape_BX &src1_scale) {
  MATMULMX_Impl(dst, src0, src0_scale, src1, src1_scale);
}

template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_B,
          is_tile_data_v tile_shape_BX, is_tile_data_v tile_shape_C>
void MATMULMXB(tile_shape_C &dst, tile_shape_A &src0, tile_shape_B &src1, tile_shape_BX &src1_scale) {
  MATMULMXB_Impl(dst, src0, src1, src1_scale);
}

template <is_tile_data_v tile_shape_A,  is_tile_data_v tile_shape_AX,
          is_tile_data_v tile_shape_B,  is_tile_data_v tile_shape_BX,
          is_tile_data_v tile_shape_C>
void MATMACCMX(tile_shape_C &dst, tile_shape_A &src0,  tile_shape_AX &src0x,
               tile_shape_B &src1,  tile_shape_BX &src1x) {
  MATMACCMX_Impl(dst, src0, src0x, src1, src1x);
}

template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_B, 
          is_tile_data_v tile_shape_BX, is_tile_data_v tile_shape_C>
void MATMACCMXB(tile_shape_C &dst, tile_shape_A &src0,
                tile_shape_B &src1, tile_shape_BX &src1x) {
  MATMACCMXB_Impl(dst, src0, src1, src1x);
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCAST(tile_shape_out &dst, tile_shape_in &src) {
  TCAST_Impl(dst, src);
}
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TRESHAPE(tile_shape_out &dst, tile_shape_in &src) {
  TRESHAPE_Impl(dst, src);
}
template <is_tile_data_v tile_shape, typename T, int descending>
void TCI(tile_shape &dst, T s) {
  TCI_Impl<tile_shape, T, descending>(dst, s);
}
template <is_tile_data_v tile_shape>
void TCOPY(tile_shape &dst, tile_shape &src) {
  TCOPY_Impl(dst, src);
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
