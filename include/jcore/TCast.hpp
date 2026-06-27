#ifndef TCAST_HPP
#define TCAST_HPP

#include "common/pto_tile.hpp"
#include "jcore/constants.hpp"
#include "template_asm.hpp"

using namespace pto;

template <typename tile_shape_out, typename tile_shape_in>
void __vec__
TCast_RowMajor_Imp(typename tile_shape_out::TileDType __out__ dst,
                   const typename tile_shape_in::TileDType __in__ src) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  using type = typename tile_shape_out::DType;
  size_t idx = j * tile_shape_in::RowStride + i;
  blkv_get_tile_ptr(dst)[idx] = static_cast<type>(blkv_get_tile_ptr(src)[idx]);
}

template <typename tile_shape_out, typename tile_shape_in>
void __vec__
TCast_ColMajor_Imp(typename tile_shape_out::TileDType __out__ dst,
                   const typename tile_shape_in::TileDType __in__ src) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  using type = typename tile_shape_out::DType;
  size_t idx = j * tile_shape_in::ColStride + i;
  blkv_get_tile_ptr(dst)[idx] = static_cast<type>(blkv_get_tile_ptr(src)[idx]);
}

template <typename tile_shape_out, typename tile_shape_in>
void __vec__
TCast_NzLayout_Imp(typename tile_shape_out::TileDType __out__ dst,
                   const typename tile_shape_in::TileDType __in__ src) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  static constexpr int block_cols =
      tile_shape_in::Cols / tile_shape_in::InnerCols;
  __vbuf__ typename tile_shape_out::DType *dst_tile_ptr = blkv_get_tile_ptr(dst);
  __vbuf__ typename tile_shape_in::DType *src_tile_ptr = blkv_get_tile_ptr(src);
  using type = typename tile_shape_out::DType;
#pragma clang loop unroll(full)
  for (size_t k = 0; k < block_cols; ++k) {
    size_t idx =
        k * tile_shape_in::Rows * tile_shape_in::InnerCols + j * LaneNum + i;
    dst_tile_ptr[idx] = static_cast<type>(src_tile_ptr[idx]);
  }
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCAST_Impl(tile_shape_out &dst, tile_shape_in &src) {
  static_assert(tile_shape_in::Rows == tile_shape_out::Rows &&
                    tile_shape_in::Cols == tile_shape_out::Cols,
                "Error! Input shape != Output shape");
  static_assert(tile_shape_in::InnerRows == tile_shape_out::InnerRows &&
                    tile_shape_in::InnerCols == tile_shape_out::InnerCols,
                "Error! Inner shape is not equal!");
  static_assert(tile_shape_out::Loc != Location::Acc &&
                tile_shape_in::Loc != Location::Acc, "Unsupport ACC to be input or output here");
  // TCAST is a pure type cast (no layout conversion); require identical layout
  // on src and dst. The previous SIMT <<<>>> implementation (TCast_*_Imp)
  // crashed the ClockHands pass; use the tileblock-asm TCVT path instead
  // (BSTART.TEPL TCVT + B.DATR SrcType/DstType), which lowers to a single
  // hardware TCVT without launching a SIMT kernel.
  static_assert(tile_shape_in::isRowMajor == tile_shape_out::isRowMajor &&
                is_Nz_layout<tile_shape_in>::value ==
                    is_Nz_layout<tile_shape_out>::value,
                "TCAST requires identical layout on src and dst "
                "(pure type cast, no layout convert)");
  TCVT_T(dst, src);
}

#endif
