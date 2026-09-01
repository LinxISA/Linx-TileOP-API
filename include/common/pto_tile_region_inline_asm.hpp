#ifndef PTO_TILE_REGION_INLINE_ASM_HPP
#define PTO_TILE_REGION_INLINE_ASM_HPP

#include "common/pto_tile_region.hpp"

namespace pto {

template <int Opcode, typename Out, typename Parent, typename SubTile>
inline void
pto_region_unary(Out &dst, region::SubTileView<Parent, SubTile> &src) {
  static_assert(SubTile::BFractal == BLayout::RowMajor,
                "inline Tile region path requires RowMajor fragments");
  static_assert(SubTile::SFractal == SLayout::NoneBox,
                "inline Tile region path requires unboxed fragments");
  const uintptr_t region_base_units = src.GetRangeBase();
  asm volatile(
      "BSTART.TEPL %c[Opcode], %D[Type]\n"
      "B.DIM %[VC], 0, ->lb0\n"
      "B.DIM %[VR], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[Size]>\n"
      "B.SUBVIEW 0, %[Base], 0, %c[SubviewSize]\n"
      : [Dst] "=Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [Type] "i"(type_traits<typename SubTile::DType>::TypeCode),
        [VC] "r"(src.GetValidCol()), [VR] "r"(src.GetValidRow()),
        [Cols] "i"(SubTile::Cols),
        [Size] "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        [SubviewSize] "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        [Opcode] "i"(Opcode), [Base] "r"(region_base_units)
      : "memory");
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
inline void TMULS(Out &dst,
                             region::SubTileView<Parent, SubTile> &src,
                             typename SubTile::DType scalar) {
  volatile typename SubTile::DType value = scalar;
  const uintptr_t region_base_units = src.GetRangeBase();
  asm volatile(
      "BSTART.TEPL 34, %D[Type]\n"
      "B.DIM %[VC], 0, ->lb0\n"
      "B.DIM %[VR], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[Size]>\n"
      "B.SUBVIEW 0, %[Base], 0, %c[SubviewSize]\n"
      "B.IOR [%[Scalar]],[]\n"
      : [Dst] "=Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [Type] "i"(type_traits<typename SubTile::DType>::TypeCode),
        [VC] "r"(src.GetValidCol()), [VR] "r"(src.GetValidRow()),
        [Cols] "i"(SubTile::Cols),
        [Size] "i"(tile_type_traits<typename Out::TileDType>::TilesizeCode),
        [SubviewSize] "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode),
        [Scalar] "r"(value), [Base] "r"(region_base_units)
      : "memory");
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
inline void TROWMAX(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<65>(dst, src);
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
inline void TROWSUM(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<64>(dst, src);
}

template <is_tile_data_v Out, typename Parent, typename SubTile>
inline void TEXP(
    Out &dst, region::SubTileView<Parent, SubTile> &src) {
  pto_region_unary<19>(dst, src);
}

template <int ParentSize, bool Init, bool Last, typename SubTile, typename In>
inline void
pto_region_tcvt_assemble(region::TileArrayOutputRef<SubTile> &dst, In &src) {
  static_assert(SubTile::Rows == In::Rows && SubTile::Cols == In::Cols,
                "TCVT assembly slot requires matching physical shape");
  constexpr int encoded_parent_size = Init ? ParentSize : 0;
  const uintptr_t range_base_units = dst.range_base_units();
#define PTO_REGION_TCVT_ASSEMBLY_BODY                                       \
  "BSTART.TEPL 27, %D[SrcType]\n"                                         \
  "B.DATR %D[DstType], RNONE\n"                                           \
  "B.DIM %[VC], 0, ->lb0\n"                                                \
  "B.DIM %[VR], 0, ->lb1\n"                                                \
  "B.DIM zero, %c[Cols], ->lb2\n"                                         \
  "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[Size]>\n"                   \
  "B.ASSEMBLE %c[Init], %c[Last], %[Base], 0, %c[ParentSize]\n"
#define PTO_REGION_TCVT_ASSEMBLY_INPUTS                                    \
  [Src] "Tr"(src.data()),                                                  \
  [SrcType] "i"(type_traits<typename In::DType>::TypeCode),                \
  [DstType] "i"(type_traits<typename SubTile::DType>::TypeCode),           \
  [VC] "r"(src.GetValidCol()), [VR] "r"(src.GetValidRow()),               \
  [Cols] "i"(SubTile::Cols),                                               \
  [Size] "i"(tile_type_traits<typename SubTile::TileDType>::TilesizeCode), \
  [Base] "r"(range_base_units),                                             \
  [ParentSize] "i"(encoded_parent_size), [Init] "i"(Init),                \
  [Last] "i"(Last)
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
inline void
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

template <typename SubTile, typename In>
inline void TCVT(region::TileArrayOutputRef<SubTile> dst, In &src) {
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

} // namespace pto

#endif
