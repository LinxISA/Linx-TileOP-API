#ifndef PTO_TILE_REGION_INLINE_ASM_HPP
#define PTO_TILE_REGION_INLINE_ASM_HPP

#include "common/pto_tile_region.hpp"

namespace pto {

#define PTO_REGION_ALWAYS_INLINE __attribute__((always_inline)) inline

template <int Opcode, typename Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void
pto_region_unary(Out &dst, region::SubTileView<Parent, SubTile> &src) {
  static_assert(SubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  const uintptr_t region_base_units = src.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c8, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %2, mask=1111, last, ->%0<%Z6>\n"
      "B.SUBVIEW 0, %9, 0, %c7\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "Tr"(src.data()), "i"(std::remove_reference_t<decltype(src)>::ValidCol),
        "i"(std::remove_reference_t<decltype(src)>::ValidRow),
        "i"(SubTile::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(Opcode), "r"(region_base_units)
      : "memory");
}

template <int Opcode, typename Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar(
    Out &dst, region::SubTileView<Parent, SubTile> &src,
    typename SubTile::DType scalar) {
  static_assert(SubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  volatile typename SubTile::DType value = scalar;
  const uintptr_t region_base_units = src.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c10, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %2, mask=1111, last, ->%0<%Z6>\n"
      "B.SUBVIEW 0, %9, 0, %c7\n"
      "B.IOR [%8],[]\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "Tr"(src.data()), "i"(std::remove_reference_t<decltype(src)>::ValidCol),
        "i"(std::remove_reference_t<decltype(src)>::ValidRow),
        "i"(SubTile::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "r"(value), "r"(region_base_units), "i"(Opcode)
      : "memory");
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void TROWMAX(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<65>(dst, src);
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void TROWSUM(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<64>(dst, src);
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void TEXP(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<18>(dst, src);
}

template <int ParentSize, bool Init, bool Last, int Opcode,
          typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void pto_region_unary_assemble(
    region::TileArrayOutputRef<SubTile> &dst, In &src) {
  static_assert(SubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == In::Rows && SubTile::Cols == In::Cols,
                "TileArray slot requires matching physical shape");
  static_assert(SubTile::ValidRow == In::ValidRow &&
                    SubTile::ValidCol == In::ValidCol,
                "TileArray slot requires matching valid shape");
  static_assert(std::is_same_v<typename SubTile::DType, typename In::DType>,
                "TileArray slot requires matching element types");
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t range_base_units = dst.range_base_units();
#define PTO_REGION_UNARY_ASSEMBLY_BODY                                      \
  "BSTART.TEPL %c7, %D1\n"                                                \
  "B.DIM zero, %c3, ->lb0\n"                                                    \
  "B.DIM zero, %c4, ->lb1\n"                                                    \
  "B.DIM zero, %c5, ->lb2\n"                                                \
  "B.IOT %2, mask=1111, last, ->%0<%Z6>\n"                                   \
  "B.ASSEMBLE %c10, %c11, %8, 0, %c9\n"
#define PTO_REGION_UNARY_ASSEMBLY_INPUTS                                   \
  "i"(type_traits<typename In::DType>::TypeCode),                          \
  "Tr"(src.data()),                                                       \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                  \
  "i"(SubTile::Cols),                                                       \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),         \
  "i"(Opcode), "r"(range_base_units), "i"(encoded_parent_size),          \
  "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_UNARY_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_UNARY_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_UNARY_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_UNARY_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_UNARY_ASSEMBLY_INPUTS
#undef PTO_REGION_UNARY_ASSEMBLY_BODY
}

template <int ParentSize, int Opcode, typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void pto_region_unary_assemble_phase(
    region::TileArrayOutputRef<SubTile> &dst, In &src) {
  if (dst.slot_count() == 1)
    pto_region_unary_assemble<ParentSize, true, true, Opcode>(dst, src);
  else if (dst.ordinal() == 0)
    pto_region_unary_assemble<ParentSize, true, false, Opcode>(dst, src);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_unary_assemble<ParentSize, false, true, Opcode>(dst, src);
  else
    pto_region_unary_assemble<ParentSize, false, false, Opcode>(dst, src);
}

template <int Opcode, typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void pto_region_unary_assemble_dispatch(
    region::TileArrayOutputRef<SubTile> &dst, In &src) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_UNARY_PARENT_CASE(N)                                     \
  case N:                                                                   \
    pto_region_unary_assemble_phase<N, Opcode>(dst, src);                    \
    break
    PTO_REGION_UNARY_PARENT_CASE(1);
    PTO_REGION_UNARY_PARENT_CASE(2);
    PTO_REGION_UNARY_PARENT_CASE(3);
    PTO_REGION_UNARY_PARENT_CASE(4);
    PTO_REGION_UNARY_PARENT_CASE(5);
    PTO_REGION_UNARY_PARENT_CASE(6);
    PTO_REGION_UNARY_PARENT_CASE(7);
    PTO_REGION_UNARY_PARENT_CASE(8);
    PTO_REGION_UNARY_PARENT_CASE(9);
    PTO_REGION_UNARY_PARENT_CASE(10);
    PTO_REGION_UNARY_PARENT_CASE(11);
    PTO_REGION_UNARY_PARENT_CASE(12);
#undef PTO_REGION_UNARY_PARENT_CASE
  default:
    __builtin_trap();
  }
}

template <int ParentSize, bool Init, bool Last, int Opcode,
          typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar_assemble(
    region::TileArrayOutputRef<SubTile> &dst, In &src,
    typename In::DType scalar) {
  static_assert(SubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == In::Rows && SubTile::Cols == In::Cols,
                "TileArray slot requires matching physical shape");
  static_assert(std::is_same_v<typename SubTile::DType, typename In::DType>,
                "TileArray slot requires matching element types");
  volatile typename In::DType value = scalar;
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t range_base_units = dst.range_base_units();
#define PTO_REGION_SCALAR_ASSEMBLY_BODY                                    \
  "BSTART.TEPL %c7, %D1\n"                                                \
  "B.DIM zero, %c3, ->lb0\n"                                                  \
  "B.DIM zero, %c4, ->lb1\n"                                                  \
  "B.DIM zero, %c5, ->lb2\n"                                              \
  "B.IOT %2, mask=1111, last, ->%0<%Z6>\n"                                 \
  "B.IOR [%12],[]\n"                                                       \
  "B.ASSEMBLE %c10, %c11, %8, 0, %c9\n"
#define PTO_REGION_SCALAR_ASSEMBLY_INPUTS                                  \
  "i"(type_traits<typename In::DType>::TypeCode),                         \
  "Tr"(src.data()),                                                       \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                  \
  "i"(SubTile::Cols),                                                       \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),         \
  "i"(Opcode), "r"(range_base_units), "i"(encoded_parent_size),            \
  "i"(Init), "i"(Last), "r"(value)
  if constexpr (Init) {
    asm volatile(PTO_REGION_SCALAR_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_SCALAR_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_SCALAR_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_SCALAR_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_SCALAR_ASSEMBLY_INPUTS
#undef PTO_REGION_SCALAR_ASSEMBLY_BODY
}

template <int ParentSize, int Opcode, typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar_assemble_phase(
    region::TileArrayOutputRef<SubTile> &dst, In &src,
    typename In::DType scalar) {
  if (dst.slot_count() == 1)
    pto_region_scalar_assemble<ParentSize, true, true, Opcode>(dst, src,
                                                                scalar);
  else if (dst.ordinal() == 0)
    pto_region_scalar_assemble<ParentSize, true, false, Opcode>(dst, src,
                                                                 scalar);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_scalar_assemble<ParentSize, false, true, Opcode>(dst, src,
                                                                 scalar);
  else
    pto_region_scalar_assemble<ParentSize, false, false, Opcode>(dst, src,
                                                                  scalar);
}

template <int Opcode, typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar_assemble_dispatch(
    region::TileArrayOutputRef<SubTile> &dst, In &src,
    typename In::DType scalar) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_SCALAR_PARENT_CASE(N)                                    \
  case N:                                                                   \
    pto_region_scalar_assemble_phase<N, Opcode>(dst, src, scalar);          \
    break
    PTO_REGION_SCALAR_PARENT_CASE(1);
    PTO_REGION_SCALAR_PARENT_CASE(2);
    PTO_REGION_SCALAR_PARENT_CASE(3);
    PTO_REGION_SCALAR_PARENT_CASE(4);
    PTO_REGION_SCALAR_PARENT_CASE(5);
    PTO_REGION_SCALAR_PARENT_CASE(6);
    PTO_REGION_SCALAR_PARENT_CASE(7);
    PTO_REGION_SCALAR_PARENT_CASE(8);
    PTO_REGION_SCALAR_PARENT_CASE(9);
    PTO_REGION_SCALAR_PARENT_CASE(10);
    PTO_REGION_SCALAR_PARENT_CASE(11);
    PTO_REGION_SCALAR_PARENT_CASE(12);
#undef PTO_REGION_SCALAR_PARENT_CASE
  default:
    __builtin_trap();
  }
}

template <int ParentSize, bool Init, bool Last, int Opcode,
          typename SubTile, typename Parent, typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar_subview_assemble(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src,
    typename SourceSubTile::DType scalar) {
  static_assert(SubTile::BFractal == BLayout::RowMajor &&
                    SourceSubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox &&
                    SourceSubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == SourceSubTile::Rows &&
                    SubTile::Cols == SourceSubTile::Cols,
                "TileArray slot requires matching physical shape");
  static_assert(SubTile::ValidRow == SourceSubTile::ValidRow &&
                    SubTile::ValidCol == SourceSubTile::ValidCol,
                "TileArray slot requires matching valid shape");
  static_assert(std::is_same_v<typename SubTile::DType,
                               typename SourceSubTile::DType>,
                "TileArray slot requires matching element types");
  volatile typename SourceSubTile::DType value = scalar;
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t source_base_units = src.GetRangeBase();
  const uintptr_t destination_base_units = dst.range_base_units();
#define PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_BODY                             \
  "BSTART.TEPL %c7, %D1\n"                                                \
  "B.DIM zero, %c3, ->lb0\n"                                                  \
  "B.DIM zero, %c4, ->lb1\n"                                                  \
  "B.DIM zero, %c5, ->lb2\n"                                              \
  "B.IOT %2, mask=1111, last, ->%0<%Z6>\n"                                 \
  "B.SUBVIEW 0, %8, 0, %c9\n"                                             \
  "B.IOR [%14],[]\n"                                                       \
  "B.ASSEMBLE %c12, %c13, %10, 0, %c11\n"
#define PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS                          \
  "i"(type_traits<typename SourceSubTile::DType>::TypeCode),               \
  "Tr"(src.data()),                                                       \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                  \
  "i"(SourceSubTile::Cols),                                                 \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),         \
  "i"(Opcode), "r"(source_base_units),                                    \
  "i"(tile_type_traits<typename SourceSubTile::TileDType>::TilesizeCode),   \
  "r"(destination_base_units), "i"(encoded_parent_size),                  \
  "i"(Init), "i"(Last), "r"(value)
  if constexpr (Init) {
    asm volatile(PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS
#undef PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_BODY
}

template <int ParentSize, int Opcode, typename SubTile, typename Parent,
          typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar_subview_assemble_phase(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src,
    typename SourceSubTile::DType scalar) {
  if (dst.slot_count() == 1)
    pto_region_scalar_subview_assemble<ParentSize, true, true, Opcode>(
        dst, src, scalar);
  else if (dst.ordinal() == 0)
    pto_region_scalar_subview_assemble<ParentSize, true, false, Opcode>(
        dst, src, scalar);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_scalar_subview_assemble<ParentSize, false, true, Opcode>(
        dst, src, scalar);
  else
    pto_region_scalar_subview_assemble<ParentSize, false, false, Opcode>(
        dst, src, scalar);
}

template <int Opcode, typename SubTile, typename Parent,
          typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar_subview_assemble_dispatch(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src,
    typename SourceSubTile::DType scalar) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(N)                            \
  case N:                                                                   \
    pto_region_scalar_subview_assemble_phase<N, Opcode>(dst, src, scalar);  \
    break
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(1);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(2);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(3);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(4);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(5);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(6);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(7);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(8);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(9);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(10);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(11);
    PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE(12);
#undef PTO_REGION_SCALAR_SUBVIEW_PARENT_CASE
  default:
    __builtin_trap();
  }
}

template <int ParentSize, bool Init, bool Last, int Opcode,
          typename SubTile, typename Parent, typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_unary_subview_assemble(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src) {
  static_assert(SubTile::BFractal == BLayout::RowMajor &&
                    SourceSubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox &&
                    SourceSubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == SourceSubTile::Rows &&
                    SubTile::Cols == SourceSubTile::Cols,
                "TileArray slot requires matching physical shape");
  static_assert(SubTile::ValidRow == SourceSubTile::ValidRow &&
                    SubTile::ValidCol == SourceSubTile::ValidCol,
                "TileArray slot requires matching valid shape");
  static_assert(std::is_same_v<typename SubTile::DType,
                               typename SourceSubTile::DType>,
                "TileArray slot requires matching element types");
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t source_base_units = src.GetRangeBase();
  const uintptr_t destination_base_units = dst.range_base_units();
#define PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_BODY                              \
  "BSTART.TEPL %c7, %D1\n"                                                \
  "B.DIM zero, %c3, ->lb0\n"                                                  \
  "B.DIM zero, %c4, ->lb1\n"                                                  \
  "B.DIM zero, %c5, ->lb2\n"                                              \
  "B.IOT %2, mask=1111, last, ->%0<%Z6>\n"                                 \
  "B.SUBVIEW 0, %8, 0, %c9\n"                                             \
  "B.ASSEMBLE %c12, %c13, %10, 0, %c11\n"
#define PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS                           \
  "i"(type_traits<typename SourceSubTile::DType>::TypeCode),               \
  "Tr"(src.data()),                                                       \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                  \
  "i"(SourceSubTile::Cols),                                                 \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),         \
  "i"(Opcode), "r"(source_base_units),                                    \
  "i"(tile_type_traits<typename SourceSubTile::TileDType>::TilesizeCode),   \
  "r"(destination_base_units), "i"(encoded_parent_size),                  \
  "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS
#undef PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_BODY
}

template <int ParentSize, int Opcode, typename SubTile, typename Parent,
          typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_unary_subview_assemble_phase(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src) {
  if (dst.slot_count() == 1)
    pto_region_unary_subview_assemble<ParentSize, true, true, Opcode>(dst,
                                                                     src);
  else if (dst.ordinal() == 0)
    pto_region_unary_subview_assemble<ParentSize, true, false, Opcode>(dst,
                                                                      src);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_unary_subview_assemble<ParentSize, false, true, Opcode>(dst,
                                                                      src);
  else
    pto_region_unary_subview_assemble<ParentSize, false, false, Opcode>(dst,
                                                                       src);
}

template <int Opcode, typename SubTile, typename Parent,
          typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_unary_subview_assemble_dispatch(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(N)                             \
  case N:                                                                   \
    pto_region_unary_subview_assemble_phase<N, Opcode>(dst, src);           \
    break
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(1);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(2);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(3);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(4);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(5);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(6);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(7);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(8);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(9);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(10);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(11);
    PTO_REGION_UNARY_SUBVIEW_PARENT_CASE(12);
#undef PTO_REGION_UNARY_SUBVIEW_PARENT_CASE
  default:
    __builtin_trap();
  }
}

#define PTO_REGION_UNARY_SOURCE_WRAPPER(Name, Opcode)                        \
  template <is_tile_data_v Out, typename Parent, typename SubTile>           \
  PTO_REGION_ALWAYS_INLINE void Name(                                      \
      Out &dst, region::SubTileView<Parent, SubTile> &src) {                \
    pto_region_unary<Opcode>(dst, src);                                      \
  }

#define PTO_REGION_UNARY_DEST_WRAPPER(Name, Opcode)                          \
  template <typename SubTile, is_tile_data_v In>                             \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      region::TileArrayOutputRef<SubTile> dst, In &src) {                    \
    pto_region_unary_assemble_dispatch<Opcode>(dst, src);                    \
  }

#define PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(Name, Opcode)                  \
  template <typename SubTile, typename Parent, typename SourceSubTile>       \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      region::TileArrayOutputRef<SubTile> dst,                              \
      region::SubTileView<Parent, SourceSubTile> &src) {                    \
    pto_region_unary_subview_assemble_dispatch<Opcode>(dst, src);           \
  }

PTO_REGION_UNARY_SOURCE_WRAPPER(TABS, 15)
PTO_REGION_UNARY_SOURCE_WRAPPER(TNOT, 16)
PTO_REGION_UNARY_SOURCE_WRAPPER(TNEG, 17)
PTO_REGION_UNARY_SOURCE_WRAPPER(TLOG, 19)
PTO_REGION_UNARY_SOURCE_WRAPPER(TRECIP, 20)
PTO_REGION_UNARY_SOURCE_WRAPPER(TSQRT, 21)
PTO_REGION_UNARY_SOURCE_WRAPPER(TRSQRT, 22)
PTO_REGION_UNARY_SOURCE_WRAPPER(TRELU, 23)

#define PTO_REGION_SCALAR_SOURCE_WRAPPER(Name, Opcode)                       \
  template <is_tile_data_v Out, typename Parent, typename SubTile>           \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent, SubTile> &src,                   \
      typename SubTile::DType scalar) {                                      \
    pto_region_scalar<Opcode>(dst, src, scalar);                             \
  }

PTO_REGION_SCALAR_SOURCE_WRAPPER(TADDS, 32)
PTO_REGION_SCALAR_SOURCE_WRAPPER(TSUBS, 33)
PTO_REGION_SCALAR_SOURCE_WRAPPER(TMULS, 34)
PTO_REGION_SCALAR_SOURCE_WRAPPER(TDIVS, 35)
PTO_REGION_SCALAR_SOURCE_WRAPPER(TMAXS, 43)
PTO_REGION_SCALAR_SOURCE_WRAPPER(TMINS, 44)

PTO_REGION_UNARY_DEST_WRAPPER(TABS, 15)
PTO_REGION_UNARY_DEST_WRAPPER(TNOT, 16)
PTO_REGION_UNARY_DEST_WRAPPER(TNEG, 17)
PTO_REGION_UNARY_DEST_WRAPPER(TEXP, 18)
PTO_REGION_UNARY_DEST_WRAPPER(TLOG, 19)
PTO_REGION_UNARY_DEST_WRAPPER(TRECIP, 20)
PTO_REGION_UNARY_DEST_WRAPPER(TSQRT, 21)
PTO_REGION_UNARY_DEST_WRAPPER(TRSQRT, 22)
PTO_REGION_UNARY_DEST_WRAPPER(TRELU, 23)

PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TABS, 15)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TNOT, 16)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TNEG, 17)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TEXP, 18)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TLOG, 19)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TRECIP, 20)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TSQRT, 21)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TRSQRT, 22)
PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER(TRELU, 23)

#define PTO_REGION_SCALAR_DEST_WRAPPER(Name, Opcode)                         \
  template <typename SubTile, is_tile_data_v In>                             \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      region::TileArrayOutputRef<SubTile> dst, In &src,                     \
      typename In::DType scalar) {                                           \
    pto_region_scalar_assemble_dispatch<Opcode>(dst, src, scalar);           \
  }

#define PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(Name, Opcode)                 \
  template <typename SubTile, typename Parent, typename SourceSubTile>       \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      region::TileArrayOutputRef<SubTile> dst,                              \
      region::SubTileView<Parent, SourceSubTile> &src,                      \
      typename SourceSubTile::DType scalar) {                                \
    pto_region_scalar_subview_assemble_dispatch<Opcode>(dst, src, scalar);   \
  }

PTO_REGION_SCALAR_DEST_WRAPPER(TADDS, 32)
PTO_REGION_SCALAR_DEST_WRAPPER(TSUBS, 33)
PTO_REGION_SCALAR_DEST_WRAPPER(TMULS, 34)
PTO_REGION_SCALAR_DEST_WRAPPER(TDIVS, 35)
PTO_REGION_SCALAR_DEST_WRAPPER(TMAXS, 43)
PTO_REGION_SCALAR_DEST_WRAPPER(TMINS, 44)

PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(TADDS, 32)
PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(TSUBS, 33)
PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(TMULS, 34)
PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(TDIVS, 35)
PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(TMAXS, 43)
PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER(TMINS, 44)

#undef PTO_REGION_UNARY_SOURCE_WRAPPER
#undef PTO_REGION_UNARY_DEST_WRAPPER
#undef PTO_REGION_UNARY_SUBVIEW_DEST_WRAPPER
#undef PTO_REGION_SCALAR_SOURCE_WRAPPER
#undef PTO_REGION_SCALAR_DEST_WRAPPER
#undef PTO_REGION_SCALAR_SUBVIEW_DEST_WRAPPER

template <int ParentSize, bool Init, bool Last, typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void
pto_region_tcvt_assemble(region::TileArrayOutputRef<SubTile> &dst, In &src) {
  static_assert(SubTile::Rows == In::Rows && SubTile::Cols == In::Cols,
                "TCVT assembly slot requires matching physical shape");
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t range_base_units = dst.range_base_units();
#define PTO_REGION_TCVT_ASSEMBLY_BODY                                       \
  "BSTART.TEPL 27, %D1\n"                                                  \
  "B.DATR %D2, RNONE\n"                                                    \
  "B.DIM zero, %c4, ->lb0\n"                                                   \
  "B.DIM zero, %c5, ->lb1\n"                                                   \
  "B.DIM zero, %c6, ->lb2\n"                                              \
  "B.IOT %3, mask=1111, last, ->%0<%Z7>\n"                                 \
  "B.ASSEMBLE %c10, %c11, %8, 0, %c9\n"
#define PTO_REGION_TCVT_ASSEMBLY_INPUTS                                    \
  "i"(type_traits<typename In::DType>::TypeCode),                           \
  "i"(type_traits<typename SubTile::DType>::TypeCode),                      \
  "Tr"(src.data()),                                                          \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                    \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                    \
  "i"(SubTile::Cols),                                                        \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),          \
  "r"(range_base_units), "i"(encoded_parent_size), "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_TCVT_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_TCVT_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_TCVT_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_TCVT_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_TCVT_ASSEMBLY_INPUTS
#undef PTO_REGION_TCVT_ASSEMBLY_BODY
}

template <int ParentSize, typename SubTile, typename In>
PTO_REGION_ALWAYS_INLINE void
pto_region_tcvt_phase(region::TileArrayOutputRef<SubTile> &dst, In &src) {
  if (dst.slot_count() == 1)
    pto_region_tcvt_assemble<ParentSize, true, true>(dst, src);
  else if (dst.ordinal() == 0)
    pto_region_tcvt_assemble<ParentSize, true, false>(dst, src);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_tcvt_assemble<ParentSize, false, true>(dst, src);
  else
    pto_region_tcvt_assemble<ParentSize, false, false>(dst, src);
}

template <typename SubTile, is_tile_data_v In>
PTO_REGION_ALWAYS_INLINE void TCVT(region::TileArrayOutputRef<SubTile> dst,
                                   In &src) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_PARENT_CASE(N)                                             \
  case N:                                                                     \
    pto_region_tcvt_phase<N>(dst, src);                                       \
    break
    PTO_REGION_PARENT_CASE(1);
    PTO_REGION_PARENT_CASE(2);
    PTO_REGION_PARENT_CASE(3);
    PTO_REGION_PARENT_CASE(4);
    PTO_REGION_PARENT_CASE(5);
    PTO_REGION_PARENT_CASE(6);
    PTO_REGION_PARENT_CASE(7);
    PTO_REGION_PARENT_CASE(8);
    PTO_REGION_PARENT_CASE(9);
    PTO_REGION_PARENT_CASE(10);
    PTO_REGION_PARENT_CASE(11);
    PTO_REGION_PARENT_CASE(12);
#undef PTO_REGION_PARENT_CASE
  default:
    __builtin_trap();
  }
}

template <int ParentSize, bool Init, bool Last, typename SubTile,
          typename Parent, typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_tcvt_subview_assemble(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src) {
  static_assert(SubTile::BFractal == BLayout::RowMajor &&
                    SourceSubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox &&
                    SourceSubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == SourceSubTile::Rows &&
                    SubTile::Cols == SourceSubTile::Cols,
                "TCVT TileArray slot requires matching physical shape");
  static_assert(SubTile::ValidRow == SourceSubTile::ValidRow &&
                    SubTile::ValidCol == SourceSubTile::ValidCol,
                "TCVT TileArray slot requires matching valid shape");
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t source_base_units = src.GetRangeBase();
  const uintptr_t destination_base_units = dst.range_base_units();
#define PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_BODY                               \
  "BSTART.TEPL 27, %D1\n"                                                  \
  "B.DATR %D2, RNONE\n"                                                    \
  "B.DIM zero, %c4, ->lb0\n"                                                   \
  "B.DIM zero, %c5, ->lb1\n"                                                   \
  "B.DIM zero, %c6, ->lb2\n"                                               \
  "B.IOT %3, mask=1111, last, ->%0<%Z7>\n"                                 \
  "B.SUBVIEW 0, %8, 0, %c9\n"                                             \
  "B.ASSEMBLE %c12, %c13, %10, 0, %c11\n"
#define PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS                            \
  "i"(type_traits<typename SourceSubTile::DType>::TypeCode),               \
  "i"(type_traits<typename SubTile::DType>::TypeCode), "Tr"(src.data()),  \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                    \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                    \
  "i"(SourceSubTile::Cols),                                                 \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),         \
  "r"(source_base_units),                                                   \
  "i"(tile_type_traits<typename SourceSubTile::TileDType>::TilesizeCode),   \
  "r"(destination_base_units), "i"(encoded_parent_size),                  \
  "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS
#undef PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_BODY
}

template <int ParentSize, typename SubTile, typename Parent,
          typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_tcvt_subview_assemble_phase(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent, SourceSubTile> &src) {
  if (dst.slot_count() == 1)
    pto_region_tcvt_subview_assemble<ParentSize, true, true>(dst, src);
  else if (dst.ordinal() == 0)
    pto_region_tcvt_subview_assemble<ParentSize, true, false>(dst, src);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_tcvt_subview_assemble<ParentSize, false, true>(dst, src);
  else
    pto_region_tcvt_subview_assemble<ParentSize, false, false>(dst, src);
}

template <typename SubTile, typename Parent, typename SourceSubTile>
PTO_REGION_ALWAYS_INLINE void TCVT(
    region::TileArrayOutputRef<SubTile> dst,
    region::SubTileView<Parent, SourceSubTile> &src) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(N)                              \
  case N:                                                                   \
    pto_region_tcvt_subview_assemble_phase<N>(dst, src);                    \
    break
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(1);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(2);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(3);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(4);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(5);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(6);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(7);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(8);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(9);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(10);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(11);
    PTO_REGION_TCVT_SUBVIEW_PARENT_CASE(12);
#undef PTO_REGION_TCVT_SUBVIEW_PARENT_CASE
  default:
    __builtin_trap();
  }
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void
TCVT(Out &dst, region::SubTileView<Parent, SubTile> &src) {
  static_assert(SubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == Out::Rows && SubTile::Cols == Out::Cols,
                "TCVT region source requires matching physical shape");
  const uintptr_t region_base_units = src.GetRangeBase();
  asm volatile(
      "BSTART.TEPL 27, %D1\n"
      "B.DATR %D2, RNONE\n"
      "B.DIM zero, %c4, ->lb0\n"
      "B.DIM zero, %c5, ->lb1\n"
      "B.DIM zero, %c6, ->lb2\n"
      "B.IOT %3, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 0, %8, 0, %c9\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "i"(type_traits<typename Out::DType>::TypeCode), "Tr"(src.data()),
        "i"(std::remove_reference_t<decltype(src)>::ValidCol),
        "i"(std::remove_reference_t<decltype(src)>::ValidRow), "i"(SubTile::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "r"(region_base_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode)
      : "memory");
}

template <int Opcode, typename Out, typename Parent0, typename SubTile0,
          typename Parent1, typename SubTile1>
PTO_REGION_ALWAYS_INLINE void pto_region_binary(
    Out &dst, region::SubTileView<Parent0, SubTile0> &src0,
    region::SubTileView<Parent1, SubTile1> &src1) {
  static_assert(SubTile0::BFractal == BLayout::RowMajor &&
                    SubTile1::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile0::SFractal == SLayout::NoneBox &&
                    SubTile1::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile0::Rows == SubTile1::Rows &&
                    SubTile0::Cols == SubTile1::Cols,
                "binary region sources require matching physical shapes");
  static_assert(SubTile0::ValidRow == SubTile1::ValidRow &&
                    SubTile0::ValidCol == SubTile1::ValidCol,
                "binary region sources require matching valid shapes");
  static_assert(std::is_same_v<typename SubTile0::DType,
                               typename SubTile1::DType>,
                "binary region sources require matching element types");
  const uintptr_t range_base0_units = src0.GetRangeBase();
  const uintptr_t range_base1_units = src1.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c10, %D1\n"
      "B.DIM zero, %c4, ->lb0\n"
      "B.DIM zero, %c5, ->lb1\n"
      "B.DIM zero, %c6, ->lb2\n"
      "B.IOT %2, %3, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 0, %8, 0, %c11\n"
      "B.SUBVIEW 1, %9, 0, %c11\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile0::DType>::TypeCode),
        "Tr"(src0.data()), "Tr"(src1.data()),
        "i"(std::remove_reference_t<decltype(src0)>::ValidCol),
        "i"(std::remove_reference_t<decltype(src0)>::ValidRow),
        "i"(SubTile0::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "r"(range_base0_units), "r"(range_base1_units), "i"(Opcode),
        "i"(tile_type_traits<typename SubTile0::TileDType>::TilesizeCode)
      : "memory");
}

#define PTO_REGION_BINARY_SOURCE_WRAPPER(Name, Opcode)                       \
  template <is_tile_data_v Out, typename Parent0, typename SubTile0,         \
            typename Parent1, typename SubTile1>                              \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent0, SubTile0> &src0,                \
      region::SubTileView<Parent1, SubTile1> &src1) {                         \
    pto_region_binary<Opcode>(dst, src0, src1);                              \
  }

PTO_REGION_BINARY_SOURCE_WRAPPER(TADD, 0)
PTO_REGION_BINARY_SOURCE_WRAPPER(TSUB, 1)
PTO_REGION_BINARY_SOURCE_WRAPPER(TMUL, 2)
PTO_REGION_BINARY_SOURCE_WRAPPER(TDIV, 3)
PTO_REGION_BINARY_SOURCE_WRAPPER(TREM, 4)
PTO_REGION_BINARY_SOURCE_WRAPPER(TAND, 6)
PTO_REGION_BINARY_SOURCE_WRAPPER(TOR, 7)
PTO_REGION_BINARY_SOURCE_WRAPPER(TXOR, 8)
PTO_REGION_BINARY_SOURCE_WRAPPER(TMAX, 11)
PTO_REGION_BINARY_SOURCE_WRAPPER(TMIN, 12)

#undef PTO_REGION_BINARY_SOURCE_WRAPPER

template <int ParentSize, bool Init, bool Last, int Opcode,
          typename SubTile, typename Parent0, typename SubTile0,
          typename Parent1, typename SubTile1>
PTO_REGION_ALWAYS_INLINE void pto_region_binary_assemble(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent0, SubTile0> &src0,
    region::SubTileView<Parent1, SubTile1> &src1) {
  static_assert(SubTile::BFractal == BLayout::RowMajor &&
                    SubTile0::BFractal == BLayout::RowMajor &&
                    SubTile1::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox &&
                    SubTile0::SFractal == SLayout::NoneBox &&
                    SubTile1::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == SubTile0::Rows &&
                    SubTile::Cols == SubTile0::Cols &&
                    SubTile0::Rows == SubTile1::Rows &&
                    SubTile0::Cols == SubTile1::Cols,
                "binary TileArray slot requires matching physical shapes");
  static_assert(SubTile::ValidRow == SubTile0::ValidRow &&
                    SubTile::ValidCol == SubTile0::ValidCol &&
                    SubTile0::ValidRow == SubTile1::ValidRow &&
                    SubTile0::ValidCol == SubTile1::ValidCol,
                "binary TileArray slot requires matching valid shapes");
  static_assert(std::is_same_v<typename SubTile::DType,
                               typename SubTile0::DType> &&
                    std::is_same_v<typename SubTile0::DType,
                                   typename SubTile1::DType>,
                "binary TileArray slot requires matching element types");
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t source0_base_units = src0.GetRangeBase();
  const uintptr_t source1_base_units = src1.GetRangeBase();
  const uintptr_t destination_base_units = dst.range_base_units();
#define PTO_REGION_BINARY_ASSEMBLY_BODY                                     \
  "BSTART.TEPL %c11, %D1\n"                                               \
  "B.DIM zero, %c4, ->lb0\n"                                                   \
  "B.DIM zero, %c5, ->lb1\n"                                                   \
  "B.DIM zero, %c6, ->lb2\n"                                               \
  "B.IOT %2, %3, mask=1111, last, ->%0<%Z7>\n"                             \
  "B.SUBVIEW 0, %8, 0, %c12\n"                                             \
  "B.SUBVIEW 1, %9, 0, %c12\n"                                             \
  "B.ASSEMBLE %c14, %c15, %10, 0, %c13\n"
#define PTO_REGION_BINARY_ASSEMBLY_INPUTS                                  \
  "i"(type_traits<typename SubTile0::DType>::TypeCode),                    \
  "Tr"(src0.data()), "Tr"(src1.data()),                                   \
  "i"(std::remove_reference_t<decltype(src0)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src0)>::ValidRow),                  \
  "i"(SubTile0::Cols),                                                      \
  "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),        \
  "r"(source0_base_units), "r"(source1_base_units),                       \
  "r"(destination_base_units), "i"(Opcode),                              \
  "i"(tile_type_traits<typename SubTile0::TileDType>::TilesizeCode),       \
  "i"(encoded_parent_size), "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_BINARY_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_BINARY_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    asm volatile(PTO_REGION_BINARY_ASSEMBLY_BODY
                 : [Dst] "+Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_BINARY_ASSEMBLY_INPUTS
                 : "memory");
  }
#undef PTO_REGION_BINARY_ASSEMBLY_INPUTS
#undef PTO_REGION_BINARY_ASSEMBLY_BODY
}

template <int ParentSize, int Opcode, typename SubTile, typename Parent0,
          typename SubTile0, typename Parent1, typename SubTile1>
PTO_REGION_ALWAYS_INLINE void pto_region_binary_assemble_phase(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent0, SubTile0> &src0,
    region::SubTileView<Parent1, SubTile1> &src1) {
  if (dst.slot_count() == 1)
    pto_region_binary_assemble<ParentSize, true, true, Opcode>(dst, src0,
                                                               src1);
  else if (dst.ordinal() == 0)
    pto_region_binary_assemble<ParentSize, true, false, Opcode>(dst, src0,
                                                                src1);
  else if (dst.ordinal() == dst.slot_count() - 1)
    pto_region_binary_assemble<ParentSize, false, true, Opcode>(dst, src0,
                                                                src1);
  else
    pto_region_binary_assemble<ParentSize, false, false, Opcode>(dst, src0,
                                                                 src1);
}

template <int Opcode, typename SubTile, typename Parent0, typename SubTile0,
          typename Parent1, typename SubTile1>
PTO_REGION_ALWAYS_INLINE void pto_region_binary_assemble_dispatch(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent0, SubTile0> &src0,
    region::SubTileView<Parent1, SubTile1> &src1) {
  switch (dst.parent_size_code()) {
#define PTO_REGION_BINARY_PARENT_CASE(N)                                    \
  case N:                                                                   \
    pto_region_binary_assemble_phase<N, Opcode>(dst, src0, src1);           \
    break
    PTO_REGION_BINARY_PARENT_CASE(1);
    PTO_REGION_BINARY_PARENT_CASE(2);
    PTO_REGION_BINARY_PARENT_CASE(3);
    PTO_REGION_BINARY_PARENT_CASE(4);
    PTO_REGION_BINARY_PARENT_CASE(5);
    PTO_REGION_BINARY_PARENT_CASE(6);
    PTO_REGION_BINARY_PARENT_CASE(7);
    PTO_REGION_BINARY_PARENT_CASE(8);
    PTO_REGION_BINARY_PARENT_CASE(9);
    PTO_REGION_BINARY_PARENT_CASE(10);
    PTO_REGION_BINARY_PARENT_CASE(11);
    PTO_REGION_BINARY_PARENT_CASE(12);
#undef PTO_REGION_BINARY_PARENT_CASE
  default:
    __builtin_trap();
  }
}

#define PTO_REGION_BINARY_DEST_WRAPPER(Name, Opcode)                         \
  template <typename SubTile, typename Parent0, typename SubTile0,           \
            typename Parent1, typename SubTile1>                             \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      region::TileArrayOutputRef<SubTile> dst,                              \
      region::SubTileView<Parent0, SubTile0> &src0,                         \
      region::SubTileView<Parent1, SubTile1> &src1) {                       \
    pto_region_binary_assemble_dispatch<Opcode>(dst, src0, src1);           \
  }

PTO_REGION_BINARY_DEST_WRAPPER(TADD, 0)
PTO_REGION_BINARY_DEST_WRAPPER(TSUB, 1)
PTO_REGION_BINARY_DEST_WRAPPER(TMUL, 2)
PTO_REGION_BINARY_DEST_WRAPPER(TDIV, 3)
PTO_REGION_BINARY_DEST_WRAPPER(TREM, 4)
PTO_REGION_BINARY_DEST_WRAPPER(TAND, 6)
PTO_REGION_BINARY_DEST_WRAPPER(TOR, 7)
PTO_REGION_BINARY_DEST_WRAPPER(TXOR, 8)
PTO_REGION_BINARY_DEST_WRAPPER(TMAX, 11)
PTO_REGION_BINARY_DEST_WRAPPER(TMIN, 12)

#undef PTO_REGION_BINARY_DEST_WRAPPER

#undef PTO_REGION_ALWAYS_INLINE

} // namespace pto

#endif
