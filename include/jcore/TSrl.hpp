#ifndef TSRL_HPP
#define TSRL_HPP

#include "common/pto_tile.hpp"
#include "jcore/constants.hpp"
#include <type_traits>
using namespace pto;

// TSRL performs a logical (unsigned) right shift: src0 is treated as unsigned
// before shifting, so the sign bit does not propagate. This matches the
// hardware v.srl semantics (logical shift right).
template <typename tile_shape>
void __vec__ TSrl_Vec_RowMajor(
  typename tile_shape::TileDType __out__ dst,
  const typename tile_shape::TileDType __in__ src0,
  const typename tile_shape::TileDType __in__ src1) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  size_t index = j * tile_shape::RowStride + i;
  using UDType = std::make_unsigned_t<typename tile_shape::DType>;
  blkv_get_tile_ptr(dst)[index] =
      static_cast<typename tile_shape::DType>(
          static_cast<UDType>(blkv_get_tile_ptr(src0)[index]) >>
          blkv_get_tile_ptr(src1)[index]);
}

template <typename tile_shape>
void __vec__ TSrl_Vec_ColMajor(
  typename tile_shape::TileDType __out__ dst,
  const typename tile_shape::TileDType __in__ src0,
  const typename tile_shape::TileDType __in__ src1) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  size_t index = j * tile_shape::ColStride + i;
  using UDType = std::make_unsigned_t<typename tile_shape::DType>;
  blkv_get_tile_ptr(dst)[index] =
      static_cast<typename tile_shape::DType>(
          static_cast<UDType>(blkv_get_tile_ptr(src0)[index]) >>
          blkv_get_tile_ptr(src1)[index]);
}

template <is_tile_data_v tile_shape>
void TSRL_Impl(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  static constexpr size_t row = tile_shape::ValidRow;
  static constexpr size_t col = tile_shape::ValidCol;

  static_assert(tile_shape::Loc == Location::Vec, "Only VEC tile type are supported");
  static_assert(row != DYNAMIC && col != DYNAMIC,
                "TODO: Support tile dynamic shape!");
  static_assert(!tile_shape::isBoxedLayout, "TSRL not support Boxed Layout!");

  if constexpr (std::is_same<typename tile_shape::DType, int64_t>::value ||
                std::is_same<typename tile_shape::DType, int32_t>::value ||
                std::is_same<typename tile_shape::DType, int16_t>::value ||
                std::is_same<typename tile_shape::DType, int8_t>::value ||
                std::is_same<typename tile_shape::DType, unsigned long>::value ||
                std::is_same<typename tile_shape::DType, unsigned int>::value ||
                std::is_same<typename tile_shape::DType, unsigned short>::value ||
                std::is_same<typename tile_shape::DType, unsigned char>::value) {
    if constexpr (tile_shape::isRowMajor) {
      TSrl_Vec_RowMajor<tile_shape><<<col, row, 1>>>
                        (dst.data(), src0.data(), src1.data());
    } else {
      TSrl_Vec_ColMajor<tile_shape><<<row, col, 1>>>
                        (dst.data(), src0.data(), src1.data());
    }
  } else {
    static_assert(std::is_same<typename tile_shape::DType, int64_t>::value ||
                  std::is_same<typename tile_shape::DType, int32_t>::value ||
                  std::is_same<typename tile_shape::DType, int16_t>::value ||
                  std::is_same<typename tile_shape::DType, int8_t>::value ||
                  std::is_same<typename tile_shape::DType, unsigned long>::value ||
                  std::is_same<typename tile_shape::DType, unsigned int>::value ||
                  std::is_same<typename tile_shape::DType, unsigned short>::value ||
                  std::is_same<typename tile_shape::DType, unsigned char>::value,
                  "Only int data type are supported");
  }
}

#endif
