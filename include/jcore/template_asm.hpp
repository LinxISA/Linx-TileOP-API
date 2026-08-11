#ifndef TEMPLATE_ASM_HPP
#define TEMPLATE_ASM_HPP

#include "common/pto_tile.hpp"

using namespace pto;

template <class...>
inline constexpr bool pto_dependent_false_v = false;

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void ACCSCALE_T(tile_shape_out &, tile_shape_in &,
                typename tile_shape_in::DType) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "ACCSCALE_T used the removed v5 ACCCVT opcode; use a "
                "TMATMUL*.FIXP operation that produces an ordinary Tile");
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void ACCSCALE_NZ2DN(tile_shape_out &, tile_shape_in &,
                    typename tile_shape_in::DType) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "ACCSCALE_NZ2DN used the removed v5 ACCCVT opcode; use a "
                "TMATMUL*.FIXP operation that produces an ordinary Tile");
}

template <is_tile_data_v tile_shape_max, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void ACCCVT_RMAX_SCALE_NZ2DN(tile_shape_max &, tile_shape_out &,
                            tile_shape_in &,
                            typename tile_shape_in::DType) {
  static_assert(
      pto_dependent_false_v<tile_shape_max, tile_shape_out, tile_shape_in>,
      "ACCCVT_RMAX_SCALE_NZ2DN used the removed v5 ACCCVT opcode; migrate "
      "to the matching TMATMUL*.FIXP variant and its RowMax operands");
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1>
void TMAX_T(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  asm volatile(
    "BSTART.VPAR 0b0000100011, %c3\n"
    "B.IOT %1, %2, mask=15, last, ->%0<%Z4>\n"
    "B.DIM %5, 0, ->lb0\n"
    "B.DIM %6, 0, ->lb1\n"

    ""
    : "=Tr"(dst.data())
    : "Tr"(src0.data()), "Tr"(src1.data()), \
      "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(src0.GetValidCol()), "r"(src0.GetValidRow())
  );
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1>
void TSUB_EXP_EXPAND_T(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  asm volatile(
    "BSTART.VPAR 0b0001000011, %c3\n"
    "B.IOT %1, %2, mask=15, last, ->%0<%Z4>\n"
    "B.DIM %5, 0, ->lb0\n"
    "B.DIM %6, 0, ->lb1\n"

    ""
    : "=Tr"(dst.data())
    : "Tr"(src0.data()), "Tr"(src1.data()), \
      "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(src0.GetValidCol()), "r"(src0.GetValidRow())
  );
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1, is_tile_data_v tile_shape_in2>
void TMUL_ADD_ROWSUM_T(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1, tile_shape_in2 &src2) {
  asm volatile(
    "BSTART.VPAR 0b0001100011, %c4\n"
    "B.IOT %1, %2, mask=15, 0, ->%0<%Z5>\n"
    "B.IOT %3, mask=15, last\n"
    "B.DIM %6, 0, ->lb0\n"
    "B.DIM %7, 0, ->lb1\n"

    ""
    : "=Tr"(dst.data())
    : "Tr"(src0.data()), "Tr"(src1.data()), "Tr"(src2.data()),
      "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(src0.GetValidCol()), "r"(src0.GetValidRow())
  );
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1, is_tile_data_v tile_shape_in2>
void TADD_MUL_EXPAND_T(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1, tile_shape_in2 &src2) {
  asm volatile(
    "BSTART.VPAR 0b0010000011, %c4\n"
    "B.IOT %1, %2, mask=15, 0, ->%0<%Z5>\n"
    "B.IOT %3, mask=15, last\n"
    "B.DIM %6, 0, ->lb0\n"
    "B.DIM %7, 0, ->lb1\n"

    ""
    : "=Tr"(dst.data())
    : "Tr"(src0.data()), "Tr"(src1.data()), "Tr"(src2.data()),
      "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(src0.GetValidCol()), "r"(src0.GetValidRow())
  );
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCVT_T(tile_shape_out &dst,  tile_shape_in &src) {
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TEPL 27, %c1\n"
    "B.DATR %c2, RNone\n"
    "B.IOT %3, mask=15, last, ->%0<%Z4>\n"
    "B.DIM %5, 0, ->lb0\n"
    "B.DIM %6, 0, ->lb1\n"
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(valid_col),
      "r"(valid_row)
  );
}

#define DEFINE_TMOV_LAYOUT(LAYOUT_NAME)                                          \
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>           \
void TMOV_##LAYOUT_NAME(tile_shape_out &dst, tile_shape_in &src) {               \
  asm volatile(                                                                  \
    "BSTART.TLSU 2, %c2\n"                                                        \
    "B.DATR " #LAYOUT_NAME ".normal, Null\n"                                     \
    "B.IOT %1, mask=15, last, ->%0<%Z3>\n"                                              \
    "B.DIM %4, 0, ->lb0\n"                                                   \
    "B.DIM %5, 0, ->lb1\n"                                                   \
    : "=Tr"(dst.data())                                                          \
    : "Tr"(src.data()),                                                          \
      "i"(type_traits<typename tile_shape_in::DType>::TypeCode),                 \
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),   \
      "r"(src.GetValidCol()),                                              \
      "r"(src.GetValidRow())                                               \
  );                                                                             \
}

DEFINE_TMOV_LAYOUT(ND2NZ)
DEFINE_TMOV_LAYOUT(NZ2ND)
DEFINE_TMOV_LAYOUT(ND2ZN)
DEFINE_TMOV_LAYOUT(DN2ZN)
DEFINE_TMOV_LAYOUT(DN2NZ)
DEFINE_TMOV_LAYOUT(NZ2DN)
DEFINE_TMOV_LAYOUT(NZ2ZN)
DEFINE_TMOV_LAYOUT(ZN2NZ)
DEFINE_TMOV_LAYOUT(NORM)

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TMOV_DN2NZ_DYN(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TLSU 2, %c2\n"
    "B.DATR DN2NZ.normal, Null\n"
    "B.IOT %1, mask=15, last, ->%0<%Z3>\n"
    "B.DIM %4, 0, ->lb0\n"
    "B.DIM %5, 0, ->lb1\n"

    : "=Tr"(dst.data())
    : "Tr"(src.data()),
      "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow())
  );
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void THISTOGRAM(tile_shape_out &dst, tile_shape_in &src, tile_shape_in &Idx, int ByteId) {
#define THISTOGRAM_ASM(BYTE_NAME)                                      \
  asm volatile(                                                        \
    "BSTART.TEPL 0b1101000, %c1\n"                                     \
    "B.DATR %c2," BYTE_NAME ",Null\n"                                  \
    "B.DIM %3, 0, ->LB0\n"                                         \
    "B.DIM %4, 0, ->LB1\n"                                         \
    "B.DIM zero, %c5, ->LB2\n"                                         \
    "B.IOT %6, %7, mask=15, last, ->%0<%Z8>\n"                                \
    ""                                                                 \
    : "=Tr"(dst.data())                                                \
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),       \
      "i"(type_traits<typename tile_shape_out::DType>::TypeCode),      \
      "r"(src.GetValidCol()),                                    \
      "r"(src.GetValidRow()),                                    \
      "i"(tile_shape_in::Cols),                                        \
      "Tr"(src.data()),                                                \
      "Tr"(Idx.data()),                                                \
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode))

  switch (ByteId) {
    case 0:
      THISTOGRAM_ASM("Byte0");
      break;
    case 1:
      THISTOGRAM_ASM("Byte1");
      break;
    case 2:
      THISTOGRAM_ASM("Byte2");
      break;
    case 3:
      THISTOGRAM_ASM("Byte3");
      break;
    default:
      return;  // ByteId > 3 或 < 0，无效
  }

#undef THISTOGRAM_ASM
}


template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD2_ND2NZ(tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Nz_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Nz ");
  asm volatile(
    "BSTART.TLSU TLOAD, %c[__pto_SrcType]\n"
    "B.DATR ND2NZ.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=15, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, last, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst1.GetValidCol()*2), [__pto_VROW]"r"(dst1.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*2),
      [__pto_GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD2_ND2ZN(tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Zn_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Zn ");
  asm volatile(
    "BSTART.TLSU TLOAD, %c[__pto_SrcType]\n"
    "B.DATR ND2ZN.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=15, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, last, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst1.GetValidCol()*2), [__pto_VROW]"r"(dst1.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*2),
      [__pto_GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD2_DN2ZN(tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(!gm_shape::isRowMajor && is_Nz_layout<tile_shape>::value,
                    "GM_SHAPE should DN and TILE_SHAPE should be Zn ");
  asm volatile(
    "BSTART.TLSU TLOAD, %c[__pto_SrcType]\n"
    "B.DATR DN2ZN.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=15, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, last, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst1.GetValidCol()), [__pto_VROW]"r"(dst1.GetValidRow()*2), [__pto_COL]"i"(tile_shape::Cols),
      [__pto_GmStride]"r"(gm_shape::ColStride * sizeof(typename gm_shape::DType))
  );
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TSTORE2_DN2DN(gm_shape &dst, tile_shape &src1, tile_shape &src0) {
  static_assert(!gm_shape::isRowMajor && !tile_shape::isRowMajor,
                    "GM_SHAPE should DN and TILE_SHAPE should be DN");
  asm volatile(
    "BSTART.TLSU TSTORE, %c[__pto_SrcType]\n"
    "B.DATR NORM.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT %[__pto_s0], %[s1], mask=15, last\n"
    "B.IOR [%[__pto_d0],%[__pto_GmStride]], []\n"
    : 
    : [__pto_d0]"r"(dst.data()), [__pto_s0]"Tr"(src0.data()), [s1]"Tr"(src1.data()),
      [__pto_DstType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_VCOL]"r"(src0.GetValidRow()*2), [__pto_VROW]"r"(src0.GetValidCol()), [__pto_COL]"i"(tile_shape::Rows*2),
      [__pto_GmStride]"r"(gm_shape::ColStride * sizeof(typename gm_shape::DType))
  );
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD4_ND2NZ(tile_shape &dst3, tile_shape &dst2, tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Nz_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Nz ");
  asm volatile(
    "BSTART.TLSU TLOAD, %c[__pto_SrcType]\n"
    "B.DATR ND2NZ.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=15, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, 0, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, 0, ->%[d2]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, last, ->%[d3]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data()),[d2]"=Tr"(dst2.data()),[d3]"=Tr"(dst3.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst3.GetValidCol()*4), [__pto_VROW]"r"(dst3.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*4),
      [__pto_GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD4_ND2ZN(tile_shape &dst3, tile_shape &dst2, tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Zn_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Nz ");
  asm volatile(
    "BSTART.TLSU TLOAD, %c[__pto_SrcType]\n"
    "B.DATR ND2ZN.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=15, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, 0, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, 0, ->%[d2]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, last, ->%[d3]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data()),[d2]"=Tr"(dst2.data()),[d3]"=Tr"(dst3.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst3.GetValidRow()*4), [__pto_VROW]"r"(dst3.GetValidCol()), [__pto_COL]"i"(tile_shape::Rows*4),
      [__pto_GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD4_DN2ZN(tile_shape &dst3, tile_shape &dst2, tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(!gm_shape::isRowMajor && is_Zn_layout<tile_shape>::value,
                    "GM_SHAPE should DN and TILE_SHAPE should be Zn ");
  asm volatile(
    "BSTART.TLSU TLOAD, %c[__pto_SrcType]\n"
    "B.DATR DN2ZN.normal, %c[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=15, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, 0, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, 0, ->%[d2]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=15, last, ->%[d3]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data()),[d2]"=Tr"(dst2.data()),[d3]"=Tr"(dst3.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst3.GetValidCol()*4), [__pto_VROW]"r"(dst3.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*4),
      [__pto_GmStride]"r"(gm_shape::ColStride * sizeof(typename gm_shape::DType))
  );
}

enum class TmaPadValue : int {
  Zero = 0,
  Max = 1,
  Min = 2,
  Null = 3,
};

template <typename tile_shape_out, typename tile_shape_offset, typename gm_shape,
          TmaPadValue Pad = TmaPadValue::Null>
inline void MGATHER(tile_shape_out &dst, const gm_shape &src,
                    const tile_shape_offset &offset) {
  static_assert(tile_shape_offset::ValidCol <= tile_shape_offset::Cols, "");
  static_assert(tile_type_traits<typename tile_shape_out::TileDType>::IsValidActiveSize,
                "MGATHER dst logical Tile size must be 512 B..32 KB (TSize=1..7) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MGATHER, %c[DataType]\n"
      "B.DATR Null\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[off], mask=15, last, ->%[dst]<%Z[TileSize]>\n"
      "B.IOR [%[base], %[GmStride]], []\n"
      : [dst] "=Tr"(dst.data())
      : [base] "r"(src.data()), [off] "Tr"(offset.data()),
        [DataType] "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        [PadValue] "i"(static_cast<int>(Pad)),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [ValidCol] "r"(offset.GetValidCol()),
        [ValidRow] "r"(offset.GetValidRow()),
        [Col] "i"(tile_shape_offset::Cols),
        [GmStride] "r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
      : "memory");
}

template <typename tile_shape_in, typename tile_shape_offset, typename gm_shape>
inline void MSCATTER(gm_shape &dst, const tile_shape_in &src,
                     const tile_shape_offset &offset) {
  static_assert(tile_shape_offset::ValidCol <= tile_shape_offset::Cols, "");
  static_assert(tile_type_traits<typename tile_shape_in::TileDType>::IsValidActiveSize,
                "MSCATTER src logical Tile size must be 512 B..32 KB (TSize=1..7) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MSCATTER, %c[DataType]\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[src], %[off], mask=15, last\n"
      "B.IOR [%[base], %[GmStride]], []\n"
      :
      : [base] "r"(dst.data()), [src] "Tr"(src.data()),
        [off] "Tr"(offset.data()),
        [DataType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [ValidCol] "r"(offset.GetValidCol()),
        [ValidRow] "r"(offset.GetValidRow()),
        [Col] "i"(tile_shape_offset::Cols),
        [GmStride] "r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
      : "memory");
}

template <typename tile_shape_out, typename tile_shape_offset,
          typename tile_shape_mask, typename gm_shape,
          TmaPadValue Pad = TmaPadValue::Null>
inline void MGATHER_MASK(tile_shape_out &dst, const gm_shape &src,
                         const tile_shape_offset &offset,
                         const tile_shape_mask &mask) {
  static_assert(tile_shape_offset::ValidCol <= tile_shape_offset::Cols, "");
  static_assert(tile_type_traits<typename tile_shape_out::TileDType>::IsValidActiveSize,
                "MGATHER_MASK dst logical Tile size must be 512 B..32 KB (TSize=1..7) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MGATHER.MASK, %c[DataType]\n"
      "B.DATR Null\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[off], mask=15, 0, ->%[dst]<%Z[TileSize]>\n"
      "B.IOT %[mask], mask=15, last\n"
      "B.IOR [%[base], %[GmStride]], []\n"
      : [dst] "=Tr"(dst.data())
      : [base] "r"(src.data()), [off] "Tr"(offset.data()),
        [mask] "Tr"(mask.data()),
        [DataType] "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        [PadValue] "i"(static_cast<int>(Pad)),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [ValidCol] "r"(offset.GetValidCol()),
        [ValidRow] "r"(offset.GetValidRow()),
        [Col] "i"(tile_shape_offset::Cols),
        [GmStride] "r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
      : "memory");
}

template <typename tile_shape_in, typename tile_shape_offset,
          typename tile_shape_mask, typename gm_shape>
inline void MSCATTER_MASK(gm_shape &dst, const tile_shape_in &src,
                          const tile_shape_offset &offset,
                          const tile_shape_mask &mask) {
  static_assert(tile_shape_offset::ValidCol <= tile_shape_offset::Cols, "");
  static_assert(tile_type_traits<typename tile_shape_in::TileDType>::IsValidActiveSize,
                "MSCATTER_MASK src logical Tile size must be 512 B..32 KB (TSize=1..7) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MSCATTER.MASK, %c[DataType]\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[src], %[off], mask=15, 0\n"
      "B.IOT %[mask], mask=15, last\n"
      "B.IOR [%[base], %[GmStride]], []\n"
      :
      : [base] "r"(dst.data()), [src] "Tr"(src.data()),
        [off] "Tr"(offset.data()), [mask] "Tr"(mask.data()),
        [DataType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [ValidCol] "r"(offset.GetValidCol()),
        [ValidRow] "r"(offset.GetValidRow()),
        [Col] "i"(tile_shape_offset::Cols),
        [GmStride] "r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
      : "memory");
}

#ifndef LINX_CVT_INLINE
#define LINX_CVT_INLINE __attribute__((always_inline)) inline
#endif

#ifndef SIMPLE_STORAGE
#define SIMPLE_STORAGE(d) (d)
#endif

// 默认生成：
//   v.cvt.xxx2yyy %1.src, ->%0.dst, RMode, sat
//
// 如果你的汇编器支持 "-> %0."，可以在 include 前改成：
//   #define LINX_CVT_DST_PREFIX ", -> %0."
//
// 如果你的汇编器完全不需要 "->"，可以在 include 前改成：
//   #define LINX_CVT_DST_PREFIX ", %0."
#ifndef LINX_CVT_DST_PREFIX
#define LINX_CVT_DST_PREFIX ", ->%0."
#endif

enum LinxRMode {
  LINX_RNONE = 0,
  LINX_RNE   = 1,
  LINX_RTZ   = 2,
  LINX_RDN   = 3,
  LINX_RUP   = 4,
  LINX_RNA   = 5,
  LINX_RTO   = 6,
  LINX_RHB   = 7,
};

enum LinxSat {
  LINX_NOSAT = 0,
  LINX_SAT   = 1,
};

template <class...>
struct linx_cvt_false {
  enum { value = 0 };
};

template <int RMode>
struct linx_valid_rmode {
  enum { value = RMode >= LINX_RNONE && RMode <= LINX_RHB };
};

template <int Sat>
struct linx_valid_sat {
  enum { value = Sat == LINX_NOSAT || Sat == LINX_SAT };
};

#define LINX_CVT_ASM(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG, RMODE, SAT)        \
  "v.cvt." SRC_TYPE "2" DST_TYPE " %1." SRC_REG LINX_CVT_DST_PREFIX DST_REG   \
  ", " RMODE ", " SAT "\n"

#define LINX_CVT_PACKED_ASM(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG, RMODE, SAT) \
  "v.cvt." SRC_TYPE "2" DST_TYPE " %1." SRC_REG ", %2." SRC_REG               \
  LINX_CVT_DST_PREFIX DST_REG ", " RMODE ", " SAT "\n"

#define LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,            \
                             DST_STORAGE, SRC_STORAGE, DST, SRC,              \
                             RMODE_STR, SAT_STR)                              \
  asm volatile(LINX_CVT_ASM(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,             \
                            RMODE_STR, SAT_STR)                               \
               : "=vr"(DST_STORAGE(DST))                                      \
               : "vr"(SRC_STORAGE(SRC)))

#define LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,            \
                             DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,        \
                             RMODE_STR, SAT_STR)                              \
  asm volatile(LINX_CVT_PACKED_ASM(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,      \
                                   RMODE_STR, SAT_STR)                        \
               : "=vr"(DST_STORAGE(DST))                                      \
               : "vr"(SRC_STORAGE(SRC0)),                                     \
                 "vr"(SRC_STORAGE(SRC1)))

#define LINX_CVT_DISPATCH_NORMAL(RMODE, SAT, SRC_TYPE, DST_TYPE, SRC_REG,     \
                                 DST_REG, DST_STORAGE, SRC_STORAGE, DST, SRC)  \
  do {                                                                        \
    static_assert(linx_valid_rmode<RMODE>::value, "invalid cvt RMode");       \
    static_assert(linx_valid_sat<SAT>::value, "invalid cvt Sat");             \
    if constexpr ((RMODE) == LINX_RNONE && (SAT) == LINX_NOSAT) {             \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RNONE", "nosat");                                 \
    } else if constexpr ((RMODE) == LINX_RNONE && (SAT) == LINX_SAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RNONE", "sat");                                   \
    } else if constexpr ((RMODE) == LINX_RNE && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RNE", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RNE && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RNE", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RTZ && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTZ", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RTZ && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTZ", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RDN && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RDN", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RDN && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RDN", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RUP", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RUP", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RNA && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RNA", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RNA && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RNA", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RTO && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTO", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RTO && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTO", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RHB && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RHB", "nosat");                                   \
    } else {                                                                  \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RHB", "sat");                                     \
    }                                                                         \
  } while (0)

#define LINX_CVT_DISPATCH_PACKED(RMODE, SAT, SRC_TYPE, DST_TYPE, SRC_REG,     \
                                 DST_REG, DST_STORAGE, SRC_STORAGE, DST,       \
                                 SRC0, SRC1)                                  \
  do {                                                                        \
    static_assert(linx_valid_rmode<RMODE>::value, "invalid cvt RMode");       \
    static_assert(linx_valid_sat<SAT>::value, "invalid cvt Sat");             \
    if constexpr ((RMODE) == LINX_RNONE && (SAT) == LINX_NOSAT) {             \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RNONE", "nosat");                                 \
    } else if constexpr ((RMODE) == LINX_RNONE && (SAT) == LINX_SAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RNONE", "sat");                                   \
    } else if constexpr ((RMODE) == LINX_RNE && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RNE", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RNE && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RNE", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RTZ && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTZ", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RTZ && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTZ", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RDN && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RDN", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RDN && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RDN", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RUP", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RUP", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RNA && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RNA", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RNA && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RNA", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RTO && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTO", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RTO && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTO", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RHB && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RHB", "nosat");                                   \
    } else {                                                                  \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RHB", "sat");                                     \
    }                                                                         \
  } while (0)

template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT, class Dst, class Src>
LINX_CVT_INLINE void linx_cvt(Dst &, const Src &) {
  static_assert(linx_cvt_false<Dst, Src>::value,
                "unsupported linx_cvt(dst, src) type pair");
}

template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT,
          class Dst, class Src0, class Src1>
LINX_CVT_INLINE void linx_cvt_packed(Dst &, const Src0 &, const Src1 &) {
  static_assert(linx_cvt_false<Dst, Src0, Src1>::value,
                "unsupported linx_cvt_packed(dst, src0, src1) type pair");
}

#define LINX_DEFINE_CVT(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE,              \
                        SRC_CPP, SRC_TYPE, SRC_REG, SRC_STORAGE)              \
  template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>                     \
  LINX_CVT_INLINE void linx_cvt(DST_CPP &dst, const SRC_CPP &src) {           \
    LINX_CVT_DISPATCH_NORMAL(RMode, Sat, SRC_TYPE, DST_TYPE, SRC_REG,         \
                             DST_REG, DST_STORAGE, SRC_STORAGE, dst, src);    \
  }

#define LINX_DEFINE_CVT_PACKED(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE,       \
                               SRC_CPP, SRC_TYPE, SRC_REG, SRC_STORAGE)       \
  template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>                     \
  LINX_CVT_INLINE void linx_cvt_packed(DST_CPP &dst,                          \
                                       const SRC_CPP &src0,                   \
                                       const SRC_CPP &src1) {                 \
    LINX_CVT_DISPATCH_PACKED(RMode, Sat, SRC_TYPE, DST_TYPE, SRC_REG,         \
                             DST_REG, DST_STORAGE, SRC_STORAGE, dst,          \
                             src0, src1);                                     \
  }

#define LINX_FOR_EACH_NORMAL_SRC(M, DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)  \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, double, "fp64", "fd",            \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, float, "fp32", "fs",             \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __tf32, "tf32", "fs",            \
    __tf32_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __hf32, "hf32", "fs",            \
    __hf32_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __half, "fp16", "fh",            \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __bf16, "bf16", "fh",            \
    __blkc_bf16_STORAGE)                                                      \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __hif8, "hif8", "fb",            \
    __hif8_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e4m3, "e4m3", "fb",        \
    __fp8_e4m3_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e5m2, "e5m2", "fb",        \
    __fp8_e5m2_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp6_e3m2, "e3m2", "fb",        \
    __fp6_e3m2_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp6_e2m3, "e2m3", "fb",        \
    __fp6_e2m3_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e8m0, "e8m0", "fb",        \
    __fp8_e8m0_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e6m2, "e6m2", "fb",        \
    __fp8_e6m2_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, long, "s64", "sd",               \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, int, "s32", "sw",                \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, short, "s16", "sh",              \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, char, "s8", "sb",                \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, unsigned long, "u64", "ud",      \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, unsigned int, "u32", "uw",       \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, unsigned short, "u16", "uh",     \
    SIMPLE_STORAGE)                                                           \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, unsigned char, "u8", "ub",       \
    SIMPLE_STORAGE)

#define LINX_FOR_EACH_NORMAL_DST(M)                                           \
  M(double, "fp64", "d", SIMPLE_STORAGE)                                      \
  M(float, "fp32", "w", SIMPLE_STORAGE)                                       \
  M(__tf32, "tf32", "w", __tf32_STORAGE)                                      \
  M(__hf32, "hf32", "w", __hf32_STORAGE)                                      \
  M(__half, "fp16", "h", SIMPLE_STORAGE)                                      \
  M(__bf16, "bf16", "h", __blkc_bf16_STORAGE)                                \
  M(__hif8, "hif8", "b", __hif8_STORAGE)                                      \
  M(__fp8_e4m3, "e4m3", "b", __fp8_e4m3_STORAGE)                             \
  M(__fp8_e5m2, "e5m2", "b", __fp8_e5m2_STORAGE)                             \
  M(__fp6_e3m2, "e3m2", "b", __fp6_e3m2_STORAGE)                             \
  M(__fp6_e2m3, "e2m3", "b", __fp6_e2m3_STORAGE)                             \
  M(__fp8_e8m0, "e8m0", "b", __fp8_e8m0_STORAGE)                             \
  M(__fp8_e6m2, "e6m2", "b", __fp8_e6m2_STORAGE)                             \
  M(long, "s64", "d", SIMPLE_STORAGE)                                        \
  M(int, "s32", "w", SIMPLE_STORAGE)                                         \
  M(short, "s16", "h", SIMPLE_STORAGE)                                       \
  M(char, "s8", "b", SIMPLE_STORAGE)                                         \
  M(unsigned long, "u64", "d", SIMPLE_STORAGE)                               \
  M(unsigned int, "u32", "w", SIMPLE_STORAGE)                                \
  M(unsigned short, "u16", "h", SIMPLE_STORAGE)                              \
  M(unsigned char, "u8", "b", SIMPLE_STORAGE)

#define LINX_FOR_EACH_PACKED_ONLY_SRC(M, DST_CPP, DST_TYPE, DST_REG,          \
                                      DST_STORAGE)                            \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp4_e2m1x2, "e2m1x2", "fb",    \
    __fp4_e2m1x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp4_e1m2x2, "e1m2x2", "fb",    \
    __fp4_e1m2x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp4_hif4x2, "hif4x2", "fb",    \
    __fp4_hif4x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e6m2x2, "e6m2x2", "fh",    \
    __fp8_e6m2x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e4m3x2, "e4m3x2", "fh",    \
    __fp8_e4m3x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e5m2x2, "e5m2x2", "fh",    \
    __fp8_e5m2x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp16x2, "fp16x2", "fh",        \
    __fp16x2_STORAGE)                                                         \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __uint4x2, "u4x2", "ub",         \
    __uint4x2_STORAGE)                                                        \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __int4x2, "s4x2", "sb",          \
    __int4x2_STORAGE)


#define LINX_DEFINE_CVT_TO(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)           \
  LINX_FOR_EACH_NORMAL_SRC(LINX_DEFINE_CVT, DST_CPP, DST_TYPE, DST_REG,       \
                           DST_STORAGE)

LINX_FOR_EACH_NORMAL_DST(LINX_DEFINE_CVT_TO)

// 明确禁止：packed/x2 source -> normal scalar/vector destination。
// 例如禁止生成：
//   v.cvt.e1m2x22s8 ...
//   v.cvt.fp16x22fp32 ...
//   v.cvt.u4x22s8 ...
#define LINX_DELETE_CVT(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE,              \
                        SRC_CPP, SRC_TYPE, SRC_REG, SRC_STORAGE)              \
  template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>                     \
  void linx_cvt(DST_CPP &, const SRC_CPP &) = delete;

#define LINX_DELETE_PACKED_SRC_TO_NORMAL_DST(DST_CPP, DST_TYPE, DST_REG,      \
                                             DST_STORAGE)                     \
  LINX_FOR_EACH_PACKED_ONLY_SRC(LINX_DELETE_CVT, DST_CPP, DST_TYPE, DST_REG,  \
                                DST_STORAGE)

LINX_FOR_EACH_NORMAL_DST(LINX_DELETE_PACKED_SRC_TO_NORMAL_DST)

//===----------------------------------------------------------------------===//
// Packed-to-Packed CVT
// one packed src -> one packed dst
//
// Supports:
//   x2 -> x2:
//     float x2: e2m1x2, e1m2x2, hif4x2, fp16x2, bf16x2
//     int   x2: u4x2, u16x2, s4x2, s16x2
//
//   x4 -> x4:
//     float x4: e4m3x4, e5m2x4
//     int   x4: u8x4, s8x4
//===----------------------------------------------------------------------===//

#define LINX_FOR_EACH_P2P_X2_SRC(M, DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)  \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp4_e2m1x2, "e2m1x2", "fb",    \
    __fp4_e2m1x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp4_e1m2x2, "e1m2x2", "fb",    \
    __fp4_e1m2x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp4_hif4x2, "hif4x2", "fb",    \
    __fp4_hif4x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e6m2x2, "e6m2x2", "fh",    \
    __fp8_e6m2x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e4m3x2, "e4m3x2", "fh",    \
    __fp8_e4m3x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e5m2x2, "e5m2x2", "fh",    \
    __fp8_e5m2x2_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp16x2, "fp16x2", "fh",        \
    __fp16x2_STORAGE)                                                         \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __bf16x2, "bf16x2", "fh",        \
    __bf16x2_STORAGE)                                                         \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __uint4x2, "u4x2", "ub",         \
    __uint4x2_STORAGE)                                                        \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __uint16x2, "u16x2", "uh",       \
    __uint16x2_STORAGE)                                                       \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __int4x2, "s4x2", "sb",          \
    __int4x2_STORAGE)                                                         \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __int16x2, "s16x2", "sh",        \
    __int16x2_STORAGE)


#define LINX_FOR_EACH_P2P_X2_DST(M)                                           \
  M(__fp4_e2m1x2, "e2m1x2", "b", __fp4_e2m1x2_STORAGE)                       \
  M(__fp4_e1m2x2, "e1m2x2", "b", __fp4_e1m2x2_STORAGE)                       \
  M(__fp4_hif4x2, "hif4x2", "b", __fp4_hif4x2_STORAGE)                       \
  M(__fp8_e6m2x2, "e6m2x2", "h", __fp8_e6m2x2_STORAGE)                       \
  M(__fp8_e4m3x2, "e4m3x2", "h", __fp8_e4m3x2_STORAGE)                       \
  M(__fp8_e5m2x2, "e5m2x2", "h", __fp8_e5m2x2_STORAGE)                       \
  M(__fp16x2, "fp16x2", "w", __fp16x2_STORAGE)                               \
  M(__bf16x2, "bf16x2", "w", __bf16x2_STORAGE)                               \
  M(__uint4x2, "u4x2", "b", __uint4x2_STORAGE)                               \
  M(__uint16x2, "u16x2", "w", __uint16x2_STORAGE)                            \
  M(__int4x2, "s4x2", "b", __int4x2_STORAGE)                                 \
  M(__int16x2, "s16x2", "w", __int16x2_STORAGE)


#define LINX_FOR_EACH_P2P_X4_SRC(M, DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)  \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e4m3x4, "e4m3x4", "fb",    \
    __fp8_e4m3x4_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __fp8_e5m2x4, "e5m2x4", "fb",    \
    __fp8_e5m2x4_STORAGE)                                                     \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __uint8x4, "u8x4", "ub",         \
    __uint8x4_STORAGE)                                                        \
  M(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE, __int8x4, "s8x4", "sb",          \
    __int8x4_STORAGE)

#define LINX_FOR_EACH_P2P_X4_DST(M)                                           \
  M(__fp8_e4m3x4, "e4m3x4", "w", __fp8_e4m3x4_STORAGE)                       \
  M(__fp8_e5m2x4, "e5m2x4", "w", __fp8_e5m2x4_STORAGE)                       \
  M(__uint8x4, "u8x4", "w", __uint8x4_STORAGE)                               \
  M(__int8x4, "s8x4", "w", __int8x4_STORAGE)

#define LINX_DEFINE_P2P_X2_CVT_TO(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)    \
  LINX_FOR_EACH_P2P_X2_SRC(LINX_DEFINE_CVT, DST_CPP, DST_TYPE, DST_REG,       \
                           DST_STORAGE)

#define LINX_DEFINE_P2P_X4_CVT_TO(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)    \
  LINX_FOR_EACH_P2P_X4_SRC(LINX_DEFINE_CVT, DST_CPP, DST_TYPE, DST_REG,       \
                           DST_STORAGE)

LINX_FOR_EACH_P2P_X2_DST(LINX_DEFINE_P2P_X2_CVT_TO)
LINX_FOR_EACH_P2P_X4_DST(LINX_DEFINE_P2P_X4_CVT_TO)

// packed destination 只支持：
//   normal src0 + normal src1 -> packed dst
//
// 不支持：
//   packed src -> normal dst
//   packed src0 + packed src1 -> packed dst
#define LINX_FOR_EACH_PACKED_SRC(M, DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)  \
  LINX_FOR_EACH_NORMAL_SRC(M, DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)

#define LINX_FOR_EACH_PACKED_DST(M)                                           \
  M(__fp4_e2m1x2, "e2m1x2", "b", __fp4_e2m1x2_STORAGE)                       \
  M(__fp4_e1m2x2, "e1m2x2", "b", __fp4_e1m2x2_STORAGE)                       \
  M(__fp4_hif4x2, "hif4x2", "b", __fp4_hif4x2_STORAGE)                       \
  M(__fp8_e6m2x2, "e6m2x2", "h", __fp8_e6m2x2_STORAGE)                       \
  M(__fp8_e4m3x2, "e4m3x2", "h", __fp8_e4m3x2_STORAGE)                       \
  M(__fp8_e5m2x2, "e5m2x2", "h", __fp8_e5m2x2_STORAGE)                       \
  M(__fp16x2, "fp16x2", "w", __fp16x2_STORAGE)                               \
  M(__bf16x2, "bf16x2", "w", __bf16x2_STORAGE)                               \
  M(__uint4x2, "u4x2", "b", __uint4x2_STORAGE)                               \
  M(__int4x2, "s4x2", "b", __int4x2_STORAGE)


#define LINX_DEFINE_PACKED_CVT_TO(DST_CPP, DST_TYPE, DST_REG, DST_STORAGE)    \
  LINX_FOR_EACH_PACKED_SRC(LINX_DEFINE_CVT_PACKED, DST_CPP, DST_TYPE,         \
                           DST_REG, DST_STORAGE)

LINX_FOR_EACH_PACKED_DST(LINX_DEFINE_PACKED_CVT_TO)

typedef __attribute__((address_space(6))) __fp4_e1m2x2
    __fp4_e1m2x2_as6;

typedef __attribute__((address_space(6))) __bf16x2
    __bf16x2_as6;

typedef __attribute__((address_space(6))) __fp8_e6m2x2
    __fp8_e6m2x2_as6;

typedef __attribute__((address_space(6))) __fp16x2
    __fp16x2_as6;

// __bf16x2 -> address_space(6) __fp8_e6m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp8_e6m2x2_as6 &dst,
                              const __bf16x2 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "bf16x2", "e6m2x2",
                           "fh", "h",
                           __fp8_e6m2x2_STORAGE,
                           __bf16x2_STORAGE,
                           dst, src);
}


// address_space(6) __bf16x2 -> __fp8_e6m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp8_e6m2x2 &dst,
                              const __bf16x2_as6 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "bf16x2", "e6m2x2",
                           "fh", "h",
                           __fp8_e6m2x2_STORAGE,
                           __bf16x2_STORAGE,
                           dst, src);
}


// address_space(6) __bf16x2 -> address_space(6) __fp8_e6m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp8_e6m2x2_as6 &dst,
                              const __bf16x2_as6 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "bf16x2", "e6m2x2",
                           "fh", "h",
                           __fp8_e6m2x2_STORAGE,
                           __bf16x2_STORAGE,
                           dst, src);
}

// bf16x2 -> address_space(6) __fp4_e1m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp4_e1m2x2_as6 &dst,
                              const __bf16x2 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "bf16x2", "e1m2x2",
                           "fh", "b",
                           __fp4_e1m2x2_STORAGE,
                           __bf16x2_STORAGE,
                           dst, src);
}


// raw __blkc_bf16 + raw __blkc_bf16 -> address_space(6) __fp4_e1m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt_packed(__fp4_e1m2x2_as6 &dst,
                                     const __blkc_bf16 &src0,
                                     const __blkc_bf16 &src1) {
  LINX_CVT_DISPATCH_PACKED(RMode, Sat,
                           "bf16", "e1m2x2",
                           "fh", "b",
                           __fp4_e1m2x2_STORAGE,
                           SIMPLE_STORAGE,
                           dst, src0, src1);
}


// float + float -> address_space(6) __bf16x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt_packed(__bf16x2_as6 &dst,
                                     const float &src0,
                                     const float &src1) {
  LINX_CVT_DISPATCH_PACKED(RMode, Sat,
                           "fp32", "bf16x2",
                           "fs", "w",
                           __bf16x2_STORAGE,
                           SIMPLE_STORAGE,
                           dst, src0, src1);
}

typedef __attribute__((address_space(6))) float float_as6;


// address_space(6) float -> raw __blkc_bf16
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__blkc_bf16 &dst,
                              const float_as6 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "fp32", "bf16",
                           "fs", "h",
                           SIMPLE_STORAGE,
                           SIMPLE_STORAGE,
                           dst, src);
}

// __fp16x2 -> address_space(6) __fp8_e6m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp8_e6m2x2_as6 &dst,
                              const __fp16x2 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "fp16x2", "e6m2x2",
                           "fh", "h",
                           __fp8_e6m2x2_STORAGE,
                           __fp16x2_STORAGE,
                           dst, src);
}


// address_space(6) __fp8_e6m2x2 -> __fp16x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp16x2 &dst,
                              const __fp8_e6m2x2_as6 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "e6m2x2", "fp16x2",
                           "fh", "w",
                           __fp16x2_STORAGE,
                           __fp8_e6m2x2_STORAGE,
                           dst, src);
}


// address_space(6) __fp16x2 -> __fp8_e6m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp8_e6m2x2 &dst,
                              const __fp16x2_as6 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "fp16x2", "e6m2x2",
                           "fh", "h",
                           __fp8_e6m2x2_STORAGE,
                           __fp16x2_STORAGE,
                           dst, src);
}


// address_space(6) __fp16x2 -> address_space(6) __fp8_e6m2x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt(__fp8_e6m2x2_as6 &dst,
                              const __fp16x2_as6 &src) {
  LINX_CVT_DISPATCH_NORMAL(RMode, Sat,
                           "fp16x2", "e6m2x2",
                           "fh", "h",
                           __fp8_e6m2x2_STORAGE,
                           __fp16x2_STORAGE,
                           dst, src);
}

typedef __attribute__((address_space(6))) __blkc_bf16
    __blkc_bf16_as6;


// address_space(6) __blkc_bf16 + address_space(6) __blkc_bf16 -> __bf16x2
template <int RMode = LINX_RNONE, int Sat = LINX_NOSAT>
LINX_CVT_INLINE void linx_cvt_packed(__bf16x2 &dst,
                                     const __blkc_bf16_as6 &src0,
                                     const __blkc_bf16_as6 &src1) {
  LINX_CVT_DISPATCH_PACKED(RMode, Sat,
                           "bf16", "bf16x2",
                           "fh", "w",
                           __bf16x2_STORAGE,
                           SIMPLE_STORAGE,
                           dst, src0, src1);
}

template <class Dst, class Src>
LINX_CVT_INLINE Dst linx_cvt_as(const Src &src) {
  Dst dst;
  linx_cvt(dst, src);
  return dst;
}

template <int RMode, int Sat, class Dst, class Src>
LINX_CVT_INLINE Dst linx_cvt_as(const Src &src) {
  Dst dst;
  linx_cvt<RMode, Sat>(dst, src);
  return dst;
}

template <class Dst, class Src>
LINX_CVT_INLINE void linx_cvt_package(Dst &dst,
                                      const Src &src0,
                                      const Src &src1) {
  linx_cvt_packed(dst, src0, src1);
}

template <int RMode, int Sat, class Dst, class Src>
LINX_CVT_INLINE void linx_cvt_package(Dst &dst,
                                      const Src &src0,
                                      const Src &src1) {
  linx_cvt_packed<RMode, Sat>(dst, src0, src1);
}

template <class Dst, class Src>
LINX_CVT_INLINE Dst linx_cvt_package_as(const Src &src0, const Src &src1) {
  Dst dst;
  linx_cvt_package(dst, src0, src1);
  return dst;
}

template <int RMode, int Sat, class Dst, class Src>
LINX_CVT_INLINE Dst linx_cvt_package_as(const Src &src0, const Src &src1) {
  Dst dst;
  linx_cvt_package<RMode, Sat>(dst, src0, src1);
  return dst;
}


#ifndef BLKV_BF16_OPS_HPP
#define BLKV_BF16_OPS_HPP

#ifndef BLKV_BF16_INLINE
#define BLKV_BF16_INLINE __attribute__((always_inline)) inline
#endif

#ifndef BLKV_BF16_STORAGE
#ifdef __blkc_bf16_STORAGE
#define BLKV_BF16_STORAGE(d) __blkc_bf16_STORAGE(d)
#else
#define BLKV_BF16_STORAGE(d) ((d).data)
#endif
#endif

// 默认生成：", -> %0.h"
// 如果你的 asm parser 使用新格式 ", %0.h"，可在 include 前定义：
// #define BLKV_BF16_DST_PREFIX ", %0.h"
#ifndef BLKV_BF16_DST_PREFIX
#define BLKV_BF16_DST_PREFIX ", -> %0.h"
#endif

// 如果后端实际 mnemonic 是 v.musb，可在 include 前定义：
// #define BLKV_BF16_FMSUB_MNEMONIC "musb"
#ifndef BLKV_BF16_FMSUB_MNEMONIC
#define BLKV_BF16_FMSUB_MNEMONIC "fmsub"
#endif

#ifndef BLKV_BF16X2_FMSUB_MNEMONIC
#define BLKV_BF16X2_FMSUB_MNEMONIC BLKV_BF16_FMSUB_MNEMONIC
#endif

enum BlkvBf16RMode {
  BLKV_RNONE = 0,
  BLKV_RNE   = 1,
  BLKV_RTZ   = 2,
  BLKV_RDN   = 3,
  BLKV_RUP   = 4,
  BLKV_RNA   = 5,
  BLKV_RTO   = 6,
  BLKV_RHB   = 7,
};

enum BlkvBf16Sat {
  BLKV_NOSAT = 0,
  BLKV_SAT   = 1,
};

#define BLKV_BF16_EMIT_UNARY(OP, DST, SRC, RMODE_STR, SAT_STR)               \
  asm volatile("v." OP " %1.bf" BLKV_BF16_DST_PREFIX ", " RMODE_STR           \
               ", " SAT_STR "\n"                                              \
               : "=vr"(BLKV_BF16_STORAGE(DST))                               \
               : "vr"(BLKV_BF16_STORAGE(SRC)))

#define BLKV_BF16_EMIT_BINARY(OP, DST, SRC_L, SRC_R, RMODE_STR, SAT_STR)      \
  asm volatile("v." OP " %1.bf, %2.bf" BLKV_BF16_DST_PREFIX ", " RMODE_STR    \
               ", " SAT_STR "\n"                                              \
               : "=vr"(BLKV_BF16_STORAGE(DST))                               \
               : "vr"(BLKV_BF16_STORAGE(SRC_L)),                              \
                 "vr"(BLKV_BF16_STORAGE(SRC_R)))

#define BLKV_BF16_EMIT_TERNARY(OP, DST, SRC_L, SRC_R, SRC_A,                 \
                               RMODE_STR, SAT_STR)                           \
  asm volatile("v." OP " %1.bf, %2.bf, %3.bf" BLKV_BF16_DST_PREFIX ", "       \
               RMODE_STR ", " SAT_STR "\n"                                    \
               : "=vr"(BLKV_BF16_STORAGE(DST))                               \
               : "vr"(BLKV_BF16_STORAGE(SRC_L)),                              \
                 "vr"(BLKV_BF16_STORAGE(SRC_R)),                              \
                 "vr"(BLKV_BF16_STORAGE(SRC_A)))

#define BLKV_BF16_EMIT_FMAX(DST, SRC_L, SRC_R)                                \
  asm volatile("v.fmax %1.bf, %2.bf" BLKV_BF16_DST_PREFIX "\n"                \
               : "=vr"(BLKV_BF16_STORAGE(DST))                               \
               : "vr"(BLKV_BF16_STORAGE(SRC_L)),                              \
                 "vr"(BLKV_BF16_STORAGE(SRC_R)))

#define BLKV_BF16_DISPATCH_RMODE_SAT(RMODE, SAT, EMIT, ...)                  \
  do {                                                                        \
    static_assert((RMODE) >= 0 && (RMODE) <= 7, "invalid bf16 RMode");        \
    static_assert((SAT) == 0 || (SAT) == 1, "invalid bf16 Sat");              \
    if constexpr ((RMODE) == 0 && (SAT) == 0) {                               \
      EMIT(__VA_ARGS__, "RNONE", "nosat");                                    \
    } else if constexpr ((RMODE) == 0 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RNONE", "sat");                                      \
    } else if constexpr ((RMODE) == 1 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RNE", "nosat");                                      \
    } else if constexpr ((RMODE) == 1 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RNE", "sat");                                        \
    } else if constexpr ((RMODE) == 2 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RTZ", "nosat");                                      \
    } else if constexpr ((RMODE) == 2 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RTZ", "sat");                                        \
    } else if constexpr ((RMODE) == 3 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RDN", "nosat");                                      \
    } else if constexpr ((RMODE) == 3 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RDN", "sat");                                        \
    } else if constexpr ((RMODE) == 4 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RUP", "nosat");                                      \
    } else if constexpr ((RMODE) == 4 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RUP", "sat");                                        \
    } else if constexpr ((RMODE) == 5 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RNA", "nosat");                                      \
    } else if constexpr ((RMODE) == 5 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RNA", "sat");                                        \
    } else if constexpr ((RMODE) == 6 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RTO", "nosat");                                      \
    } else if constexpr ((RMODE) == 6 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RTO", "sat");                                        \
    } else if constexpr ((RMODE) == 7 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RHB", "nosat");                                      \
    } else {                                                                  \
      EMIT(__VA_ARGS__, "RHB", "sat");                                        \
    }                                                                         \
  } while (0)

#define BLKV_DEFINE_BF16_UNARY(NAME, OP)                                      \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT, class Src>         \
  BLKV_BF16_INLINE void blkv_bf16_##NAME(__bf16 &dst,                         \
                                         const Src &src) {                    \
    BLKV_BF16_DISPATCH_RMODE_SAT(RMode, Sat, BLKV_BF16_EMIT_UNARY,            \
                                 OP, dst, src);                               \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT, class Src>         \
  BLKV_BF16_INLINE __bf16 blkv_bf16_##NAME(const Src &src) {                  \
    __bf16 dst;                                                               \
    blkv_bf16_##NAME<RMode, Sat>(dst, src);                                   \
    return dst;                                                               \
  }

#define BLKV_DEFINE_BF16_BINARY(NAME, OP)                                     \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR>                                           \
  BLKV_BF16_INLINE void blkv_bf16_##NAME(__bf16 &dst,                         \
                                         const SrcL &src_l,                   \
                                         const SrcR &src_r) {                 \
    BLKV_BF16_DISPATCH_RMODE_SAT(RMode, Sat, BLKV_BF16_EMIT_BINARY,           \
                                 OP, dst, src_l, src_r);                      \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR>                                           \
  BLKV_BF16_INLINE __bf16 blkv_bf16_##NAME(const SrcL &src_l,                 \
                                           const SrcR &src_r) {               \
    __bf16 dst;                                                               \
    blkv_bf16_##NAME<RMode, Sat>(dst, src_l, src_r);                          \
    return dst;                                                               \
  }

#define BLKV_DEFINE_BF16_TERNARY(NAME, OP)                                    \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR, class SrcA>                               \
  BLKV_BF16_INLINE void blkv_bf16_##NAME(__bf16 &dst,                         \
                                         const SrcL &src_l,                   \
                                         const SrcR &src_r,                   \
                                         const SrcA &src_a) {                 \
    BLKV_BF16_DISPATCH_RMODE_SAT(RMode, Sat, BLKV_BF16_EMIT_TERNARY,          \
                                 OP, dst, src_l, src_r, src_a);               \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR, class SrcA>                               \
  BLKV_BF16_INLINE __bf16 blkv_bf16_##NAME(const SrcL &src_l,                 \
                                           const SrcR &src_r,                 \
                                           const SrcA &src_a) {               \
    __bf16 dst;                                                               \
    blkv_bf16_##NAME<RMode, Sat>(dst, src_l, src_r, src_a);                   \
    return dst;                                                               \
  }


#define BLKV_DEFINE_BF16_UNARY_ALIAS(ALIAS, TARGET)                           \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE void blkv_bf16_##ALIAS(__bf16 &dst,                        \
                                          const __bf16 &src) {                \
    blkv_bf16_##TARGET<RMode, Sat>(dst, src);                                 \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE __bf16 blkv_bf16_##ALIAS(const __bf16 &src) {              \
    return blkv_bf16_##TARGET<RMode, Sat>(src);                               \
  }

#define BLKV_DEFINE_BF16_BINARY_ALIAS(ALIAS, TARGET)                          \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE void blkv_bf16_##ALIAS(__bf16 &dst,                        \
                                          const __bf16 &src_l,                \
                                          const __bf16 &src_r) {              \
    blkv_bf16_##TARGET<RMode, Sat>(dst, src_l, src_r);                        \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE __bf16 blkv_bf16_##ALIAS(const __bf16 &src_l,              \
                                            const __bf16 &src_r) {            \
    return blkv_bf16_##TARGET<RMode, Sat>(src_l, src_r);                      \
  }

#define BLKV_DEFINE_BF16_TERNARY_ALIAS(ALIAS, TARGET)                         \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE void blkv_bf16_##ALIAS(__bf16 &dst,                        \
                                          const __bf16 &src_l,                \
                                          const __bf16 &src_r,                \
                                          const __bf16 &src_a) {              \
    blkv_bf16_##TARGET<RMode, Sat>(dst, src_l, src_r, src_a);                 \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE __bf16 blkv_bf16_##ALIAS(const __bf16 &src_l,              \
                                            const __bf16 &src_r,              \
                                            const __bf16 &src_a) {            \
    return blkv_bf16_##TARGET<RMode, Sat>(src_l, src_r, src_a);               \
  }

// v.fadd / v.fsub / v.fmul / v.fdiv
BLKV_DEFINE_BF16_BINARY(fadd, "fadd")
BLKV_DEFINE_BF16_BINARY(fsub, "fsub")
BLKV_DEFINE_BF16_BINARY(fmul, "fmul")
BLKV_DEFINE_BF16_BINARY(fdiv, "fdiv")

// v.fexp
BLKV_DEFINE_BF16_UNARY(fexp, "fexp")

// v.fsqrt
BLKV_DEFINE_BF16_UNARY(fsqrt, "fsqrt")

// v.fmadd / v.fmsub
BLKV_DEFINE_BF16_TERNARY(fmadd, "fmadd")
BLKV_DEFINE_BF16_TERNARY(fmsub, BLKV_BF16_FMSUB_MNEMONIC)

// v.fmax: no rmode / sat
template <class SrcL, class SrcR>
BLKV_BF16_INLINE void blkv_bf16_fmax(__bf16 &dst,
                                     const SrcL &src_l,
                                     const SrcR &src_r) {
  BLKV_BF16_EMIT_FMAX(dst, src_l, src_r);
}

template <class SrcL, class SrcR>
BLKV_BF16_INLINE __bf16 blkv_bf16_fmax(const SrcL &src_l,
                                       const SrcR &src_r) {
  __bf16 dst;
  blkv_bf16_fmax(dst, src_l, src_r);
  return dst;
}


// short aliases
BLKV_DEFINE_BF16_BINARY_ALIAS(add, fadd)
BLKV_DEFINE_BF16_BINARY_ALIAS(sub, fsub)
BLKV_DEFINE_BF16_BINARY_ALIAS(mul, fmul)
BLKV_DEFINE_BF16_BINARY_ALIAS(div, fdiv)

BLKV_DEFINE_BF16_UNARY_ALIAS(exp, fexp)
BLKV_DEFINE_BF16_UNARY_ALIAS(sqrt, fsqrt)

BLKV_DEFINE_BF16_TERNARY_ALIAS(madd, fmadd)
BLKV_DEFINE_BF16_TERNARY_ALIAS(fmmad, fmadd)
BLKV_DEFINE_BF16_TERNARY_ALIAS(msub, fmsub)

BLKV_BF16_INLINE void blkv_bf16_max(__bf16 &dst,
                                    const __bf16 &src_l,
                                    const __bf16 &src_r) {
  blkv_bf16_fmax(dst, src_l, src_r);
}

BLKV_BF16_INLINE __bf16 blkv_bf16_max(const __bf16 &src_l,
                                      const __bf16 &src_r) {
  return blkv_bf16_fmax(src_l, src_r);
}

//===----------------------------------------------------------------------===//
// bf16x2 ops
//===----------------------------------------------------------------------===//

#ifndef BLKV_BF16X2_TYPE
#define BLKV_BF16X2_TYPE __bf16x2
#endif

#ifndef BLKV_BF16X2_STORAGE
#ifdef __blkc_bf16x2_STORAGE
#define BLKV_BF16X2_STORAGE(d) __blkc_bf16x2_STORAGE(d)
#else
#define BLKV_BF16X2_STORAGE(d) ((d).data)
#endif
#endif

// 默认生成：", -> %0.h"
// 如果你的 asm parser 使用新格式 ", %0.h"，可在 include 前定义：
// #define BLKV_BF16X2_DST_PREFIX ", %0.h"
// 如果目的类型也需要 x2，比如 ", -> %0.hx2"，也可以在 include 前改。
#ifndef BLKV_BF16X2_DST_PREFIX
#define BLKV_BF16X2_DST_PREFIX ", -> %0.w"
#endif

#define BLKV_BF16X2_EMIT_UNARY(OP, DST, SRC, RMODE_STR, SAT_STR)             \
  asm volatile("v." OP " %1.bfx2" BLKV_BF16X2_DST_PREFIX ", " RMODE_STR       \
               ", " SAT_STR "\n"                                              \
               : "=vr"(BLKV_BF16X2_STORAGE(DST))                             \
               : "vr"(BLKV_BF16X2_STORAGE(SRC)))

#define BLKV_BF16X2_EMIT_BINARY(OP, DST, SRC_L, SRC_R, RMODE_STR, SAT_STR)    \
  asm volatile("v." OP " %1.bfx2, %2.bfx2" BLKV_BF16X2_DST_PREFIX ", "        \
               RMODE_STR ", " SAT_STR "\n"                                    \
               : "=vr"(BLKV_BF16X2_STORAGE(DST))                             \
               : "vr"(BLKV_BF16X2_STORAGE(SRC_L)),                            \
                 "vr"(BLKV_BF16X2_STORAGE(SRC_R)))

#define BLKV_BF16X2_EMIT_TERNARY(OP, DST, SRC_L, SRC_R, SRC_A,               \
                                  RMODE_STR, SAT_STR)                       \
  asm volatile("v." OP " %1.bfx2, %2.bfx2, %3.bfx2"                          \
               BLKV_BF16X2_DST_PREFIX ", " RMODE_STR ", " SAT_STR "\n"       \
               : "=vr"(BLKV_BF16X2_STORAGE(DST))                             \
               : "vr"(BLKV_BF16X2_STORAGE(SRC_L)),                            \
                 "vr"(BLKV_BF16X2_STORAGE(SRC_R)),                            \
                 "vr"(BLKV_BF16X2_STORAGE(SRC_A)))

#define BLKV_BF16X2_EMIT_FMAX(DST, SRC_L, SRC_R)                              \
  asm volatile("v.fmax %1.bfx2, %2.bfx2" BLKV_BF16X2_DST_PREFIX "\n"          \
               : "=vr"(BLKV_BF16X2_STORAGE(DST))                             \
               : "vr"(BLKV_BF16X2_STORAGE(SRC_L)),                            \
                 "vr"(BLKV_BF16X2_STORAGE(SRC_R)))

#define BLKV_DEFINE_BF16X2_UNARY(NAME, OP)                                    \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT, class Src>         \
  BLKV_BF16_INLINE void blkv_bf16x2_##NAME(BLKV_BF16X2_TYPE &dst,             \
                                           const Src &src) {                  \
    BLKV_BF16_DISPATCH_RMODE_SAT(RMode, Sat, BLKV_BF16X2_EMIT_UNARY,          \
                                 OP, dst, src);                               \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT, class Src>         \
  BLKV_BF16_INLINE BLKV_BF16X2_TYPE blkv_bf16x2_##NAME(const Src &src) {      \
    BLKV_BF16X2_TYPE dst;                                                     \
    blkv_bf16x2_##NAME<RMode, Sat>(dst, src);                                 \
    return dst;                                                               \
  }

#define BLKV_DEFINE_BF16X2_BINARY(NAME, OP)                                   \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR>                                           \
  BLKV_BF16_INLINE void blkv_bf16x2_##NAME(BLKV_BF16X2_TYPE &dst,             \
                                           const SrcL &src_l,                 \
                                           const SrcR &src_r) {               \
    BLKV_BF16_DISPATCH_RMODE_SAT(RMode, Sat, BLKV_BF16X2_EMIT_BINARY,         \
                                 OP, dst, src_l, src_r);                      \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR>                                           \
  BLKV_BF16_INLINE BLKV_BF16X2_TYPE blkv_bf16x2_##NAME(const SrcL &src_l,     \
                                                       const SrcR &src_r) {   \
    BLKV_BF16X2_TYPE dst;                                                     \
    blkv_bf16x2_##NAME<RMode, Sat>(dst, src_l, src_r);                        \
    return dst;                                                               \
  }

#define BLKV_DEFINE_BF16X2_TERNARY(NAME, OP)                                 \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR, class SrcA>                               \
  BLKV_BF16_INLINE void blkv_bf16x2_##NAME(BLKV_BF16X2_TYPE &dst,             \
                                           const SrcL &src_l,                 \
                                           const SrcR &src_r,                 \
                                           const SrcA &src_a) {               \
    BLKV_BF16_DISPATCH_RMODE_SAT(RMode, Sat, BLKV_BF16X2_EMIT_TERNARY,        \
                                 OP, dst, src_l, src_r, src_a);               \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT,                    \
            class SrcL, class SrcR, class SrcA>                               \
  BLKV_BF16_INLINE BLKV_BF16X2_TYPE                                           \
  blkv_bf16x2_##NAME(const SrcL &src_l,                                       \
                     const SrcR &src_r,                                       \
                     const SrcA &src_a) {                                     \
    BLKV_BF16X2_TYPE dst;                                                     \
    blkv_bf16x2_##NAME<RMode, Sat>(dst, src_l, src_r, src_a);                 \
    return dst;                                                               \
  }

#define BLKV_DEFINE_BF16X2_UNARY_ALIAS(ALIAS, TARGET)                         \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE void blkv_bf16x2_##ALIAS(BLKV_BF16X2_TYPE &dst,            \
                                            const BLKV_BF16X2_TYPE &src) {    \
    blkv_bf16x2_##TARGET<RMode, Sat>(dst, src);                               \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE BLKV_BF16X2_TYPE                                           \
  blkv_bf16x2_##ALIAS(const BLKV_BF16X2_TYPE &src) {                          \
    return blkv_bf16x2_##TARGET<RMode, Sat>(src);                             \
  }

#define BLKV_DEFINE_BF16X2_BINARY_ALIAS(ALIAS, TARGET)                        \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE void blkv_bf16x2_##ALIAS(BLKV_BF16X2_TYPE &dst,            \
                                            const BLKV_BF16X2_TYPE &src_l,    \
                                            const BLKV_BF16X2_TYPE &src_r) {  \
    blkv_bf16x2_##TARGET<RMode, Sat>(dst, src_l, src_r);                      \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE BLKV_BF16X2_TYPE                                           \
  blkv_bf16x2_##ALIAS(const BLKV_BF16X2_TYPE &src_l,                          \
                      const BLKV_BF16X2_TYPE &src_r) {                        \
    return blkv_bf16x2_##TARGET<RMode, Sat>(src_l, src_r);                    \
  }

#define BLKV_DEFINE_BF16X2_TERNARY_ALIAS(ALIAS, TARGET)                      \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE void blkv_bf16x2_##ALIAS(BLKV_BF16X2_TYPE &dst,            \
                                            const BLKV_BF16X2_TYPE &src_l,    \
                                            const BLKV_BF16X2_TYPE &src_r,    \
                                            const BLKV_BF16X2_TYPE &src_a) {  \
    blkv_bf16x2_##TARGET<RMode, Sat>(dst, src_l, src_r, src_a);               \
  }                                                                           \
                                                                              \
  template <int RMode = BLKV_RNONE, int Sat = BLKV_NOSAT>                    \
  BLKV_BF16_INLINE BLKV_BF16X2_TYPE                                           \
  blkv_bf16x2_##ALIAS(const BLKV_BF16X2_TYPE &src_l,                          \
                      const BLKV_BF16X2_TYPE &src_r,                          \
                      const BLKV_BF16X2_TYPE &src_a) {                        \
    return blkv_bf16x2_##TARGET<RMode, Sat>(src_l, src_r, src_a);             \
  }

// v.fadd / v.fsub / v.fmul / v.fdiv
BLKV_DEFINE_BF16X2_BINARY(fadd, "fadd")
BLKV_DEFINE_BF16X2_BINARY(fsub, "fsub")
BLKV_DEFINE_BF16X2_BINARY(fmul, "fmul")
BLKV_DEFINE_BF16X2_BINARY(fdiv, "fdiv")

// v.fexp
BLKV_DEFINE_BF16X2_UNARY(fexp, "fexp")

// v.fsqrt
BLKV_DEFINE_BF16X2_UNARY(fsqrt, "fsqrt")

// v.fmadd / v.fmsub
BLKV_DEFINE_BF16X2_TERNARY(fmadd, "fmadd")
BLKV_DEFINE_BF16X2_TERNARY(fmsub, BLKV_BF16X2_FMSUB_MNEMONIC)

// v.fmax: no rmode / sat
template <class SrcL, class SrcR>
BLKV_BF16_INLINE void blkv_bf16x2_fmax(BLKV_BF16X2_TYPE &dst,
                                       const SrcL &src_l,
                                       const SrcR &src_r) {
  BLKV_BF16X2_EMIT_FMAX(dst, src_l, src_r);
}

template <class SrcL, class SrcR>
BLKV_BF16_INLINE BLKV_BF16X2_TYPE blkv_bf16x2_fmax(const SrcL &src_l,
                                                   const SrcR &src_r) {
  BLKV_BF16X2_TYPE dst;
  blkv_bf16x2_fmax(dst, src_l, src_r);
  return dst;
}

// short aliases
BLKV_DEFINE_BF16X2_BINARY_ALIAS(add, fadd)
BLKV_DEFINE_BF16X2_BINARY_ALIAS(sub, fsub)
BLKV_DEFINE_BF16X2_BINARY_ALIAS(mul, fmul)
BLKV_DEFINE_BF16X2_BINARY_ALIAS(div, fdiv)

BLKV_DEFINE_BF16X2_UNARY_ALIAS(exp, fexp)
BLKV_DEFINE_BF16X2_UNARY_ALIAS(sqrt, fsqrt)

BLKV_DEFINE_BF16X2_TERNARY_ALIAS(madd, fmadd)
BLKV_DEFINE_BF16X2_TERNARY_ALIAS(fmmad, fmadd)
BLKV_DEFINE_BF16X2_TERNARY_ALIAS(msub, fmsub)

BLKV_BF16_INLINE void blkv_bf16x2_max(BLKV_BF16X2_TYPE &dst,
                                      const BLKV_BF16X2_TYPE &src_l,
                                      const BLKV_BF16X2_TYPE &src_r) {
  blkv_bf16x2_fmax(dst, src_l, src_r);
}

BLKV_BF16_INLINE BLKV_BF16X2_TYPE
blkv_bf16x2_max(const BLKV_BF16X2_TYPE &src_l,
                const BLKV_BF16X2_TYPE &src_r) {
  return blkv_bf16x2_fmax(src_l, src_r);
}

#endif // BLKV_BF16_OPS_HPP


//===----------------------------------------------------------------------===//
// One-layer inline-asm tileop templates (header-form, no __vec__ kernel).
//
// These map 1:1 to the DavinciOO isa/intrinsic definitions and the Linx v0.56
// block ISA. The interface name IS the tileop name: programmers call TLOAD /
// TSTORE / MGATHER / ... and get the hand-written block assembly directly.
//
// Encoding (all already supported by the LinxV5 backend, no backend change):
//   BSTART.TLSU  op: TLOAD=0 TSTORE=1 MGATHER=4 MSCATTER=5 MGATHER_MASK=6 MSCATTER_MASK=7
//   BSTART.CUBE op: TMATMUL=0, TMATMUL.BIAS=1, TMATMUL.ACC=2,
//                   TMATMULMX=3, TMATMULMX.BIAS=4, TMATMULMX.ACC=5,
//                   TMATMUL*.FIXP=9..14
// All variants below are the NORM (no layout conversion) generic form.
//===----------------------------------------------------------------------===//

// TLOAD: GM -> Local Tile (BSTART.TLSU TLOAD). dst[i,j] = src[r0+i, c0+j].
template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD(tile_shape &dst, gm_shape &src) {
  static_assert(
      tile_type_traits<typename tile_shape::TileDType>::IsValidActiveSize,
      "TLOAD dst logical Tile size must be 512 B..32 KB (TSize=1..7)");
  const size_t valid_col = dst.GetValidCol();
  const size_t valid_row = dst.GetValidRow();
  asm volatile(
    "BSTART.TLSU TLOAD, %c[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOT mask=15, last, ->%[d0]<%Z[TileSize]>\n"
    "B.IOR [%[s0],%[GmStride]], []\n"
    : [d0]"=Tr"(dst.data())
    : [s0]"r"(src.data()),
      [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(tile_shape::Cols),
      [GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

// TLOAD: GM -> Shared Tile (PTO v0.58 reissue). The destination is one
// absolute Core-local Shared register; B.IOS carries the per-PE size and PE
// mask. B.IOR carries only the GM address operands (RegDst is zero).
template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
SharedTile<shp> TLOAD(const gm_shape &src) {
  using shp_dtype = typename shp::TileDType;
  static_assert(PEMask > 0 && PEMask < 16, "PEMask must be 1..15");
  static_assert(
      tile_type_traits<shp_dtype>::IsValidActiveSize,
      "TLOAD Shared dst logical Tile size must be 128 B..8 KB (TSize=1..7)");
  SharedTile<shp> result;
  const size_t valid_col = result.GetValidCol();
  const size_t valid_row = result.GetValidRow();
  asm volatile(
    "BSTART.TLSU TLOAD, %c[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOS mask=%c[PEMask], ->%S[Shared]<%Z[TileSize]>\n"
    "B.IOR [%[s0],%[GmStride]], []\n"
    : [Shared] "=Sr"(result.handle_ref())
    : [s0]"r"(src.data()),
      [PEMask]"i"(PEMask),
      [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [TileSize]"i"(tile_type_traits<shp_dtype>::TilesizeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(shp::Cols),
      [GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
  return result;
}

template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
void TLOAD(SharedTile<shp> &dst, const gm_shape &src) {
  using shp_dtype = typename shp::TileDType;
  static_assert(PEMask > 0 && PEMask < 16, "PEMask must be 1..15");
  static_assert(
      tile_type_traits<shp_dtype>::IsValidActiveSize,
      "TLOAD Shared dst logical Tile size must be 128 B..8 KB (TSize=1..7)");
  const size_t valid_col = dst.GetValidCol();
  const size_t valid_row = dst.GetValidRow();
  asm volatile(
    "BSTART.TLSU TLOAD, %c[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOS mask=%c[PEMask], ->%S[Shared]<%Z[TileSize]>\n"
    "B.IOR [%[s0],%[GmStride]], []\n"
    : [Shared] "=Sr"(dst.handle_ref())
    : [s0]"r"(src.data()),
      [PEMask]"i"(PEMask),
      [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [TileSize]"i"(tile_type_traits<shp_dtype>::TilesizeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(shp::Cols),
      [GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

// TSTORE: Tile -> GM (BSTART.TLSU TSTORE). dst[r0+i, c0+j] = src[i,j].
template <is_global_data_v gm_shape, is_tile_data_v tile_shape>
void TSTORE(gm_shape &dst, tile_shape &src) {
  static_assert(tile_type_traits<typename tile_shape::TileDType>::IsValidActiveSize,
                "TSTORE src logical Tile size must be 512 B..32 KB (TSize=1..7) "
                "per DavinciOO v5 B.IOT encoding");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TLSU TSTORE, %c[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOT %[s0], mask=15, last\n"
    "B.IOR [%[d0],%[GmStride]], []\n"
    :
    : [d0]"r"(dst.data()), [s0]"Tr"(src.data()),
      [SrcType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(tile_shape::Cols),
      [GmStride]"r"(gm_shape::RowStride * sizeof(typename gm_shape::DType))
  );
}

// Low-level v5 GMOV. All four PEs must reach the same dynamic instance;
// PEMask only selects requesters and does not reduce the Core4 collective.
template <int PEMask = 15, is_tile_data_v tile_shape_dst,
          is_tile_data_v tile_shape_src>
void GMOV(tile_shape_dst &dst, uint64_t peer_tid, const tile_shape_src &src) {
  static_assert(PEMask > 0 && PEMask < 16, "GMOV PEMask must be 1..15");
  static_assert(std::is_same_v<typename tile_shape_dst::DType,
                               typename tile_shape_src::DType>,
                "GMOV source and destination dtypes must match");
  static_assert(tile_shape_dst::Rows == tile_shape_src::Rows &&
                    tile_shape_dst::Cols == tile_shape_src::Cols &&
                    (tile_shape_dst::ValidRow == DYNAMIC ||
                     tile_shape_src::ValidRow == DYNAMIC ||
                     tile_shape_dst::ValidRow == tile_shape_src::ValidRow) &&
                    (tile_shape_dst::ValidCol == DYNAMIC ||
                     tile_shape_src::ValidCol == DYNAMIC ||
                     tile_shape_dst::ValidCol == tile_shape_src::ValidCol) &&
                    tile_shape_dst::BFractal == tile_shape_src::BFractal &&
                    tile_shape_dst::SFractal == tile_shape_src::SFractal,
                "GMOV source and destination descriptors must match");
  static_assert(
      tile_type_traits<typename tile_shape_dst::TileDType>::IsValidActiveSize,
      "GMOV logical Tile size must be 512 B..32 KB (TSize=1..7)");
  static_assert(sizeof(typename tile_shape_dst::TileDType) ==
                    sizeof(typename tile_shape_src::TileDType),
                "GMOV source and destination logical sizes must match");
  asm volatile(
      "BSTART.TLSU GMOV, %c[DataType]\n"
      "B.IOT %[src], mask=%c[PEMask], last, ->%[dst]<%Z[TileSize]>\n"
      "B.IOR [%[peer]],[]\n"
      : [dst] "=Tr"(dst.data())
      : [src] "Tr"(src.data()), [peer] "r"(peer_tid),
        [PEMask] "i"(PEMask),
        [DataType] "i"(type_traits<typename tile_shape_src::DType>::TypeCode),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_dst::TileDType>::TilesizeCode)
      : "memory");
}

// Shared TMOV primitives. The compiler allocates the returned SharedTile
// handle to an absolute S#0..S#255 register through the "Sr" constraint.
// These wrappers must inline so the opaque Shared value never crosses the
// ordinary C++ ABI as an integer or memory-resident object.
#define PTO_SHARED_INLINE __attribute__((always_inline)) inline

template <int PEMask = 15, is_tile_data_v tile_shape_src>
PTO_SHARED_INLINE void
TMOV_L2S_INSERT(SharedTile<tile_shape_src> &dst,
                const tile_shape_src &src) {
  static_assert(PEMask > 0 && PEMask < 16, "PEMask must be 1..15");
  static_assert(
      tile_type_traits<typename tile_shape_src::TileDType>::IsValidActiveSize,
      "TMOV.L2S.INSERT logical Tile size must be 512 B..32 KB");
  dst.SetValidShape(src);
  asm volatile(
      "BSTART.TLSU TMOV.L2S.INSERT, %c[DataType]\n"
      "B.IOS mask=%c[PEMask], ->%S[Shared]<%Z[TileSize]>\n"
      "B.IOT %[src], mask=%c[PEMask], last\n"
      : [Shared] "=Sr"(dst.handle_ref())
      : [src] "Tr"(src.data()),
        [PEMask] "i"(PEMask),
        [DataType] "i"(type_traits<typename tile_shape_src::DType>::TypeCode),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_src::TileDType>::TilesizeCode)
      : "memory");
}

template <int PEMask = 15, is_tile_data_v tile_shape_src>
PTO_SHARED_INLINE SharedTile<tile_shape_src>
TMOV_L2S_INSERT(const tile_shape_src &src) {
  SharedTile<tile_shape_src> result(src);
  TMOV_L2S_INSERT<PEMask>(result, src);
  return result;
}

template <int PEMask = 15, is_tile_data_v tile_shape_src>
PTO_SHARED_INLINE void
TMOV_L2S_PUBLISH(SharedTile<tile_shape_src> &dst,
                 const tile_shape_src &src) {
  static_assert(PEMask > 0 && PEMask < 16, "PEMask must be 1..15");
  static_assert(
      tile_type_traits<typename tile_shape_src::TileDType>::IsValidActiveSize,
      "TMOV.L2S.PUBLISH logical Tile size must be 512 B..32 KB");
  dst.SetValidShape(src);
  asm volatile(
      "BSTART.TLSU TMOV.L2S.PUBLISH, %c[DataType]\n"
      "B.IOS mask=%c[PEMask], ->%S[Shared]<%Z[TileSize]>\n"
      "B.IOT %[src], mask=%c[PEMask], last\n"
      : [Shared] "=Sr"(dst.handle_ref())
      : [src] "Tr"(src.data()),
        [PEMask] "i"(PEMask),
        [DataType] "i"(type_traits<typename tile_shape_src::DType>::TypeCode),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_src::TileDType>::TilesizeCode)
      : "memory");
}

template <int PEMask = 15, is_tile_data_v tile_shape_src>
PTO_SHARED_INLINE SharedTile<tile_shape_src>
TMOV_L2S_PUBLISH(const tile_shape_src &src) {
  SharedTile<tile_shape_src> result(src);
  TMOV_L2S_PUBLISH<PEMask>(result, src);
  return result;
}

template <int PEMask = 15, is_tile_data_v tile_shape_dst, typename LocalTile>
PTO_SHARED_INLINE void
TMOV_S2L_BROADCAST(tile_shape_dst &dst,
                   const SharedTile<LocalTile> &shared) {
  static_assert(PEMask > 0 && PEMask < 16, "PEMask must be 1..15");
  static_assert(
      tile_type_traits<typename tile_shape_dst::TileDType>::IsValidActiveSize,
      "TMOV.S2L.BROADCAST logical Tile size must be 512 B..32 KB");
  asm volatile(
      "BSTART.TLSU TMOV.S2L.BROADCAST, %c[DataType]\n"
      "B.IOS %S[Shared], mask=%c[PEMask]\n"
      "B.IOT mask=%c[PEMask], last, ->%[dst]<%Z[TileSize]>\n"
      : [dst] "=Tr"(dst.data())
      : [Shared] "Sr"(shared.handle()), [PEMask] "i"(PEMask),
        [DataType] "i"(type_traits<typename tile_shape_dst::DType>::TypeCode),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_dst::TileDType>::TilesizeCode)
      : "memory");
}

template <int PEMask = 15, is_tile_data_v tile_shape_dst, typename LocalTile>
PTO_SHARED_INLINE void TMOV_S2L_EXTRACT(
    tile_shape_dst &dst, const SharedTile<LocalTile> &shared) {
  static_assert(PEMask > 0 && PEMask < 16, "PEMask must be 1..15");
  static_assert(
      tile_type_traits<typename tile_shape_dst::TileDType>::IsValidActiveSize,
      "TMOV.S2L.EXTRACT logical Tile size must be 512 B..32 KB");
  asm volatile(
      "BSTART.TLSU TMOV.S2L.EXTRACT, %c[DataType]\n"
      "B.IOS %S[Shared], mask=%c[PEMask]\n"
      "B.IOT mask=%c[PEMask], last, ->%[dst]<%Z[TileSize]>\n"
      : [dst] "=Tr"(dst.data())
      : [Shared] "Sr"(shared.handle()), [PEMask] "i"(PEMask),
        [DataType] "i"(type_traits<typename tile_shape_dst::DType>::TypeCode),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_dst::TileDType>::TilesizeCode)
      : "memory");
}

// ACCCVT was removed from DavinciOO v5. FIXP matrix operations write an
// ordinary Tile directly and are the supported replacement.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void ACCCVT(tile_shape_out &, tile_shape_in &) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "ACCCVT was removed from DavinciOO v5 and cannot export the "
                "implicit ACC; use the corresponding TMATMUL*.FIXP variant");
}

namespace pto_matmul_detail {

#define PTO_MATMUL_HEADER(OPCODE, EXTRA_ATTRS)                                  \
  "BSTART.CUBE " OPCODE ", %c[DataTypeA]\n"                                 \
  "B.DATR %c[DataTypeB], byte0, Null\n" EXTRA_ATTRS                         \
  "B.DIM %[M], 0, ->lb0\n"                                                   \
  "B.DIM %[N], 0, ->lb1\n"                                                   \
  "B.DIM %[K], 0, ->lb2\n"

#define PTO_MATMUL_COMMON_INPUTS(DstType, AType, BType, MValue, NValue, KValue) \
  [M] "r"(MValue), [N] "r"(NValue), [K] "r"(KValue),                         \
      [DataTypeA] "i"(type_traits<typename AType::DType>::TypeCode),           \
      [DataTypeB] "i"(type_traits<typename BType::DType>::TypeCode),           \
      [TileSize] "i"(                                                           \
          tile_type_traits<typename DstType::TileDType>::TilesizeCode)

template <typename Matrix>
inline size_t matrix_valid_row(const Matrix &matrix) {
  return matrix.GetValidRow();
}

template <typename Matrix>
inline size_t matrix_valid_col(const Matrix &matrix) {
  return matrix.GetValidCol();
}

// PTO_FIXP_ATTR / PTO_FIXP_ATTR_INPUTS emit the B.FPATR line and its seven
// immediate operands. Every TMATMUL/TMATMULMX CUBE bundle carries exactly one
// B.FPATR after B.DATR, so these are shared by the whole family, not just the
// .FIXP variants. The macros reference the template parameter Attr, so the
// helper templates below take FixpAttr Attr as an NTTP. Defined here (before
// any helper that uses them) so the plain matmul free function can also use
// PTO_FIXP_ATTR.
#define PTO_FIXP_ATTR \
  "B.FPATR %c[PreQuant], %c[ReluMode], %c[GroupNCode], %c[RowMaxEn], " \
  "%c[GroupMaxEn], %c[RowMaxInit], %c[MaxAbsEn]\n"

#define PTO_FIXP_ATTR_INPUTS \
  [PreQuant] "i"(static_cast<uint8_t>(Attr.PreQuant)), \
  [ReluMode] "i"(static_cast<uint8_t>(Attr.Relu)), \
  [GroupNCode] "i"(Attr.GroupNCode), [RowMaxEn] "i"(Attr.RowMaxEn), \
  [GroupMaxEn] "i"(Attr.GroupMaxEn), \
  [RowMaxInit] "i"(Attr.RowMaxInit), [MaxAbsEn] "i"(Attr.MaxAbsEn)

template <typename A, typename B>
constexpr void validate_shared_matrix_pair() {
  static_assert(!is_shared_tile_v<A> || is_shared_tile_v<B>,
                "Shared matmul A requires B to be Shared as well; a lone "
                "B.IOS binder denotes the existing Shared-Right form");
}

template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename B>
PTO_SHARED_INLINE void matmul(Dst &dst, A &a, B &b, size_t M, size_t N,
                              size_t K) {
  validate_shared_matrix_pair<A, B>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOT %[A], %[B], mask=15, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [A] "Tr"(a.data()), [B] "Tr"(b.data()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOS %S[SharedA], mask=1111\n"
        "B.IOT %[B]\n"
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOS %S[SharedB], mask=1111\n"
        "B.IOT %[A]\n"
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOS %S[SharedA], mask=1111\n"
        "B.IOS %S[SharedB], mask=1111\n"
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  }
}

#define PTO_DEFINE_MATMUL_3SRC_HELPER(Name, Opcode)                            \
template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename B,      \
          typename Extra>                                                        \
PTO_SHARED_INLINE void Name(Dst &dst, A &a, B &b, Extra &extra,                 \
                            size_t M, size_t N, size_t K) {                      \
  validate_shared_matrix_pair<A, B>();                                            \
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {                 \
    asm volatile(                                                                \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                                \
        "B.IOT %[A], %[B], mask=15\n"                                         \
        "B.IOT %[Extra]\n"                                                    \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                       \
        : [Dst] "=&Tr"(dst.data())                                             \
        : [A] "Tr"(a.data()), [B] "Tr"(b.data()),                             \
          [Extra] "Tr"(extra.data()),                                          \
          PTO_FIXP_ATTR_INPUTS,                                                 \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                          \
        : "memory");                                                           \
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {           \
    asm volatile(                                                                \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                                \
        "B.IOS %S[SharedA], mask=1111\n"                                               \
        "B.IOT %[B]\n"                                                       \
        "B.IOT %[Extra]\n"                                                    \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                       \
        : [Dst] "=&Tr"(dst.data())                                             \
        : [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()),                     \
          [Extra] "Tr"(extra.data()),                                          \
          PTO_FIXP_ATTR_INPUTS,                                                 \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                          \
        : "memory");                                                           \
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {           \
    asm volatile(                                                                \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                                \
        "B.IOS %S[SharedB], mask=1111\n"                                               \
        "B.IOT %[A]\n"                                                       \
        "B.IOT %[Extra]\n"                                                    \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                       \
        : [Dst] "=&Tr"(dst.data())                                             \
        : [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle()),                     \
          [Extra] "Tr"(extra.data()),                                          \
          PTO_FIXP_ATTR_INPUTS,                                                 \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                          \
        : "memory");                                                           \
  } else {                                                                       \
    asm volatile(                                                                \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                                \
        "B.IOS %S[SharedA], mask=1111\n"                                               \
        "B.IOS %S[SharedB], mask=1111\n"                                               \
        "B.IOT %[Extra]\n"                                                    \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                       \
        : [Dst] "=&Tr"(dst.data())                                             \
        : [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()),             \
          [Extra] "Tr"(extra.data()),                                          \
          PTO_FIXP_ATTR_INPUTS,                                                 \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                          \
        : "memory");                                                           \
  }                                                                              \
}

PTO_DEFINE_MATMUL_3SRC_HELPER(matmul_acc, "TMATMUL.ACC")
PTO_DEFINE_MATMUL_3SRC_HELPER(matmul_bias, "TMATMUL.BIAS")
PTO_DEFINE_MATMUL_3SRC_HELPER(matmul_acc_fixp, "TMATMUL.ACC.FIXP")
PTO_DEFINE_MATMUL_3SRC_HELPER(matmul_bias_fixp, "TMATMUL.BIAS.FIXP")


#define PTO_FIXP_SRC_0 \
  "B.IOT %[A], %[B], mask=15\n"
#define PTO_FIXP_SRC_1 \
  "B.IOT %[A], %[B], mask=15\n" "B.IOT %[RowIn]\n"
#define PTO_FIXP_SRC_2 \
  "B.IOT %[A], %[B], mask=15\n" "B.IOT %[QuantTile]\n"
#define PTO_FIXP_SRC_3 \
  "B.IOT %[A], %[B], mask=15\n" "B.IOT %[RowIn], %[QuantTile]\n"
#define PTO_FIXP_SRC_4 \
  "B.IOT %[A], %[B], mask=15\n" "B.IOT %[ReluTile]\n"
#define PTO_FIXP_SRC_5 \
  "B.IOT %[A], %[B], mask=15\n" "B.IOT %[RowIn], %[ReluTile]\n"
#define PTO_FIXP_SRC_6 \
  "B.IOT %[A], %[B], mask=15\n" "B.IOT %[QuantTile], %[ReluTile]\n"
#define PTO_FIXP_SRC_7 \
  "B.IOT %[A], %[B], mask=15\n" \
  "B.IOT %[RowIn], %[QuantTile]\n" \
  "B.IOT %[ReluTile]\n"

#define PTO_FIXP_SHARED_B_SRC_0 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n"
#define PTO_FIXP_SHARED_B_SRC_1 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[RowIn]\n"
#define PTO_FIXP_SHARED_B_SRC_2 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[QuantTile]\n"
#define PTO_FIXP_SHARED_B_SRC_3 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[RowIn], %[QuantTile]\n"
#define PTO_FIXP_SHARED_B_SRC_4 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[ReluTile]\n"
#define PTO_FIXP_SHARED_B_SRC_5 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[RowIn], %[ReluTile]\n"
#define PTO_FIXP_SHARED_B_SRC_6 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[QuantTile], %[ReluTile]\n"
#define PTO_FIXP_SHARED_B_SRC_7 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  "B.IOT %[RowIn], %[QuantTile]\n" "B.IOT %[ReluTile]\n"

#define PTO_FIXP_SHARED_A_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n"
#define PTO_FIXP_SHARED_A_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn]\n"
#define PTO_FIXP_SHARED_A_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[QuantTile]\n"
#define PTO_FIXP_SHARED_A_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn]\n" \
  "B.IOT %[QuantTile]\n"
#define PTO_FIXP_SHARED_A_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[ReluTile]\n"
#define PTO_FIXP_SHARED_A_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn]\n" \
  "B.IOT %[ReluTile]\n"
#define PTO_FIXP_SHARED_A_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[QuantTile]\n" \
  "B.IOT %[ReluTile]\n"
#define PTO_FIXP_SHARED_A_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn]\n" \
  "B.IOT %[QuantTile], %[ReluTile]\n"

#define PTO_FIXP_SHARED_AB_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_1 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn]\n"
#define PTO_FIXP_SHARED_AB_SRC_2 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[QuantTile]\n"
#define PTO_FIXP_SHARED_AB_SRC_3 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], %[QuantTile]\n"
#define PTO_FIXP_SHARED_AB_SRC_4 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[ReluTile]\n"
#define PTO_FIXP_SHARED_AB_SRC_5 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], %[ReluTile]\n"
#define PTO_FIXP_SHARED_AB_SRC_6 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[QuantTile], %[ReluTile]\n"
#define PTO_FIXP_SHARED_AB_SRC_7 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], %[QuantTile]\n" \
  "B.IOT %[ReluTile]\n"

#define PTO_FIXP_IOR_0 ""
#define PTO_FIXP_IOR_1 "B.IOR [%[QuantGpr]],[]\n"
#define PTO_FIXP_IOR_2 "B.IOR [zero,%[LReluGpr]],[]\n"
#define PTO_FIXP_IOR_3 "B.IOR [%[QuantGpr],%[LReluGpr]],[]\n"

#define PTO_FIXP_OUT_0 \
  "B.IOT mask=15, last, ->%[Dst]<%Z[DstSize]>\n"
#define PTO_FIXP_OUT_1 \
  "B.IOT mask=15, ->%[Dst]<%Z[DstSize]>\n" \
  "B.IOT mask=15, last, ->%[RowOut]<%Z[RowSize]>\n"
#define PTO_FIXP_OUT_2 \
  "B.IOT mask=15, ->%[Dst]<%Z[DstSize]>\n" \
  "B.IOT mask=15, last, ->%[GroupOut]<%Z[GroupSize]>\n"
#define PTO_FIXP_OUT_3 \
  "B.IOT mask=15, ->%[Dst]<%Z[DstSize]>\n" \
  "B.IOT mask=15, ->%[RowOut]<%Z[RowSize]>\n" \
  "B.IOT mask=15, last, ->%[GroupOut]<%Z[GroupSize]>\n"

#define PTO_FIXP_OUT_DECL_0 [Dst] "=&Tr"(dst.data())
#define PTO_FIXP_OUT_DECL_1 \
  [Dst] "=&Tr"(dst.data()), [RowOut] "=&Tr"(row_out.data())
#define PTO_FIXP_OUT_DECL_2 \
  [Dst] "=&Tr"(dst.data()), [GroupOut] "=&Tr"(group_out.data())
#define PTO_FIXP_OUT_DECL_3 \
  [Dst] "=&Tr"(dst.data()), [RowOut] "=&Tr"(row_out.data()), \
  [GroupOut] "=&Tr"(group_out.data())

#define PTO_FIXP_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                               \
      PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)                    \
      PTO_FIXP_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR                                \
      : PTO_FIXP_OUT_DECL_##OUT                                              \
      : [A] "Tr"(a.data()), [B] "Tr"(b.data()),                            \
        [RowIn] "Tr"(row_in.data()), [QuantTile] "Tr"(quant_tile.data()),  \
        [ReluTile] "Tr"(relu_tile.data()),                                  \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),              \
        PTO_FIXP_ATTR_INPUTS,                                                 \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                        \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_EMIT_SHARED_B(SRC, OUT, IOR) \
  asm volatile(                                                               \
      PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)                    \
      PTO_FIXP_SHARED_B_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR       \
      : PTO_FIXP_OUT_DECL_##OUT                                              \
      : [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle()),                   \
        [RowIn] "Tr"(row_in.data()), [QuantTile] "Tr"(quant_tile.data()),  \
        [ReluTile] "Tr"(relu_tile.data()),                                  \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),              \
        PTO_FIXP_ATTR_INPUTS,                                                 \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                        \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_EMIT_SHARED_A(SRC, OUT, IOR) \
  asm volatile(                                                               \
      PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)                      \
      PTO_FIXP_SHARED_A_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR       \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()),                    \
        [RowIn] "Tr"(row_in.data()), [QuantTile] "Tr"(quant_tile.data()),   \
        [ReluTile] "Tr"(relu_tile.data()),                                   \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),               \
        PTO_FIXP_ATTR_INPUTS,                                                  \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                         \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_EMIT_SHARED_AB(SRC, OUT, IOR) \
  asm volatile(                                                               \
      PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)                      \
      PTO_FIXP_SHARED_AB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR      \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()),            \
        [RowIn] "Tr"(row_in.data()), [QuantTile] "Tr"(quant_tile.data()),   \
        [ReluTile] "Tr"(relu_tile.data()),                                   \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),               \
        PTO_FIXP_ATTR_INPUTS,                                                  \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                         \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_DISPATCH_OUT(EMIT, SRC, IOR)                                 \
  if constexpr (OutMask == 0) { EMIT(SRC, 0, IOR); }                          \
  else if constexpr (OutMask == 1) { EMIT(SRC, 1, IOR); }                     \
  else if constexpr (OutMask == 2) { EMIT(SRC, 2, IOR); }                     \
  else { EMIT(SRC, 3, IOR); }

#define PTO_FIXP_DISPATCH_SRC(EMIT, IOR)                                      \
  if constexpr (SrcMask == 0) { PTO_FIXP_DISPATCH_OUT(EMIT, 0, IOR); }        \
  else if constexpr (SrcMask == 1) { PTO_FIXP_DISPATCH_OUT(EMIT, 1, IOR); }   \
  else if constexpr (SrcMask == 2) { PTO_FIXP_DISPATCH_OUT(EMIT, 2, IOR); }  \
  else if constexpr (SrcMask == 3) { PTO_FIXP_DISPATCH_OUT(EMIT, 3, IOR); }  \
  else if constexpr (SrcMask == 4) { PTO_FIXP_DISPATCH_OUT(EMIT, 4, IOR); }  \
  else if constexpr (SrcMask == 5) { PTO_FIXP_DISPATCH_OUT(EMIT, 5, IOR); }  \
  else if constexpr (SrcMask == 6) { PTO_FIXP_DISPATCH_OUT(EMIT, 6, IOR); }  \
  else { PTO_FIXP_DISPATCH_OUT(EMIT, 7, IOR); }

#define PTO_FIXP_DISPATCH(EMIT)                                               \
  if constexpr (IorMode == 0) { PTO_FIXP_DISPATCH_SRC(EMIT, 0); }              \
  else if constexpr (IorMode == 1) { PTO_FIXP_DISPATCH_SRC(EMIT, 1); }         \
  else if constexpr (IorMode == 2) { PTO_FIXP_DISPATCH_SRC(EMIT, 2); }        \
  else { PTO_FIXP_DISPATCH_SRC(EMIT, 3); }

template <FixpAttr Attr, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename A, typename B, typename RowIn,
          typename QuantTile,
          typename ReluTile, typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_fixp(
    Dst &dst, A &a, B &b, RowIn &row_in, QuantTile &quant_tile,
    ReluTile &relu_tile, RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  validate_shared_matrix_pair<A, B>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_EMIT_LOCAL);
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_EMIT_SHARED_A);
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_EMIT_SHARED_B);
  } else {
    PTO_FIXP_DISPATCH(PTO_FIXP_EMIT_SHARED_AB);
  }
}

#undef PTO_FIXP_DISPATCH
#undef PTO_FIXP_DISPATCH_SRC
#undef PTO_FIXP_DISPATCH_OUT
#undef PTO_FIXP_EMIT_SHARED_AB
#undef PTO_FIXP_EMIT_SHARED_A
#undef PTO_FIXP_EMIT_SHARED_B
#undef PTO_FIXP_EMIT_LOCAL
#undef PTO_FIXP_OUT_DECL_3
#undef PTO_FIXP_OUT_DECL_2
#undef PTO_FIXP_OUT_DECL_1
#undef PTO_FIXP_OUT_DECL_0
#undef PTO_FIXP_OUT_3
#undef PTO_FIXP_OUT_2
#undef PTO_FIXP_OUT_1
#undef PTO_FIXP_OUT_0
#undef PTO_FIXP_IOR_3
#undef PTO_FIXP_IOR_2
#undef PTO_FIXP_IOR_1
#undef PTO_FIXP_IOR_0
#undef PTO_FIXP_SHARED_AB_SRC_7
#undef PTO_FIXP_SHARED_AB_SRC_6
#undef PTO_FIXP_SHARED_AB_SRC_5
#undef PTO_FIXP_SHARED_AB_SRC_4
#undef PTO_FIXP_SHARED_AB_SRC_3
#undef PTO_FIXP_SHARED_AB_SRC_2
#undef PTO_FIXP_SHARED_AB_SRC_1
#undef PTO_FIXP_SHARED_AB_SRC_0
#undef PTO_FIXP_SHARED_A_SRC_7
#undef PTO_FIXP_SHARED_A_SRC_6
#undef PTO_FIXP_SHARED_A_SRC_5
#undef PTO_FIXP_SHARED_A_SRC_4
#undef PTO_FIXP_SHARED_A_SRC_3
#undef PTO_FIXP_SHARED_A_SRC_2
#undef PTO_FIXP_SHARED_A_SRC_1
#undef PTO_FIXP_SHARED_A_SRC_0
#undef PTO_FIXP_SHARED_B_SRC_7
#undef PTO_FIXP_SHARED_B_SRC_6
#undef PTO_FIXP_SHARED_B_SRC_5
#undef PTO_FIXP_SHARED_B_SRC_4
#undef PTO_FIXP_SHARED_B_SRC_3
#undef PTO_FIXP_SHARED_B_SRC_2
#undef PTO_FIXP_SHARED_B_SRC_1
#undef PTO_FIXP_SHARED_B_SRC_0
#undef PTO_FIXP_SRC_7
#undef PTO_FIXP_SRC_6
#undef PTO_FIXP_SRC_5
#undef PTO_FIXP_SRC_4
#undef PTO_FIXP_SRC_3
#undef PTO_FIXP_SRC_2
#undef PTO_FIXP_SRC_1
#undef PTO_FIXP_SRC_0

#define PTO_DEFINE_MATMUL_MX_HELPER(Name, Opcode)                             \
template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename ScaleA,  \
          typename B, typename ScaleB>                                           \
PTO_SHARED_INLINE void Name(Dst &dst, A &a, ScaleA &scale_a, B &b,             \
                            ScaleB &scale_b, size_t M, size_t N, size_t K) {     \
  validate_shared_matrix_pair<A, B>();                                            \
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {                \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOT %[A], %[ScaleA], mask=15\n"                                    \
        "B.IOT %[B], %[ScaleB], mask=15, last, ->%[Dst]<%Z[TileSize]>\n"     \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[B], mask=15\n"                                   \
        "B.IOT %[ScaleB]\n"                                                  \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[A], %[ScaleA], mask=15\n"                                   \
        "B.IOT %[ScaleB]\n"                                                  \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else {                                                                      \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[ScaleB], mask=15\n"                              \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  }                                                                             \
}

PTO_DEFINE_MATMUL_MX_HELPER(matmul_mx, "TMATMULMX")
PTO_DEFINE_MATMUL_MX_HELPER(matmul_mx_fixp, "TMATMULMX.FIXP")

#define PTO_DEFINE_MATMUL_MX_5SRC_HELPER(Name, Opcode)                          \
template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename ScaleA,  \
          typename B, typename ScaleB, typename Extra>                           \
PTO_SHARED_INLINE void Name(Dst &dst, A &a, ScaleA &scale_a, B &b,             \
                            ScaleB &scale_b, Extra &extra, size_t M, size_t N,   \
                            size_t K) {                                          \
  validate_shared_matrix_pair<A, B>();                                            \
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {                \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOT %[A], %[ScaleA], mask=15\n"                                    \
        "B.IOT %[B], %[ScaleB], mask=15\n"                                   \
        "B.IOT %[Extra]\n"                                                   \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[B], mask=15\n"                                   \
        "B.IOT %[ScaleB], %[Extra], mask=15\n"                               \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[A], %[ScaleA], mask=15\n"                                   \
        "B.IOT %[ScaleB], %[Extra], mask=15\n"                               \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else {                                                                      \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[ScaleB], mask=15\n"                              \
        "B.IOT %[Extra]\n"                                                   \
        "B.IOT mask=15, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  }                                                                             \
}

PTO_DEFINE_MATMUL_MX_5SRC_HELPER(matmul_mx_bias, "TMATMULMX.BIAS")
PTO_DEFINE_MATMUL_MX_5SRC_HELPER(matmul_mx_acc, "TMATMULMX.ACC")
PTO_DEFINE_MATMUL_MX_5SRC_HELPER(matmul_mx_bias_fixp, "TMATMULMX.BIAS.FIXP")
PTO_DEFINE_MATMUL_MX_5SRC_HELPER(matmul_mx_acc_fixp, "TMATMULMX.ACC.FIXP")

#undef PTO_DEFINE_MATMUL_MX_5SRC_HELPER
#undef PTO_DEFINE_MATMUL_MX_HELPER
#undef PTO_DEFINE_MATMUL_3SRC_HELPER
#undef PTO_MATMUL_COMMON_INPUTS
#undef PTO_MATMUL_HEADER

} // namespace pto_matmul_detail

// TMATMUL: C = A(M,K) * B(K,N). Supported storage combinations are
// Local/Local, Local/Shared-Right, and Shared-Left/Shared-Right. A lone Shared
// Left is intentionally rejected because the single-binder stream is reserved
// for the existing Shared-Right ABI.
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b>
PTO_SHARED_INLINE void TMATMUL(tile_shape_c &c, tile_shape_a &a,
                              tile_shape_b &b) {
  static_assert(tile_role_v<tile_shape_a> == Location::Left,
                "TMATMUL input A must be a Left tile");
  static_assert(tile_role_v<tile_shape_b> == Location::Right,
                "TMATMUL input B must be a Right tile");
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul<Attr>(c, a, b, M, N, K);
}

template <is_tile_data_v tile_shape_c, is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL(tile_shape_c &c, tile_shape_a &a,
                              tile_shape_b &b, const Options &options);

// TMATMUL_ACC: D = C + A*B. D and C are distinct ordinary Tile operands.
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d,
          is_tile_data_v tile_shape_c, is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b>
PTO_SHARED_INLINE void TMATMUL_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                 tile_shape_b &b) {
  static_assert(tile_role_v<tile_shape_a> == Location::Left &&
                    tile_role_v<tile_shape_b> == Location::Right,
                "TMATMUL_ACC requires A=Left and B=Right");
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_acc<Attr>(d, a, b, c, M, N, K);
}

template <is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                 tile_shape_b &b, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_a> == Location::Left &&
                    tile_role_v<tile_shape_b> == Location::Right,
                "TMATMUL_ACC requires A=Left and B=Right");
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_acc<Attr>(d, a, b, c, M, N, K);
}


namespace pto_matmul_detail {

template <bool Use, typename Pointer, typename Dummy>
decltype(auto) select_fixp_operand(Pointer *PointerValue, Dummy &DummyValue) {
  if constexpr (Use)
    return *PointerValue;
  else
    return (DummyValue);
}

} // namespace pto_matmul_detail

// Unified TMATMUL_FIXP interface. All FIXP attributes and auxiliary operands
// are carried by one compile-time-shaped options object; scalar descriptors
// remain runtime GPR values and tile operands remain runtime tile registers.
template <is_tile_data_v tile_shape_d, is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b, fixp::is_options_v Options>
__attribute__((always_inline)) inline void
TMATMUL_FIXP(tile_shape_d &d, tile_shape_a &a,
                                    tile_shape_b &b, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_valid_fixp_attr(Attr),
                "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_d::DType>(),
                "TMATMUL_FIXP destination dtype does not match PreQuantMode");
  static_assert(tile_role_v<tile_shape_a> == Location::Left,
                "TMATMUL_FIXP input A must be Location::Left");
  static_assert(tile_role_v<tile_shape_b> == Location::Right,
                "TMATMUL_FIXP input B must be a Right tile");
  static_assert(tile_shape_a::Cols == tile_shape_b::Rows,
                "TMATMUL_FIXP requires A.Cols == B.Rows");
  static_assert(tile_shape_d::Rows == tile_shape_a::Rows &&
                    tile_shape_d::Cols == tile_shape_b::Cols,
                "TMATMUL_FIXP output shape must be M x N");
  static_assert(tile_type_traits<typename tile_shape_d::TileDType>::IsValidActiveSize,
                "TMATMUL_FIXP output logical Tile size must be 512 B..32 KB");

  constexpr bool HasVectorQuant =
      is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant =
      is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) |
                          (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) |
                          (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant ==
                    !std::is_same_v<typename Options::QuantTile,
                                    fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(!HasVectorQuant ==
                    std::is_same_v<typename Options::QuantTile,
                                    fixp::NoOperand>,
                "quant parameter Tile is only valid for vector PreQuant");
  static_assert(HasPRelu ==
                    !std::is_same_v<typename Options::ReluTile,
                                    fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(!HasPRelu ==
                    std::is_same_v<typename Options::ReluTile,
                                    fixp::NoOperand>,
                "PReLU parameter Tile is only valid for PRelu mode");
  static_assert(HasRowIn ==
                    !std::is_same_v<typename Options::RowMaxIn,
                                    fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(!HasRowIn ==
                    std::is_same_v<typename Options::RowMaxIn,
                                    fixp::NoOperand>,
                "RowMaxIn is only valid for RowMaxInit");
  static_assert(HasRowOut ==
                    !std::is_same_v<typename Options::RowMaxOut,
                                    fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut ==
                    !std::is_same_v<typename Options::GroupMaxOut,
                                    fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  if constexpr (HasVectorQuant) {
    using QuantTile = typename Options::QuantTile;
    static_assert(
        tile_type_traits<typename QuantTile::TileDType>::IsValidActiveSize,
        "TMATMUL_FIXP quant parameter Tile must occupy 512 B..32 KB; pad the "
        "physical Tile and keep ValidRow=1, ValidCol=N when necessary");
    static_assert(QuantTile::ValidRow == -1 || QuantTile::ValidRow == 1,
                  "TMATMUL_FIXP vector quant parameter must have ValidRow=1");
    static_assert(QuantTile::ValidCol == -1 || tile_shape_b::ValidCol == -1 ||
                      QuantTile::ValidCol == tile_shape_b::ValidCol,
                  "TMATMUL_FIXP vector quant parameter must have ValidCol=N");
  }
  if constexpr (HasPRelu) {
    using ReluTile = typename Options::ReluTile;
    static_assert(
        tile_type_traits<typename ReluTile::TileDType>::IsValidActiveSize,
        "TMATMUL_FIXP PReLU parameter Tile must occupy 512 B..32 KB; pad the "
        "physical Tile and keep ValidRow=1, ValidCol=N when necessary");
    static_assert(ReluTile::ValidRow == -1 || ReluTile::ValidRow == 1,
                  "TMATMUL_FIXP PReLU parameter must have ValidRow=1");
    static_assert(ReluTile::ValidCol == -1 || tile_shape_b::ValidCol == -1 ||
                      ReluTile::ValidCol == tile_shape_b::ValidCol,
                  "TMATMUL_FIXP PReLU parameter must have ValidCol=N");
  }
  if constexpr (HasRowOut) {
    using RowOut = typename Options::RowMaxOut;
    static_assert(RowOut::ValidRow == -1 || tile_shape_a::ValidRow == -1 ||
                      RowOut::ValidRow == tile_shape_a::ValidRow,
                  "TMATMUL_FIXP RowMaxOut must have ValidRow=M");
    static_assert(RowOut::ValidCol == -1 || RowOut::ValidCol == 1,
                  "TMATMUL_FIXP RowMaxOut must have ValidCol=1");
    static_assert(type_traits<typename RowOut::DType>::TypeCode == __type_fp32 ||
                      type_traits<typename RowOut::DType>::TypeCode ==
                          __type_int32,
                  "TMATMUL_FIXP RowMaxOut dtype must be FP32 or S32 AccType");
    static_assert(
        tile_type_traits<typename RowOut::TileDType>::IsValidActiveSize,
        "TMATMUL_FIXP RowMaxOut physical Tile must occupy 512 B..32 KB");
  }
  if constexpr (HasRowIn) {
    using RowIn = typename Options::RowMaxIn;
    using RowOut = typename Options::RowMaxOut;
    static_assert((RowIn::ValidRow == DYNAMIC ||
                       RowOut::ValidRow == DYNAMIC ||
                       RowIn::ValidRow == RowOut::ValidRow) &&
                      (RowIn::ValidCol == DYNAMIC ||
                       RowOut::ValidCol == DYNAMIC ||
                       RowIn::ValidCol == RowOut::ValidCol),
                  "TMATMUL_FIXP RowMaxIn/RowMaxOut valid shapes must match");
    static_assert(std::is_same_v<typename RowIn::DType,
                                 typename RowOut::DType>,
                  "TMATMUL_FIXP RowMaxIn/RowMaxOut dtypes must match");
    static_assert(
        tile_type_traits<typename RowIn::TileDType>::IsValidActiveSize,
        "TMATMUL_FIXP RowMaxIn physical Tile must occupy 512 B..32 KB");
  }
  if constexpr (HasGroupOut) {
    using GroupOut = typename Options::GroupMaxOut;
    constexpr int GroupN = fixp::group_n_from_code(Attr.GroupNCode);
    constexpr int ExpectedCols =
        tile_shape_b::ValidCol == -1
            ? -1
            : (tile_shape_b::ValidCol + GroupN - 1) / GroupN;
    static_assert(GroupOut::ValidRow == -1 || tile_shape_a::ValidRow == -1 ||
                      GroupOut::ValidRow == tile_shape_a::ValidRow,
                  "TMATMUL_FIXP GroupMaxOut must have ValidRow=M");
    static_assert(GroupOut::ValidCol == -1 || ExpectedCols == -1 ||
                      GroupOut::ValidCol == ExpectedCols,
                  "TMATMUL_FIXP GroupMaxOut must have ValidCol=ceil(N/GroupN)");
    static_assert(
        type_traits<typename GroupOut::DType>::TypeCode == __type_fp32 ||
            type_traits<typename GroupOut::DType>::TypeCode == __type_int32,
        "TMATMUL_FIXP GroupMaxOut dtype must be FP32 or S32 AccType");
    static_assert(
        tile_type_traits<typename GroupOut::TileDType>::IsValidActiveSize,
        "TMATMUL_FIXP GroupMaxOut physical Tile must occupy 512 B..32 KB");
  }

  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(
      options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(
      options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(
      options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(
      options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(
      options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_fixp<Attr, SrcMask, OutMask, IorMode>(
      d, a, b, row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}

// TMATMUL(D, A, B, options): the active Function 0 operation carries the
// full PostProcess capability (quant/PReLU/RowMax/GroupMax) via options. It
// forwards to the shared emit_fixp lowering (formerly the TMATMUL_FIXP
// implementation), emitting BSTART.CUBE TMATMUL + one B.FPATR + the B.IOT
// source/aux stream dictated by Config. No .FIXP mnemonic is produced.
template <is_tile_data_v tile_shape_c, is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL(tile_shape_c &c, tile_shape_a &a,
                              tile_shape_b &b, const Options &options) {
  TMATMUL_FIXP(c, a, b, options);
}


// TMATMUL_FIXP: D = FIXP(A*B), parameter-free local mode. Attr may select
// keeping the accumulator type, FP16/BF16 conversion, and optional plain ReLU.
// Modes requiring scalar/vector parameters or extra max outputs use dedicated
// overloads rather than silently emitting an incomplete operand stream.
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d,
          is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b>
PTO_SHARED_INLINE void TMATMUL_FIXP(tile_shape_d &d, tile_shape_a &a, tile_shape_b &b) {
  static_assert(is_valid_fixp_attr(Attr), "invalid B.FPATR configuration");
  static_assert(
      is_basic_fixp_attr(Attr),
      "this TMATMUL_FIXP overload supports only parameter-free conversion "
      "and ReLU; quant parameters, PReLU, RowMax and GroupMax require a "
      "dedicated overload");
  static_assert(
      is_basic_fixp_output_type<Attr, typename tile_shape_d::DType>(),
      "TMATMUL_FIXP destination dtype does not match B.FPATR PreQuant mode");
  static_assert(tile_role_v<tile_shape_a> == Location::Left,
                "TMATMUL_FIXP input A must be Location::Left");
  static_assert(tile_role_v<tile_shape_b> == Location::Right,
                "TMATMUL_FIXP input B must be Location::Right");
  static_assert(tile_shape_a::Cols == tile_shape_b::Rows,
                "TMATMUL_FIXP requires A.Cols == B.Rows");
  static_assert(tile_shape_d::Rows == tile_shape_a::Rows &&
                    tile_shape_d::Cols == tile_shape_b::Cols,
                "TMATMUL_FIXP output shape must be M x N");
  static_assert(
      tile_type_traits<typename tile_shape_d::TileDType>::IsValidActiveSize,
      "TMATMUL_FIXP output logical Tile size must be 512 B..32 KB");

  fixp::Options<Attr> options;
  TMATMUL_FIXP(d, a, b, options);
}

// TMATMUL_BIAS: C = A*B + bias (BSTART.CUBE TMATMUL.BIAS).
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_bias>
PTO_SHARED_INLINE void TMATMUL_BIAS(tile_shape_c &c, tile_shape_a &a, tile_shape_b &b,
                  tile_shape_bias &bias) {
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_BIAS supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_bias<Attr>(c, a, b, bias, M, N, K);
}

template <is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_bias, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_BIAS(tile_shape_c &c, tile_shape_a &a, tile_shape_b &b,
                  tile_shape_bias &bias, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_BIAS supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_bias<Attr>(c, a, b, bias, M, N, K);
}

// TMATMUL_MX: C = (A * aScale) * (B * bScale) (BSTART.CUBE TMATMULMX).
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_tile_data_v tile_shape_ascale,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_bscale>
PTO_SHARED_INLINE void TMATMUL_MX(tile_shape_c &c, tile_shape_a &a, tile_shape_ascale &ascale,
                tile_shape_b &b, tile_shape_bscale &bscale) {
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_mx<Attr>(c, a, ascale, b, bscale, M, N, K);
}

template <is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_tile_data_v tile_shape_ascale,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_bscale, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX(tile_shape_c &c, tile_shape_a &a, tile_shape_ascale &ascale,
                tile_shape_b &b, tile_shape_bscale &bscale,
                const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_mx<Attr>(c, a, ascale, b, bscale, M, N, K);
}

template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d,
          is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a, is_tile_data_v tile_shape_sa,
          is_local_or_shared_right tile_shape_b, is_tile_data_v tile_shape_sb>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                    tile_shape_sa &scale_a, tile_shape_b &b,
                    tile_shape_sb &scale_b) {
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_mx_acc<Attr>(d, a, scale_a, b, scale_b, c, M, N, K);
}

template <is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a, is_tile_data_v tile_shape_sa,
          is_local_or_shared_right tile_shape_b, is_tile_data_v tile_shape_sb,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                    tile_shape_sa &scale_a, tile_shape_b &b,
                    tile_shape_sb &scale_b, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_mx_acc<Attr>(d, a, scale_a, b, scale_b, c, M, N, K);
}

template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d,
          is_local_or_shared_left tile_shape_a,
          is_tile_data_v tile_shape_sa,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_sb, is_tile_data_v tile_shape_bias>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(tile_shape_d &d, tile_shape_a &a,
                     tile_shape_sa &scale_a, tile_shape_b &b,
                     tile_shape_sb &scale_b, tile_shape_bias &bias) {
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX_BIAS supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_mx_bias<Attr>(d, a, scale_a, b, scale_b, bias,
                                          M, N, K);
}

template <is_tile_data_v tile_shape_d,
          is_local_or_shared_left tile_shape_a,
          is_tile_data_v tile_shape_sa,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_sb, is_tile_data_v tile_shape_bias,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(tile_shape_d &d, tile_shape_a &a,
                     tile_shape_sa &scale_a, tile_shape_b &b,
                     tile_shape_sb &scale_b, tile_shape_bias &bias,
                     const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX_BIAS supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require a .FIXP variant");
  size_t M = pto_matmul_detail::matrix_valid_row(a);
  size_t N = pto_matmul_detail::matrix_valid_col(b);
  size_t K = pto_matmul_detail::matrix_valid_col(a);
  pto_matmul_detail::matmul_mx_bias<Attr>(d, a, scale_a, b, scale_b, bias,
                                          M, N, K);
}

#undef PTO_SHARED_INLINE


//===--- TEPL Mode 0: tile-tile elementwise ops (BSTART.TEPL) ---===//
// opcode = Mode(0) * 32 + Function. One-layer inline-asm, no __vec__ kernel.

// TADD: dst = src0 + src1
template <is_tile_data_v tile_shape>
void TADD(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 0, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TSUB: dst = src0 - src1
template <is_tile_data_v tile_shape>
void TSUB(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 1, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TMUL: dst = src0 * src1
template <is_tile_data_v tile_shape>
void TMUL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  const size_t valid_col = src0.GetValidCol();
  const size_t valid_row = src0.GetValidRow();
  asm volatile(
    "BSTART.TEPL 2, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(valid_col),
      "r"(valid_row),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TDIV: dst = src0 / src1
template <is_tile_data_v tile_shape>
void TDIV(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 3, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TREM: dst = rem(src0, src1)
template <is_tile_data_v tile_shape>
void TREM(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 4, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TFMOD: dst = fmod(src0, src1)
template <is_tile_data_v tile_shape>
void TFMOD(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 5, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TAND: dst = src0 & src1
template <is_tile_data_v tile_shape>
void TAND(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 6, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TOR: dst = src0 | src1
template <is_tile_data_v tile_shape>
void TOR(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 7, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TXOR: dst = src0 ^ src1
template <is_tile_data_v tile_shape>
void TXOR(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 8, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TSHL: dst = src0 << src1
template <is_tile_data_v tile_shape>
void TSHL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 9, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TSHR: dst = src0 >> src1
template <is_tile_data_v tile_shape>
void TSHR(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 10, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TMAX: dst = max(src0, src1)
template <is_tile_data_v tile_shape>
void TMAX(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 11, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TMIN: dst = min(src0, src1)
template <is_tile_data_v tile_shape>
void TMIN(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 12, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TCMP: compare src0 and src1, write packed predicate
template <is_tile_data_v tile_shape>
void TCMP(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 13, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TPRELU: parametric ReLU with per-element slope
template <is_tile_data_v tile_shape>
void TPRELU(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 14, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TSEL: select between two tiles using mask
template <is_tile_data_v tile_shape>
void TSEL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 26, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TABS: dst = |src|
template <is_tile_data_v tile_shape>
void TABS(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 15, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TNOT: dst = ~src
template <is_tile_data_v tile_shape>
void TNOT(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 16, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TNEG: dst = -src
template <is_tile_data_v tile_shape>
void TNEG(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 17, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TEXP: dst = exp(src)
template <is_tile_data_v tile_shape>
void TEXP(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 18, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TLOG: dst = log(src)
template <is_tile_data_v tile_shape>
void TLOG(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 19, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TRECIP: dst = 1/src
template <is_tile_data_v tile_shape>
void TRECIP(tile_shape &dst, tile_shape &src) {
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TEPL 20, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(valid_col),
      "r"(valid_row),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TSQRT: dst = sqrt(src)
template <is_tile_data_v tile_shape>
void TSQRT(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 21, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TRSQRT: dst = 1/sqrt(src)
template <is_tile_data_v tile_shape>
void TRSQRT(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 22, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TRELU: dst = max(src, 0)
template <is_tile_data_v tile_shape>
void TRELU(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 23, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TADDC: dst = src0 + src1 + src2
template <is_tile_data_v tile_shape>
void TADDC(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2) {
  asm volatile(
    "BSTART.TEPL 24, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15\n"
    "B.IOT %7, mask=15, last, ->%0<%Z8>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "Tr"(src2.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TSUBC: dst = src0 - src1 + src2
template <is_tile_data_v tile_shape>
void TSUBC(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2) {
  asm volatile(
    "BSTART.TEPL 25, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15\n"
    "B.IOT %7, mask=15, last, ->%0<%Z8>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "Tr"(src2.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TCVT: elementwise type conversion (opcode 27, already has TCVT_T)
// Use TCVT_T(dst, src) for this; TCVT is aliased below for convenience.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCVT(tile_shape_out &dst, tile_shape_in &src) {
  TCVT_T(dst, src);
}
//===--- TEPL Mode 1: tile-scalar elementwise ops (BSTART.TEPL) ---===//
// opcode = Mode(1) * 32 + Function = 32 + Function.

// TADDS: dst = src + scalar
template <is_tile_data_v tile_shape>
void TADDS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TEPL 32, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(valid_col),
      "r"(valid_row),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TSUBS: dst = src - scalar
template <is_tile_data_v tile_shape>
void TSUBS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 33, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TMULS: dst = src * scalar
template <is_tile_data_v tile_shape>
void TMULS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TEPL 34, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(valid_col),
      "r"(valid_row),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TDIVS: dst = src / scalar
template <is_tile_data_v tile_shape>
void TDIVS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 35, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TREMS: dst = rem(src, scalar)
template <is_tile_data_v tile_shape>
void TREMS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 36, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TFMODS: dst = fmod(src, scalar)
template <is_tile_data_v tile_shape>
void TFMODS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 37, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TANDS: dst = src & scalar
template <is_tile_data_v tile_shape>
void TANDS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 38, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TORS: dst = src | scalar
template <is_tile_data_v tile_shape>
void TORS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 39, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TXORS: dst = src ^ scalar
template <is_tile_data_v tile_shape>
void TXORS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 40, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TSHLS: dst = src << scalar
template <is_tile_data_v tile_shape>
void TSHLS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 41, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TSHRS: dst = src >> scalar
template <is_tile_data_v tile_shape>
void TSHRS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 42, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TMAXS: dst = max(src, scalar)
template <is_tile_data_v tile_shape>
void TMAXS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 43, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TMINS: dst = min(src, scalar)
template <is_tile_data_v tile_shape>
void TMINS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 44, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TCMPS: compare src with scalar
template <is_tile_data_v tile_shape>
void TCMPS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 45, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TLRELU: leaky ReLU with scalar slope
template <is_tile_data_v tile_shape>
void TLRELU(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 46, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TAXPY: AXPY-style fused update (DavinciOO ext)
template <is_tile_data_v tile_shape>
void TAXPY(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 47, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TADDSC: dst = src0 + scalar + src1
template <is_tile_data_v tile_shape>
void TADDSC(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 56, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    "B.IOR [%8],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TSUBSC: dst = src0 - scalar + src1
template <is_tile_data_v tile_shape>
void TSUBSC(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 57, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    "B.IOR [%8],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TSELS: select between src tile and scalar using mask
template <is_tile_data_v tile_shape>
void TSELS(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 58, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    "B.IOR [%8],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TEXPANDS: broadcast scalar into dst tile
template <is_tile_data_v tile_shape>
void TEXPANDS(tile_shape &dst, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 59, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT mask=15, last, ->%0<%Z5>\n"
    "B.IOR [%6],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(dst.GetValidCol()),
      "r"(dst.GetValidRow()),
      "i"(tile_shape::Cols),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}


//===--- TEPL Mode 0 extension: TFMA (fused multiply-add, opcode 28) ---===//

// TFMA: dst = src0 * src1 + src2 (fused element-wise multiply-add)
template <is_tile_data_v tile_shape>
void TFMA(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2) {
  asm volatile(
    "BSTART.TEPL 28, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15\n"
    "B.IOT %7, mask=15, last, ->%0<%Z8>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "Tr"(src2.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

//===--- TEPL Mode 3: complex ops (opcode = 96 + Function) ---===//

// TEXTRACT: extract sub-tile (indexRow, indexCol via B.IOR)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TEXTRACT(tile_shape_out &dst, tile_shape_in &src, int32_t indexRow, int32_t indexCol) {
  // Anti-fold: keep compile-time-constant indices (e.g. 0) off the zero
  // register so B.IOR [zero,...]/[...,zero] still matches an instruction.
  volatile int32_t irv = indexRow;
  volatile int32_t icv = indexCol;
  asm volatile(
    "BSTART.TEPL 98, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7,%8],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(irv),
      "r"(icv)
  );
}

// TINSERT: insert sub-tile (indexRow, indexCol via B.IOR)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TINSERT(tile_shape_out &dst, tile_shape_in &src, int32_t indexRow, int32_t indexCol) {
  // Anti-fold: keep compile-time-constant indices (e.g. 0) off the zero
  // register so B.IOR [zero,...]/[...,zero] still matches an instruction.
  volatile int32_t irv = indexRow;
  volatile int32_t icv = indexCol;
  asm volatile(
    "BSTART.TEPL 99, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7,%8],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(irv),
      "r"(icv)
  );
}

// TIMG2COL: feature-map to im2col transform
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TIMG2COL(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 100, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TFILLPAD: copy valid region and fill padding
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TFILLPAD(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 101, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCI: contiguous integer sequence generation
template <is_tile_data_v tile_shape>
void TCI(tile_shape &dst, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 102, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT mask=15, last, ->%0<%Z5>\n"
    "B.IOR [%6],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(dst.GetValidCol()),
      "r"(dst.GetValidRow()),
      "i"(tile_shape::Cols),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TTRI: triangular mask generation
template <is_tile_data_v tile_shape>
void TTRI(tile_shape &dst) {
  asm volatile(
    "BSTART.TEPL 103, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT mask=15, last, ->%0<%Z5>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(dst.GetValidCol()),
      "r"(dst.GetValidRow()),
      "i"(tile_shape::Cols),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TRANDOM: counter-based random tile generation
template <is_tile_data_v tile_shape>
void TRANDOM(tile_shape &dst, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 105, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT mask=15, last, ->%0<%Z5>\n"
    "B.IOR [%6],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(dst.GetValidCol()),
      "r"(dst.GetValidRow()),
      "i"(tile_shape::Cols),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      "r"(sv)
  );
}

// TQUANT: profile-defined quantization
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TQUANT(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 106, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TDEQUANT: profile-defined dequantization
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TDEQUANT(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 107, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TSORT32: sort each 32-element block
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TSORT32(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 108, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TMRGSORT: merge sorted list tiles
template <is_tile_data_v tile_shape>
void TMRGSORT(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 109, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TTRANS: tile transpose
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TTRANS(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 110, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TGATHER: index/mask tile gather
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in, is_tile_data_v tile_shape_off>
void TGATHER(tile_shape_out &dst, tile_shape_in &src, tile_shape_off &off) {
  asm volatile(
    "BSTART.TEPL 111, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(off.GetValidCol()),
      "r"(off.GetValidRow()),
      "i"(tile_shape_off::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "Tr"(off.data())
  );
}

// TSCATTER: index tile scatter
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in, is_tile_data_v tile_shape_off>
void TSCATTER(tile_shape_out &dst, tile_shape_in &src, tile_shape_off &off) {
  asm volatile(
    "BSTART.TEPL 112, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7],[]\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(off.GetValidCol()),
      "r"(off.GetValidRow()),
      "i"(tile_shape_off::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "Tr"(off.data())
  );
}

// TPARTADD: partial-valid add (moved from Mode 0)
template <is_tile_data_v tile_shape>
void TPARTADD(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 113, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TPARTMUL: partial-valid multiply (moved from Mode 0)
template <is_tile_data_v tile_shape>
void TPARTMUL(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 114, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TPARTMAX: partial-valid max (moved from Mode 0)
template <is_tile_data_v tile_shape>
void TPARTMAX(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 115, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TPARTMIN: partial-valid min (moved from Mode 0)
template <is_tile_data_v tile_shape>
void TPARTMIN(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 116, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}
//===--- TEPL Mode 2: reduction/broadcast ops (BSTART.TEPL) ---===//
// opcode = Mode(2) * 32 + Function = 64 + Function.

// TROWSUM: row sum reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWSUM(tile_shape_out &dst, tile_shape_in &src) {
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TEPL 64, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(valid_col),
      "r"(valid_row),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWMAX: row max reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWMAX(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 65, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWMIN: row min reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWMIN(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 66, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWPROD: row product reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWPROD(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 67, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPAND: broadcast first element of each row
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWEXPAND(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 68, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWARGMAX: row argmax (DavinciOO ext)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWARGMAX(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 76, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWARGMIN: row argmin (DavinciOO ext)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TROWARGMIN(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 77, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLSUM: col sum reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLSUM(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 80, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLMAX: col max reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLMAX(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 81, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLMIN: col min reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLMIN(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 82, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLPROD: col product reduction
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLPROD(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 83, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPAND: broadcast first element of each col
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLEXPAND(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 84, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLARGMAX: col argmax (DavinciOO ext)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLARGMAX(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 92, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLARGMIN: col argmin (DavinciOO ext)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCOLARGMIN(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 93, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDADD: row broadcast add
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDADD(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDADD: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDADD: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 69, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDSUB: row broadcast sub
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDSUB(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDSUB: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDSUB: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 70, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDMUL: row broadcast mul
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDMUL(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDMUL: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDMUL: src0/dst dtype must match");
  const size_t valid_col = src0.GetValidCol();
  const size_t valid_row = src0.GetValidRow();
  asm volatile(
    "BSTART.TEPL 71, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(valid_col),
      "r"(valid_row),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDDIV: row broadcast div
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDDIV(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDDIV: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDDIV: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 72, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDMAX: row broadcast max
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDMAX(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDMAX: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDMAX: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 73, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDMIN: row broadcast min
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDMIN(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDMIN: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDMIN: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 74, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TROWEXPANDEXPDIF: row exp(src0-src1)
// src1 is a per-row scalar/byte-strip operand whose shape may differ from src0
// (see pto/TROWEXPANDMUL.md Mode 1/2); only dtype is required to match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TROWEXPANDEXPDIF(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TROWEXPANDEXPDIF: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TROWEXPANDEXPDIF: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 75, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDADD: col broadcast add
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDADD(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDADD: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDADD: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 85, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDSUB: col broadcast sub
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDSUB(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDSUB: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDSUB: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 86, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDMUL: col broadcast mul
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDMUL(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDMUL: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDMUL: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 87, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDDIV: col broadcast div
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDDIV(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDDIV: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDDIV: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 88, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDMAX: col broadcast max
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDMAX(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDMAX: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDMAX: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 89, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDMIN: col broadcast min
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDMIN(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDMIN: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDMIN: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 90, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TCOLEXPANDEXPDIF: col exp(src0-src1)
// src1 is a per-column scalar/byte-strip operand whose shape may differ from src0
// (col-broadcast analog of pto/TROWEXPANDMUL.md Mode 1/2); only dtype must match.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0,
          is_tile_data_v tile_shape_in1>
void TCOLEXPANDEXPDIF(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCOLEXPANDEXPDIF: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCOLEXPANDEXPDIF: src0/dst dtype must match");
  asm volatile(
    "BSTART.TEPL 91, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(src0.GetValidCol()),
      "r"(src0.GetValidRow()),
      "i"(tile_shape_in0::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

//===--- TEPL Mode 3: complex ops (BSTART.TEPL) ---===//
// opcode = Mode(3) * 32 + Function = 96 + Function.

// TCONCAT: column concat (opcode 96)
// dst = [src0 | src1] along columns: dst has shape R x (C0+C1), src0 is R x C0,
// src1 is R x C1. The three shapes are all different by construction, so this
// is NOT a broadcast -- B.DIM must describe dst's geometry (not src0's, as the
// broadcast ops do). dtype is uniform across all three (static_assert guarded).
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1>
void TCONCAT(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_in1::DType>::value,
                "TCONCAT: src0/src1 dtype must match");
  static_assert(std::is_same<typename tile_shape_in0::DType,
                             typename tile_shape_out::DType>::value,
                "TCONCAT: src0/dst dtype must match");
  // Row count is invariant across concat (src0.Rows == src1.Rows == dst.Rows);
  // dst's valid col / total col / row stride come from dst itself.
  static_assert(tile_shape_in0::Rows == tile_shape_in1::Rows,
                "TCONCAT: src0/src1 row count must match");
  static_assert(tile_shape_in0::Rows == tile_shape_out::Rows,
                "TCONCAT: src0/dst row count must match");
  asm volatile(
    "BSTART.TEPL 96, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=15, last, ->%0<%Z7>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in0::DType>::TypeCode),
      "r"(dst.GetValidCol()),
      "r"(dst.GetValidRow()),
      "i"(tile_shape_out::Cols),
      "Tr"(src0.data()),
      "Tr"(src1.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TGATHERB: byte-offset tile gather (opcode 97)
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_offset, is_global_data_v gm_shape>
void TGATHERB(tile_shape_out &dst, gm_shape &src, tile_shape_offset &offset) {
  asm volatile(
    "BSTART.TEPL 97, %c1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=15, last, ->%0<%Z6>\n"
    "B.IOR [%7], []\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
      "r"(offset.GetValidCol()),
      "r"(offset.GetValidRow()),
      "i"(tile_shape_offset::Cols),
      "Tr"(offset.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
      "r"(src.data())
  );
}

#endif // TEMPLATE_ASM_HPP
