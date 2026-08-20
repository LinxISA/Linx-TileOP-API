#ifndef TCI_HPP
#define TCI_HPP

#include "common/pto_tile.hpp"
#include "jcore/constants.hpp"
using namespace pto;


template <typename tile_shape, int desc>
void __vec__ TCIImpl_RowMajor(typename tile_shape::TileDType __out__ dst,
                                const typename tile_shape::DType __in__ s) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  size_t index = j * tile_shape::RowStride + i;

  using UnsignedDType =
      typename std::make_unsigned<typename tile_shape::DType>::type;
  const UnsignedDType start = static_cast<UnsignedDType>(s);
  const UnsignedDType offset = static_cast<UnsignedDType>(index);
  const UnsignedDType value = desc ? start - offset : start + offset;
  blkv_get_tile_ptr(dst)[index] = static_cast<typename tile_shape::DType>(value);
}
template <typename tile_shape, int desc>
void __vec__ TCIImpl_ColMajor(typename tile_shape::TileDType __out__ dst,
                                const typename tile_shape::DType __in__ s) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  size_t index = j * tile_shape::ColStride + i;

  if constexpr (desc) {
      // 降序：s - index
      blkv_get_tile_ptr(dst)[index] = s - static_cast<typename tile_shape::DType>(index);
  } else {
      // 升序：s + index
      blkv_get_tile_ptr(dst)[index] = s + static_cast<typename tile_shape::DType>(index);
  }
}

template <is_tile_data_v tile_shape, typename T, int descending>
void TCI_Impl(tile_shape &dst, T s) {
  static constexpr size_t row = tile_shape::ValidRow;
  static constexpr size_t col = tile_shape::ValidCol;

  static_assert(std::is_same<typename tile_shape::DType, T>::value, "Dst and scalar must be same data type!");
  static_assert((descending == 0) || (descending == 1), "descending must be 0 or 1!");
  static_assert(row != DYNAMIC && col != DYNAMIC,
              "TODO: Support tile dynamic shape!");
  static_assert(tile_shape::Loc == Location::Vec, "Only VEC tile type are supported");
  static_assert(tile_shape::isRowMajor, "TCI requires RowMajor layout");
  static_assert(row == 1, "TCI requires ValidRow == 1");
  static_assert(col > 0 && tile_shape::Cols >= col,
                "TCI requires 0 < ValidCol <= Cols");
  static_assert(!tile_shape::isBoxedLayout, "TCI not support Boxed Layout!");
if constexpr (std::is_same<typename tile_shape::DType, int32_t>::value ||
              std::is_same<typename tile_shape::DType, uint32_t>::value ||
              std::is_same<typename tile_shape::DType, int16_t>::value ||
              std::is_same<typename tile_shape::DType, uint16_t>::value) {
    TCIImpl_RowMajor<tile_shape, descending><<<col, 1, 1>>>(dst.data(), s);
  } else {
    static_assert(std::is_same<typename tile_shape::DType, int32_t>::value ||
                  std::is_same<typename tile_shape::DType, uint32_t>::value ||
                  std::is_same<typename tile_shape::DType, int16_t>::value ||
                  std::is_same<typename tile_shape::DType, uint16_t>::value,
                  "Data type not supported");
  }
}

#endif
