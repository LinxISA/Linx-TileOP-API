#ifndef TCI_HPP
#define TCI_HPP

#include "common/pto_tile.hpp"

using namespace pto;

template <typename tile_shape, int desc>
void TCI_AArch64_Impl(typename tile_shape::TileDType dst,
                     const typename tile_shape::DType s) {
  for (uint16_t i = 0; i < tile_shape::ValidRow; ++i)
    for (uint16_t j = 0; j < tile_shape::ValidCol; ++j) {
        size_t index = i * tile_shape::RowStride + j;
        using UnsignedDType =
            typename std::make_unsigned<typename tile_shape::DType>::type;
        const UnsignedDType start = static_cast<UnsignedDType>(s);
        const UnsignedDType offset = static_cast<UnsignedDType>(index);
        const UnsignedDType value = desc ? start - offset : start + offset;
        dst[index] = static_cast<typename tile_shape::DType>(value);
    }
}

template <is_tile_data_v tile_shape, typename T, int descending>
void TCI_Impl(tile_shape &dst, T s) {
  static_assert(std::is_same<typename tile_shape::DType, T>::value,
                "TCI destination and start must have the same type");
  static_assert(descending == 0 || descending == 1,
                "TCI direction must be ascending (0) or descending (1)");
  static_assert(tile_shape::Loc == Location::Vec && tile_shape::isRowMajor &&
                    !tile_shape::isBoxedLayout,
                "TCI requires an unboxed Local RowMajor tile");
  static_assert(tile_shape::ValidRow == 1 && tile_shape::ValidCol > 0 &&
                    tile_shape::Cols >= tile_shape::ValidCol,
                "TCI requires one valid row and 0 < ValidCol <= Cols");
  static_assert(std::is_same<T, int32_t>::value ||
                    std::is_same<T, int16_t>::value ||
                    std::is_same<T, uint32_t>::value ||
                    std::is_same<T, uint16_t>::value,
                "TCI supports only S32, S16, U32, and U16");
  TCI_AArch64_Impl<tile_shape, descending>(dst.data(), s);
}

#endif
