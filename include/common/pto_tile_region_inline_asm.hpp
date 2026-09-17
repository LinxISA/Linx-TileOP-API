#ifndef PTO_TILE_REGION_INLINE_ASM_HPP
#define PTO_TILE_REGION_INLINE_ASM_HPP

#include "common/pto_tile_region.hpp"

namespace pto {

#define PTO_REGION_ALWAYS_INLINE __attribute__((always_inline)) inline

template <int Opcode, typename Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void
pto_region_unary(Out &dst, region::SubTileView<Parent, SubTile> &src) {
  static_assert(range::is_legal_subview_parent_v<Parent>,
                "B.SUBVIEW source must be an assigned Local CUBE or Shared RowMajor "
                "tile layout");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  const uintptr_t region_base_units = src.GetRangeBase();
  {
    static_assert(SubTile::IsCubeLayout,
                  "B.SUBVIEW source fragment must use a CUBE layout");
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
}

template <int Opcode, typename Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_scalar(
    Out &dst, region::SubTileView<Parent, SubTile> &src,
    typename SubTile::DType scalar) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  volatile typename SubTile::DType value = scalar;
  const uintptr_t region_base_units = src.GetRangeBase();
  {
    static_assert(range::is_legal_subview_parent_v<Parent>,
                  "B.SUBVIEW source must be an assigned Local Matrix Tile with "
                  "a CUBE layout");
    static_assert(SubTile::IsCubeLayout,
                  "B.SUBVIEW source fragment must use a CUBE layout");
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

template <is_tile_data_v Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void TROWMIN(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<66>(dst, src);
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
PTO_REGION_ALWAYS_INLINE void TROWPROD(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<67>(dst, src);
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
  // PTO-ISA #265 (issue #702): B.ASSEMBLE field 5 is the WRITER extent in
  // every phase. On INIT the destination B.IOT is the allocating binder and
  // must carry the parent capacity; MIDDLE/LAST reuse the parent register
  // through the destination-only binder (non-allocating, SizeCode=0 form on
  // the parent register) and still encode the nonzero writer extent.
  constexpr int binder_size = Init ? ParentSize : 0;
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
  "i"(ParentSize),                                                          \
  "i"(Opcode), "r"(range_base_units), "i"(writer_size),                  \
  "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_UNARY_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_UNARY_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: the final source-form SizeCode=0
    // binder is exactly one Local ParentRef. The math input stays a
    // source-only B.IOT and the parent register binds source-form.
    asm volatile("BSTART.TEPL %c7, %D1\n"
                 "B.DIM zero, %c3, ->lb0\n"
                 "B.DIM zero, %c4, ->lb1\n"
                 "B.DIM zero, %c5, ->lb2\n"
                 "B.IOT %2, mask=1111, last\n"
                 "B.IOT %0, mask=1111\n"
                 "B.ASSEMBLE %c10, %c11, %8, 0, %c9\n"
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_UNARY_ASSEMBLY_INPUTS
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
  // PTO-ISA #265 (issue #702): field 5 is the writer extent; the INIT
  // destination B.IOT is the allocating binder and carries the parent.
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
  "i"(ParentSize),                                                          \
  "i"(Opcode), "r"(range_base_units), "i"(writer_size),                  \
  "i"(Init), "i"(Last), "r"(value)
  if constexpr (Init) {
    asm volatile(PTO_REGION_SCALAR_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_SCALAR_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: final source-form SizeCode=0 binder
    // carries the ParentRef; the math input stays source-only.
    asm volatile("BSTART.TEPL %c7, %D1\n"
                 "B.DIM zero, %c3, ->lb0\n"
                 "B.DIM zero, %c4, ->lb1\n"
                 "B.DIM zero, %c5, ->lb2\n"
                 "B.IOT %2, mask=1111, last\n"
                 "B.IOR [%12],[]\n"
                 "B.IOT %0, mask=1111\n"
                 "B.ASSEMBLE %c10, %c11, %8, 0, %c9\n"
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_SCALAR_ASSEMBLY_INPUTS
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
                    SourceSubTile::IsCubeLayout,
                "B.ASSEMBLE destination must be RowMajor and B.SUBVIEW "
                "source must use a CUBE layout");
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
  // PTO-ISA #265 (issue #702): field 5 is the writer extent in every phase.
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
#define PTO_REGION_SCALAR_SUBVIEW_CONT_BODY                                 \
  "BSTART.TEPL %c7, %D1\n"                                                \
  "B.DIM zero, %c3, ->lb0\n"                                                  \
  "B.DIM zero, %c4, ->lb1\n"                                                  \
  "B.DIM zero, %c5, ->lb2\n"                                              \
  "B.IOT %2, mask=1111, last\n"                                           \
  "B.SUBVIEW 0, %8, 0, %c9\n"                                             \
  "B.IOR [%14],[]\n"                                                       \
  "B.IOT %0, mask=1111\n"                                                 \
  "B.ASSEMBLE %c12, %c13, %10, 0, %c11\n"
#define PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS                          \
  "i"(type_traits<typename SourceSubTile::DType>::TypeCode),               \
  "Tr"(src.data()),                                                       \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                  \
  "i"(SourceSubTile::Cols),                                                 \
  "i"(ParentSize),                                                          \
  "i"(Opcode), "r"(source_base_units),                                    \
  "i"(tile_type_traits<typename SourceSubTile::TileDType>::TilesizeCode),   \
  "r"(destination_base_units), "i"(writer_size),                          \
  "i"(Init), "i"(Last), "r"(value)
  if constexpr (Init) {
    asm volatile(PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: final source-form SizeCode=0 binder.
    asm volatile(PTO_REGION_SCALAR_SUBVIEW_CONT_BODY
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_SCALAR_SUBVIEW_ASSEMBLY_INPUTS
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
                    SourceSubTile::IsCubeLayout,
                "B.ASSEMBLE destination must be RowMajor and B.SUBVIEW "
                "source must use a CUBE layout");
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
  // PTO-ISA #265 (issue #702): field 5 is the writer extent in every phase.
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
#define PTO_REGION_UNARY_SUBVIEW_CONT_BODY                                  \
  "BSTART.TEPL %c7, %D1\n"                                                \
  "B.DIM zero, %c3, ->lb0\n"                                                  \
  "B.DIM zero, %c4, ->lb1\n"                                                  \
  "B.DIM zero, %c5, ->lb2\n"                                              \
  "B.IOT %2, mask=1111, last\n"                                           \
  "B.SUBVIEW 0, %8, 0, %c9\n"                                             \
  "B.IOT %0, mask=1111\n"                                                 \
  "B.ASSEMBLE %c12, %c13, %10, 0, %c11\n"
#define PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS                           \
  "i"(type_traits<typename SourceSubTile::DType>::TypeCode),               \
  "Tr"(src.data()),                                                       \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                  \
  "i"(SourceSubTile::Cols),                                                 \
  "i"(ParentSize),                                                          \
  "i"(Opcode), "r"(source_base_units),                                    \
  "i"(tile_type_traits<typename SourceSubTile::TileDType>::TilesizeCode),   \
  "r"(destination_base_units), "i"(writer_size),                          \
  "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: final source-form SizeCode=0 binder.
    asm volatile(PTO_REGION_UNARY_SUBVIEW_CONT_BODY
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_UNARY_SUBVIEW_ASSEMBLY_INPUTS
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
  // PTO-ISA #265 (issue #702): field 5 is the writer extent in every phase;
  // the INIT destination B.IOT allocates and carries the parent capacity.
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
  "i"(ParentSize),                                                           \
  "r"(range_base_units), "i"(writer_size), "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_TCVT_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_TCVT_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: final source-form SizeCode=0 binder
    // carries the ParentRef; the math input stays source-only.
    asm volatile("BSTART.TEPL 27, %D1\n"
                 "B.DATR %D2, RNONE\n"
                 "B.DIM zero, %c4, ->lb0\n"
                 "B.DIM zero, %c5, ->lb1\n"
                 "B.DIM zero, %c6, ->lb2\n"
                 "B.IOT %3, mask=1111, last\n"
                 "B.IOT %0, mask=1111\n"
                 "B.ASSEMBLE %c10, %c11, %8, 0, %c9\n"
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_TCVT_ASSEMBLY_INPUTS
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
                    SourceSubTile::IsCubeLayout,
                "B.ASSEMBLE destination must be RowMajor and B.SUBVIEW "
                "source must use a CUBE layout");
  static_assert(SubTile::SFractal == SLayout::NoneBox &&
                    SourceSubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile::Rows == SourceSubTile::Rows &&
                    SubTile::Cols == SourceSubTile::Cols,
                "TCVT TileArray slot requires matching physical shape");
  static_assert(SubTile::ValidRow == SourceSubTile::ValidRow &&
                    SubTile::ValidCol == SourceSubTile::ValidCol,
                "TCVT TileArray slot requires matching valid shape");
  // PTO-ISA #265 (issue #702): field 5 is the writer extent in every phase.
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
#define PTO_REGION_TCVT_SUBVIEW_CONT_BODY                                   \
  "BSTART.TEPL 27, %D1\n"                                                  \
  "B.DATR %D2, RNONE\n"                                                    \
  "B.DIM zero, %c4, ->lb0\n"                                                   \
  "B.DIM zero, %c5, ->lb1\n"                                                   \
  "B.DIM zero, %c6, ->lb2\n"                                               \
  "B.IOT %3, mask=1111, last\n"                                           \
  "B.SUBVIEW 0, %8, 0, %c9\n"                                             \
  "B.IOT %0, mask=1111\n"                                                 \
  "B.ASSEMBLE %c12, %c13, %10, 0, %c11\n"
#define PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS                            \
  "i"(type_traits<typename SourceSubTile::DType>::TypeCode),               \
  "i"(type_traits<typename SubTile::DType>::TypeCode), "Tr"(src.data()),  \
  "i"(std::remove_reference_t<decltype(src)>::ValidCol),                    \
  "i"(std::remove_reference_t<decltype(src)>::ValidRow),                    \
  "i"(SourceSubTile::Cols),                                                 \
  "i"(ParentSize),                                                           \
  "r"(source_base_units),                                                   \
  "i"(tile_type_traits<typename SourceSubTile::TileDType>::TilesizeCode),   \
  "r"(destination_base_units), "i"(writer_size),                          \
  "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: final source-form SizeCode=0 binder.
    asm volatile(PTO_REGION_TCVT_SUBVIEW_CONT_BODY
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_TCVT_SUBVIEW_ASSEMBLY_INPUTS
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
  static_assert((SubTile0::BFractal == BLayout::RowMajor ||
                 SubTile0::IsCubeLayout) &&
                    (SubTile1::BFractal == BLayout::RowMajor ||
                     SubTile1::IsCubeLayout),
                "inline Tile region path requires RowMajor or Cube fragments");
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

template <int Opcode, typename Out, typename Parent, typename SubTile,
          typename Tile>
PTO_REGION_ALWAYS_INLINE void pto_region_binary(
    Out &dst, region::SubTileView<Parent, SubTile> &src0, Tile &src1) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(std::is_same_v<typename SubTile::DType, typename Tile::DType>,
                "binary region sources require matching element types");
  static_assert(std::is_same_v<typename SubTile::DType, typename Out::DType>,
                "binary source and destination dtypes must match");
  static_assert(SubTile::Rows == Tile::Rows && SubTile::Cols == Tile::Cols,
                "binary region sources require matching physical shapes");
  static_assert(SubTile::ValidRow == Tile::ValidRow &&
                    SubTile::ValidCol == Tile::ValidCol,
                "binary region sources require matching valid shapes");
  const uintptr_t range_base0_units = src0.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %2, %6, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 0, %7, 0, %c8\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "Tr"(src0.data()), "i"(Tile::ValidCol), "i"(Tile::ValidRow),
        "i"(Tile::Cols), "Tr"(src1.data()), "r"(range_base0_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(Opcode)
      : "memory");
}

template <int Opcode, typename Out, typename Tile, typename Parent,
          typename SubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_binary(
    Out &dst, Tile &src0, region::SubTileView<Parent, SubTile> &src1) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(std::is_same_v<typename Tile::DType, typename SubTile::DType>,
                "binary region sources require matching element types");
  static_assert(std::is_same_v<typename Tile::DType, typename Out::DType>,
                "binary source and destination dtypes must match");
  static_assert(Tile::Rows == SubTile::Rows && Tile::Cols == SubTile::Cols,
                "binary region sources require matching physical shapes");
  static_assert(Tile::ValidRow == SubTile::ValidRow &&
                    Tile::ValidCol == SubTile::ValidCol,
                "binary region sources require matching valid shapes");
  const uintptr_t range_base1_units = src1.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %6, %2, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 1, %7, 0, %c8\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename Tile::DType>::TypeCode), "Tr"(src1.data()),
        "i"(Tile::ValidCol), "i"(Tile::ValidRow), "i"(Tile::Cols),
        "Tr"(src0.data()), "r"(range_base1_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(Opcode)
      : "memory");
}

#define PTO_REGION_BINARY_SOURCE_WRAPPER(Name, Opcode)                       \
  template <is_tile_data_v Out, typename Parent0, typename SubTile0,         \
            typename Parent1, typename SubTile1>                              \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent0, SubTile0> &src0,                \
      region::SubTileView<Parent1, SubTile1> &src1) {                         \
    pto_region_binary<Opcode>(dst, src0, src1);                              \
  } \
  template <is_tile_data_v Out, typename Parent, typename SubTile,            \
            typename Tile>                                                    \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent, SubTile> &src0, Tile &src1) {     \
    pto_region_binary<Opcode>(dst, src0, src1);                              \
  }                                                                          \
  template <is_tile_data_v Out, typename Tile, typename Parent,               \
            typename SubTile>                                                 \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, Tile &src0, region::SubTileView<Parent, SubTile> &src1) {     \
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

template <int Opcode, typename Out, typename Parent0, typename SubTile0,
          typename Parent1, typename SubTile1>
PTO_REGION_ALWAYS_INLINE void pto_region_expand(
    Out &dst, region::SubTileView<Parent0, SubTile0> &src0,
    region::SubTileView<Parent1, SubTile1> &src1) {
  static_assert(SubTile0::SFractal == SLayout::NoneBox &&
                    SubTile1::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(SubTile0::ValidCol == SubTile1::ValidCol &&
                    SubTile1::ValidCol == 1,
                "row expansion requires a one-column broadcast source");
  static_assert(SubTile0::ValidRow == SubTile1::ValidRow,
                "row expansion broadcast rows must match the matrix");
  static_assert(std::is_same_v<typename SubTile0::DType,
                               typename SubTile1::DType>,
                "expansion sources require matching element types");
  static_assert(std::is_same_v<typename SubTile0::DType, typename Out::DType>,
                "expansion source and destination dtypes must match");
  static_assert(Out::ValidCol > 0 && Out::ValidRow > 0,
                "region expansion requires a static destination shape");
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
        "Tr"(src0.data()), "Tr"(src1.data()), "i"(Out::ValidCol),
        "i"(Out::ValidRow), "i"(Out::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "r"(range_base0_units), "r"(range_base1_units), "i"(Opcode),
        "i"(tile_type_traits<typename SubTile0::TileDType>::TilesizeCode)
      : "memory");
}

template <int Opcode, typename Out, typename Parent, typename SubTile,
          typename Tile>
PTO_REGION_ALWAYS_INLINE void pto_region_row_expand(
    Out &dst, region::SubTileView<Parent, SubTile> &src0, Tile &src1) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(std::is_same_v<typename SubTile::DType, typename Tile::DType>,
                "expansion sources require matching element types");
  static_assert(std::is_same_v<typename SubTile::DType, typename Out::DType>,
                "expansion source and destination dtypes must match");
  static_assert(Tile::ValidCol == 1,
                "row expansion requires a one-column broadcast source");
  static_assert(SubTile::ValidRow == Tile::ValidRow,
                "row expansion broadcast rows must match the matrix");
  static_assert(Out::ValidCol > 0 && Out::ValidRow > 0,
                "region expansion requires a static destination shape");
  const uintptr_t range_base0_units = src0.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %2, %6, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 0, %7, 0, %c8\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "Tr"(src0.data()), "i"(Out::ValidCol), "i"(Out::ValidRow),
        "i"(Out::Cols), "Tr"(src1.data()),
        "r"(range_base0_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(Opcode)
      : "memory");
}

template <int Opcode, typename Out, typename Tile, typename Parent,
          typename SubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_row_expand(
    Out &dst, Tile &src0, region::SubTileView<Parent, SubTile> &src1) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(std::is_same_v<typename Tile::DType, typename SubTile::DType>,
                "expansion sources require matching element types");
  static_assert(std::is_same_v<typename Tile::DType, typename Out::DType>,
                "expansion source and destination dtypes must match");
  static_assert(SubTile::ValidCol == 1,
                "row expansion requires a one-column broadcast source");
  static_assert(Tile::ValidRow == SubTile::ValidRow,
                "row expansion broadcast rows must match the matrix");
  static_assert(Out::ValidCol > 0 && Out::ValidRow > 0,
                "region expansion requires a static destination shape");
  const uintptr_t range_base1_units = src1.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %6, %2, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 1, %7, 0, %c8\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename Tile::DType>::TypeCode), "Tr"(src1.data()),
        "i"(Out::ValidCol), "i"(Out::ValidRow), "i"(Out::Cols),
        "Tr"(src0.data()), "r"(range_base1_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(Opcode)
      : "memory");
}

template <int Opcode, typename Out, typename Parent, typename SubTile,
          typename Tile>
PTO_REGION_ALWAYS_INLINE void pto_region_col_expand(
    Out &dst, region::SubTileView<Parent, SubTile> &src0, Tile &src1) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(std::is_same_v<typename SubTile::DType, typename Tile::DType>,
                "expansion sources require matching element types");
  static_assert(std::is_same_v<typename SubTile::DType, typename Out::DType>,
                "expansion source and destination dtypes must match");
  static_assert(Tile::ValidRow == 1,
                "column expansion requires a one-row broadcast source");
  static_assert(SubTile::ValidCol == Tile::ValidCol,
                "column expansion broadcast cols must match the matrix");
  static_assert(Out::ValidCol > 0 && Out::ValidRow > 0,
                "region expansion requires a static destination shape");
  const uintptr_t range_base0_units = src0.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %2, %6, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 0, %7, 0, %c8\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "Tr"(src0.data()), "i"(Out::ValidCol), "i"(Out::ValidRow),
        "i"(Out::Cols), "Tr"(src1.data()),
        "r"(range_base0_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(Opcode)
      : "memory");
}

template <int Opcode, typename Out, typename Tile, typename Parent,
          typename SubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_col_expand(
    Out &dst, Tile &src0, region::SubTileView<Parent, SubTile> &src1) {
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  static_assert(std::is_same_v<typename Tile::DType, typename SubTile::DType>,
                "expansion sources require matching element types");
  static_assert(std::is_same_v<typename Tile::DType, typename Out::DType>,
                "expansion source and destination dtypes must match");
  static_assert(SubTile::ValidRow == 1,
                "column expansion requires a one-row broadcast source");
  static_assert(Tile::ValidCol == SubTile::ValidCol,
                "column expansion broadcast cols must match the matrix");
  static_assert(Out::ValidCol > 0 && Out::ValidRow > 0,
                "region expansion requires a static destination shape");
  const uintptr_t range_base1_units = src1.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c3, ->lb0\n"
      "B.DIM zero, %c4, ->lb1\n"
      "B.DIM zero, %c5, ->lb2\n"
      "B.IOT %6, %2, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 1, %7, 0, %c8\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename Tile::DType>::TypeCode), "Tr"(src1.data()),
        "i"(Out::ValidCol), "i"(Out::ValidRow), "i"(Out::Cols),
        "Tr"(src0.data()), "r"(range_base1_units),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "i"(Opcode)
      : "memory");
}

#define PTO_REGION_EXPAND_SOURCE_WRAPPER(Name, Opcode)                       \
  template <is_tile_data_v Out, typename Parent0, typename SubTile0,          \
            typename Parent1, typename SubTile1>                              \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent0, SubTile0> &src0,                \
      region::SubTileView<Parent1, SubTile1> &src1) {                         \
    pto_region_expand<Opcode>(dst, src0, src1);                              \
  }

#define PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(Name, Opcode)                    \
  template <is_tile_data_v Out, typename Parent, typename SubTile,            \
            typename Tile>                                                    \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent, SubTile> &src0, Tile &src1) {     \
    pto_region_row_expand<Opcode>(dst, src0, src1);                          \
  }                                                                          \
  template <is_tile_data_v Out, typename Tile, typename Parent,               \
            typename SubTile>                                                 \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, Tile &src0, region::SubTileView<Parent, SubTile> &src1) {     \
    pto_region_row_expand<Opcode>(dst, src0, src1);                          \
  }

#define PTO_REGION_COL_EXPAND_MIXED_WRAPPER(Name, Opcode)                    \
  template <is_tile_data_v Out, typename Parent, typename SubTile,            \
            typename Tile>                                                    \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, region::SubTileView<Parent, SubTile> &src0, Tile &src1) {     \
    pto_region_col_expand<Opcode>(dst, src0, src1);                          \
  }                                                                          \
  template <is_tile_data_v Out, typename Tile, typename Parent,               \
            typename SubTile>                                                 \
  PTO_REGION_ALWAYS_INLINE void Name(                                       \
      Out &dst, Tile &src0, region::SubTileView<Parent, SubTile> &src1) {     \
    pto_region_col_expand<Opcode>(dst, src0, src1);                          \
  }

PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDADD, 69)
PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDSUB, 70)
PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDMUL, 71)
PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDDIV, 72)
PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDMAX, 73)
PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDMIN, 74)
PTO_REGION_EXPAND_SOURCE_WRAPPER(TROWEXPANDEXPDIF, 75)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDADD, 69)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDSUB, 70)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDMUL, 71)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDDIV, 72)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDMAX, 73)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDMIN, 74)
PTO_REGION_ROW_EXPAND_MIXED_WRAPPER(TROWEXPANDEXPDIF, 75)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDADD, 85)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDSUB, 86)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDMUL, 87)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDDIV, 88)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDMAX, 89)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDMIN, 90)
PTO_REGION_COL_EXPAND_MIXED_WRAPPER(TCOLEXPANDEXPDIF, 91)

#undef PTO_REGION_EXPAND_SOURCE_WRAPPER
#undef PTO_REGION_ROW_EXPAND_MIXED_WRAPPER
#undef PTO_REGION_COL_EXPAND_MIXED_WRAPPER

template <int Opcode, typename Out, typename Tile, typename Parent,
          typename SubTile>
PTO_REGION_ALWAYS_INLINE void pto_region_binary_reduction_prefix(
    Out &dst, Tile &src0,
    region::ReductionPrefixView<Parent, SubTile> &src1) {
  static_assert(Tile::IsCubeLayout && SubTile::IsCubeLayout,
                "reduction prefix sources require CUBE layouts");
  static_assert(Tile::Rows == SubTile::Rows && Tile::Cols == SubTile::Cols,
                "reduction prefix sources require matching physical shapes");
  static_assert(Tile::ValidRow == SubTile::ValidRow &&
                    Tile::ValidCol == SubTile::ValidCol,
                "reduction prefix sources require matching valid shapes");
  static_assert(std::is_same_v<typename Tile::DType, typename SubTile::DType>,
                "reduction prefix sources require matching element types");
  const uintptr_t prefix_base_units = src1.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c4, ->lb0\n"
      "B.DIM zero, %c5, ->lb1\n"
      "B.DIM zero, %c6, ->lb2\n"
      "B.IOT %2, %3, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 1, %8, 0, %c10\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename Tile::DType>::TypeCode),
        "Tr"(src0.data()), "Tr"(src1.data()),
        "i"(Tile::ValidCol), "i"(Tile::ValidRow), "i"(Tile::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "r"(prefix_base_units), "i"(Opcode),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode)
      : "memory");
}

template <int Opcode, typename Out, typename Parent, typename SubTile,
          typename Tile>
PTO_REGION_ALWAYS_INLINE void pto_region_binary_reduction_prefix(
    Out &dst, region::ReductionPrefixView<Parent, SubTile> &src0,
    Tile &src1) {
  static_assert(Tile::IsCubeLayout && SubTile::IsCubeLayout,
                "reduction prefix sources require CUBE layouts");
  static_assert(Tile::Rows == SubTile::Rows && Tile::Cols == SubTile::Cols,
                "reduction prefix sources require matching physical shapes");
  static_assert(Tile::ValidRow == SubTile::ValidRow &&
                    Tile::ValidCol == SubTile::ValidCol,
                "reduction prefix sources require matching valid shapes");
  static_assert(std::is_same_v<typename Tile::DType, typename SubTile::DType>,
                "reduction prefix sources require matching element types");
  const uintptr_t prefix_base_units = src0.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c9, %D1\n"
      "B.DIM zero, %c4, ->lb0\n"
      "B.DIM zero, %c5, ->lb1\n"
      "B.DIM zero, %c6, ->lb2\n"
      "B.IOT %2, %3, mask=1111, last, ->%0<%Z7>\n"
      "B.SUBVIEW 0, %8, 0, %c10\n"
      : [Dst] "=Tr"(dst.data())
      : "i"(type_traits<typename SubTile::DType>::TypeCode),
        "Tr"(src0.data()), "Tr"(src1.data()),
        "i"(SubTile::ValidCol), "i"(SubTile::ValidRow), "i"(SubTile::Cols),
        "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        "r"(prefix_base_units), "i"(Opcode),
        "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode)
      : "memory");
}

#define PTO_REGION_BINARY_PREFIX_WRAPPER(Name, Opcode)                         \
  template <is_tile_data_v Out, typename Tile, typename Parent, typename SubTile> \
  PTO_REGION_ALWAYS_INLINE void Name(                                        \
      Out &dst, Tile &src0,                                                   \
      region::ReductionPrefixView<Parent, SubTile> &src1) {                    \
    pto_region_binary_reduction_prefix<Opcode>(dst, src0, src1);              \
  }                                                                            \
  template <is_tile_data_v Out, typename Parent, typename SubTile, typename Tile> \
  PTO_REGION_ALWAYS_INLINE void Name(                                        \
      Out &dst, region::ReductionPrefixView<Parent, SubTile> &src0,           \
      Tile &src1) {                                                           \
    pto_region_binary_reduction_prefix<Opcode>(dst, src0, src1);              \
  }

PTO_REGION_BINARY_PREFIX_WRAPPER(TADD, 0)
PTO_REGION_BINARY_PREFIX_WRAPPER(TSUB, 1)
PTO_REGION_BINARY_PREFIX_WRAPPER(TMUL, 2)
PTO_REGION_BINARY_PREFIX_WRAPPER(TDIV, 3)
PTO_REGION_BINARY_PREFIX_WRAPPER(TREM, 4)
PTO_REGION_BINARY_PREFIX_WRAPPER(TAND, 6)
PTO_REGION_BINARY_PREFIX_WRAPPER(TOR, 7)
PTO_REGION_BINARY_PREFIX_WRAPPER(TXOR, 8)
PTO_REGION_BINARY_PREFIX_WRAPPER(TMAX, 11)
PTO_REGION_BINARY_PREFIX_WRAPPER(TMIN, 12)

#undef PTO_REGION_BINARY_PREFIX_WRAPPER

template <int ParentSize, bool Init, bool Last, int Opcode,
          typename SubTile, typename Parent0, typename SubTile0,
          typename Parent1, typename SubTile1>
PTO_REGION_ALWAYS_INLINE void pto_region_binary_assemble(
    region::TileArrayOutputRef<SubTile> &dst,
    region::SubTileView<Parent0, SubTile0> &src0,
    region::SubTileView<Parent1, SubTile1> &src1) {
  static_assert(SubTile::BFractal == BLayout::RowMajor &&
                    SubTile0::IsCubeLayout && SubTile1::IsCubeLayout,
                "B.ASSEMBLE destination must be RowMajor and B.SUBVIEW "
                "sources must use CUBE layouts");
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
  // PTO-ISA #265 (issue #702): field 5 is the writer extent in every phase;
  // the INIT destination B.IOT allocates and carries the parent capacity.
  constexpr int writer_size =
      tile_type_traits<typename SubTile::TileDType>::TilesizeCode;
  static_assert(writer_size != 0,
                "TileArray slot writer extent must be a nonzero size code");
  // The writer-fits-parent bound is guaranteed by TileArray construction:
  // ParentBytes = slot_count * SubTile::LogicalTileBytes. ParentSize here is
  // only the dispatch case label, and the switch instantiates every case,
  // so a compile-time comparison against it would spuriously fail.
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
#define PTO_REGION_BINARY_CONT_BODY                                         \
  "BSTART.TEPL %c11, %D1\n"                                               \
  "B.DIM zero, %c4, ->lb0\n"                                                   \
  "B.DIM zero, %c5, ->lb1\n"                                                   \
  "B.DIM zero, %c6, ->lb2\n"                                               \
  "B.IOT %2, %3, mask=1111, last\n"                                       \
  "B.SUBVIEW 0, %8, 0, %c12\n"                                             \
  "B.SUBVIEW 1, %9, 0, %c12\n"                                             \
  "B.IOT %0, mask=1111\n"                                                 \
  "B.ASSEMBLE %c14, %c15, %10, 0, %c13\n"
#define PTO_REGION_BINARY_ASSEMBLY_INPUTS                                  \
  "i"(type_traits<typename SubTile0::DType>::TypeCode),                    \
  "Tr"(src0.data()), "Tr"(src1.data()),                                   \
  "i"(std::remove_reference_t<decltype(src0)>::ValidCol),                  \
  "i"(std::remove_reference_t<decltype(src0)>::ValidRow),                  \
  "i"(SubTile0::Cols),                                                      \
  "i"(ParentSize),                                                           \
  "r"(source0_base_units), "r"(source1_base_units),                       \
  "r"(destination_base_units), "i"(Opcode),                              \
  "i"(tile_type_traits<typename SubTile0::TileDType>::TilesizeCode),       \
  "i"(writer_size), "i"(Init), "i"(Last)
  if constexpr (Init) {
    asm volatile(PTO_REGION_BINARY_ASSEMBLY_BODY
                 : [Dst] "=Tr"(dst.template parent_data<ParentSize>())
                 : PTO_REGION_BINARY_ASSEMBLY_INPUTS
                 : "memory");
  } else {
    // PTO-ISA #265 Local continuation: final source-form SizeCode=0 binder.
    asm volatile(PTO_REGION_BINARY_CONT_BODY
                 :
                 : [Dst] "Tr"(dst.template parent_data<ParentSize>()),
                   PTO_REGION_BINARY_ASSEMBLY_INPUTS
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
