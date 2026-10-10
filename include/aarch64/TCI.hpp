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

template <is_tile_data_v tile_shape, typename T, int row_step, int col_step>
void TCI_2D_Impl(tile_shape &dst, T s) {
  static_assert(std::is_same<typename tile_shape::DType, T>::value,
                "TCI_2D destination and start must have the same type");
  static_assert(row_step >= -1 && row_step <= 1 && col_step >= -1 &&
                    col_step <= 1,
                "TCI_2D steps must be -1, 0, or 1");
  static_assert(tile_shape::IsCubeLayout &&
                    (tile_shape::BFractal == BLayout::CubeM16 ||
                     tile_shape::BFractal == BLayout::CubeM32),
                "TCI_2D requires a CUBE_M16 or CUBE_M32 tile");
  static_assert(tile_shape::ValidRow > 0 && tile_shape::ValidCol > 0 &&
                    tile_shape::ValidRow <= tile_shape::Rows &&
                    tile_shape::ValidCol <= tile_shape::Cols,
                "TCI_2D requires positive valid dimensions within the tile");
  static_assert(std::is_same<T, int32_t>::value ||
                    std::is_same<T, int16_t>::value ||
                    std::is_same<T, uint32_t>::value ||
                    std::is_same<T, uint16_t>::value,
                "TCI_2D supports only S32, S16, U32, and U16");
  for (uint16_t r = 0; r < tile_shape::ValidRow; ++r)
    for (uint16_t c = 0; c < tile_shape::ValidCol; ++c) {
      using U = typename std::make_unsigned<T>::type;
      const int64_t offset = static_cast<int64_t>(r) * row_step +
                             static_cast<int64_t>(c) * col_step;
      const U value = static_cast<U>(s) + static_cast<U>(offset);
      dst.data()[tile_shape::CubeStorageIndex(r, c)] = static_cast<T>(value);
    }
}

#endif
