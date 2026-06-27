#ifndef TSRL_HPP
#define TSRL_HPP

#include "common/pto_tile.hpp"
#include "jcore/constants.hpp"
#include <type_traits>
using namespace pto;

// TSRL performs a uniform logical (unsigned) right shift of the whole tile by
// a scalar shift amount: dst[i] = (unsigned)src[i] >> shamt. The shift amount
// is a scalar (compile-time constant or runtime uniform value), NOT a per-lane
// tile. src is converted to its unsigned counterpart before shifting so the
// sign bit does not propagate, matching the hardware v.srli/v.srl semantics.
// When shamt is a compile-time constant, ISel lowers to v.srli; when it is a
// runtime scalar, to v.srl (shift amount masked mod 32/64).
template <typename tile_shape>
void __vec__ TSrl_Vec_RowMajor(
  typename tile_shape::TileDType __out__ dst,
  const typename tile_shape::TileDType __in__ src,
  unsigned shamt) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  size_t index = j * tile_shape::RowStride + i;
  using UDType = std::make_unsigned_t<typename tile_shape::DType>;
  blkv_get_tile_ptr(dst)[index] =
      static_cast<typename tile_shape::DType>(
          static_cast<UDType>(blkv_get_tile_ptr(src)[index]) >> shamt);
}

template <typename tile_shape>
void __vec__ TSrl_Vec_ColMajor(
  typename tile_shape::TileDType __out__ dst,
  const typename tile_shape::TileDType __in__ src,
  unsigned shamt) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  size_t index = j * tile_shape::ColStride + i;
  using UDType = std::make_unsigned_t<typename tile_shape::DType>;
  blkv_get_tile_ptr(dst)[index] =
      static_cast<typename tile_shape::DType>(
          static_cast<UDType>(blkv_get_tile_ptr(src)[index]) >> shamt);
}

template <is_tile_data_v tile_shape>
void TSRL_Impl(tile_shape &dst, tile_shape &src, unsigned shamt) {
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
                        (dst.data(), src.data(), shamt);
    } else {
      TSrl_Vec_ColMajor<tile_shape><<<row, col, 1>>>
                        (dst.data(), src.data(), shamt);
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
