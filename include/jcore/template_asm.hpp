#ifndef TEMPLATE_ASM_HPP
#define TEMPLATE_ASM_HPP

#include "common/pto_tile.hpp"

using namespace pto;

// Must be defined before any wrapper that carries an opaque Shared value or
// tile register through inline asm; always_inline keeps such values inside
// the caller (they must never cross the ordinary C++ ABI as integer/stack).
#define PTO_SHARED_INLINE __attribute__((always_inline)) inline

// B.IOT/B.IOS mask syntax is a four-digit binary token, while public APIs use
// the compact numeric PEMask template argument.  Select one canonical token
// at assembly time without emitting a decimal mask spelling.
#define PTO_PE_MASK_ASM(PREFIX, SUFFIX)                                        \
  ".if %c[PEMask] == 1\n" PREFIX "0001" SUFFIX                             \
  ".elseif %c[PEMask] == 2\n" PREFIX "0010" SUFFIX                         \
  ".elseif %c[PEMask] == 4\n" PREFIX "0100" SUFFIX                         \
  ".elseif %c[PEMask] == 8\n" PREFIX "1000" SUFFIX                         \
  ".elseif %c[PEMask] == 12\n" PREFIX "1100" SUFFIX                        \
  ".elseif %c[PEMask] == 14\n" PREFIX "1110" SUFFIX                        \
  ".elseif %c[PEMask] == 15\n" PREFIX "1111" SUFFIX                        \
  ".endif\n"

#define PTO_RMODE_DATR_ASM(SUFFIX)                                             \
  ".if %c[RMode] == 2\nB.DATR %D[__pto_DstType], RTZ" SUFFIX                    \
  ".elseif %c[RMode] == 3\nB.DATR %D[__pto_DstType], RTM" SUFFIX                \
  ".elseif %c[RMode] == 4\nB.DATR %D[__pto_DstType], RTP" SUFFIX                \
  ".elseif %c[RMode] == 5\nB.DATR %D[__pto_DstType], RNA" SUFFIX                \
  ".elseif %c[RMode] == 6\nB.DATR %D[__pto_DstType], RTO" SUFFIX                \
  ".elseif %c[RMode] == 7\nB.DATR %D[__pto_DstType], RHB" SUFFIX                \
  ".endif\n"

template <class...>
inline constexpr bool pto_dependent_false_v = false;

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void ACCSCALE_T(tile_shape_out &, tile_shape_in &,
                typename tile_shape_in::DType) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "ACCSCALE_T used the removed v5 ACCCVT opcode; use the "
                "active TMATMUL operation with B.FPATR");
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void ACCSCALE_NZ2DN(tile_shape_out &, tile_shape_in &,
                    typename tile_shape_in::DType) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "ACCSCALE_NZ2DN used the removed v5 ACCCVT opcode; use the "
                "active TMATMUL operation with B.FPATR");
}

template <is_tile_data_v tile_shape_max, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void ACCCVT_RMAX_SCALE_NZ2DN(tile_shape_max &, tile_shape_out &,
                            tile_shape_in &,
                            typename tile_shape_in::DType) {
  static_assert(
      pto_dependent_false_v<tile_shape_max, tile_shape_out, tile_shape_in>,
      "ACCCVT_RMAX_SCALE_NZ2DN used the removed v5 ACCCVT opcode; migrate "
      "to the active TMATMUL operation with B.FPATR and its RowMax operands");
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in0, is_tile_data_v tile_shape_in1>
void TMAX_T(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1) {
  asm volatile(
    "BSTART.VPAR 0b0000100011, %c3\n"
    "B.IOT %1, %2, mask=1111, last, ->%0<%Z4>\n"
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
    "B.IOT %1, %2, mask=1111, last, ->%0<%Z4>\n"
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
    "B.IOT %1, %2, mask=1111, 0, ->%0<%Z5>\n"
    "B.IOT %3, mask=1111, last\n"
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
    "B.IOT %1, %2, mask=1111, 0, ->%0<%Z5>\n"
    "B.IOT %3, mask=1111, last\n"
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
  static_assert((tile_shape_out::ValidRow == DYNAMIC ||
                 tile_shape_out::Rows >= tile_shape_out::ValidRow) &&
                    (tile_shape_out::ValidCol == DYNAMIC ||
                     tile_shape_out::Cols >= tile_shape_out::ValidCol),
                "TCVT destination physical shape must contain its valid "
                "region (valid_rows <= rows, valid_columns <= columns)");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  constexpr bool IsCubeMSource =
      tile_shape_in::BFractal == BLayout::CubeM16 ||
      tile_shape_in::BFractal == BLayout::CubeM32;
  if constexpr (IsCubeMSource) {
    static_assert(tile_shape_out::BFractal == tile_shape_in::BFractal,
                  "TCVT CUBE_M16/M32 conversion must preserve the CUBE layout");
    static_assert(tile_shape_out::ValidRow == tile_shape_in::ValidRow &&
                      tile_shape_out::ValidCol == tile_shape_in::ValidCol,
                  "TCVT CUBE_M16/M32 conversion must preserve the valid shape");
    static_assert(tile_shape_out::Loc == Location::Left ||
                      tile_shape_out::Loc == Location::Acc,
                  "TCVT CUBE_M16/M32 destination must have Matrix location");
    static_assert(tile_shape_in::Loc == Location::Left ||
                      tile_shape_in::Loc == Location::Acc,
                  "TCVT CUBE_M16/M32 source must have Matrix location");
    static_assert(tile_shape_in::TilesizeCode >= __tilesize_128B &&
                      tile_shape_in::TilesizeCode <= __tilesize_64KB,
                  "TCVT CUBE_M16/M32 source TSize must be 128 B..64 KiB");
    static_assert(tile_shape_out::TilesizeCode >= __tilesize_128B &&
                      tile_shape_out::TilesizeCode <= __tilesize_64KB,
                  "TCVT CUBE_M16/M32 destination TSize must be 128 B..64 KiB");
    asm volatile(
      "BSTART.TEPL 27, %D1\n"
      "B.DATR %D2, RNONE\n"
      "B.DIM %5, 0, ->lb0\n"
      "B.DIM %6, 0, ->lb1\n"
      "B.IOT %3, mask=1111, last, ->%0<%Z4>\n"
      : "=Tr"(dst.data())
      : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        "Tr"(src.data()),
        "i"(tile_shape_out::TilesizeCode),
        "r"(valid_col),
        "r"(valid_row)
    );
  } else {
    static_assert(!tile_shape_out::IsCubeLayout,
                  "TCVT to a CUBE layout requires a CUBE_M16/M32 source");
    static_assert(tile_shape_out::Rows == tile_shape_in::Rows &&
                      tile_shape_out::Cols == tile_shape_in::Cols,
                  "ordinary TCVT source and destination must have identical "
                  "physical Rows/Cols");
    static_assert(tile_shape_out::ValidRow == tile_shape_in::ValidRow &&
                      tile_shape_out::ValidCol == tile_shape_in::ValidCol,
                  "ordinary TCVT source and destination must have identical "
                  "ValidRow/ValidCol");
    asm volatile(
      "BSTART.TEPL 27, %D1\n"
      "B.DATR %D2, RNONE\n"
      "B.DIM %5, 0, ->lb0\n"
      "B.DIM %6, 0, ->lb1\n"
      "B.DIM zero, %c7, ->lb2\n"
      "B.IOT %3, mask=1111, last, ->%0<%Z4>\n"
      : "=Tr"(dst.data())
      : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        "Tr"(src.data()),
        "i"(tile_shape_out::TilesizeCode),
        "r"(valid_col),
        "r"(valid_row),
        "i"(tile_shape_out::Cols)
    );
  }
}

#define DEFINE_TMOV_LAYOUT(LAYOUT_NAME)                                          \
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>           \
void TMOV_##LAYOUT_NAME(tile_shape_out &dst, tile_shape_in &src) {               \
  asm volatile(                                                                  \
    "BSTART.TLSU TMOV, %D2\n"                                                        \
    "B.DATR " #LAYOUT_NAME ", DTYPE_NONE, Null\n"                                  \
    "B.IOT %1, mask=1111, last, ->%0<%Z3>\n"                                              \
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

// PTO ISA 0.58 generic Local-to-Local TMOV(dst, src). Engine TLSU function 2.
// Copies the payload and definedness from src to dst; this is not a dtype
// conversion, reshape, transpose, layout conversion or Local/Shared move.
// The same exact C++ Tile type is required on both sides, which statically
// guarantees matching rows/columns/valid-shape/layout/dtype. DATR layout is
// NORM with zero padding (pad union must be zero).
template <is_local_tile_v Tile>
inline void TMOV(Tile &dst, const Tile &src) {
  static_assert(
      tile_type_traits<typename Tile::TileDType>::IsValidActiveSize,
      "TMOV logical Tile size must be 128 B..256 KiB (SizeCode=1..12)");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TLSU TMOV, %D[DataType]\n"
    "B.DIM %[ValidCol], 0, ->lb0\n"
    "B.DIM %[ValidRow], 0, ->lb1\n"
    "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
    : [Dst] "=&Tr"(dst.data())
    : [Src] "Tr"(src.data()),
      [DataType] "i"(type_traits<typename Tile::DType>::TypeCode),
      [TileSize] "i"(
          tile_type_traits<typename Tile::TileDType>::TilesizeCode),
      [ValidCol] "r"(valid_col), [ValidRow] "r"(valid_row));
}

template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TMOV_DN2NZ_DYN(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TLSU TMOV, %D2\n"
    "B.DATR DN2NZ, DTYPE_NONE, Null\n"
    "B.IOT %1, mask=1111, last, ->%0<%Z3>\n"
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
    "BSTART.TEPL 104, %D1\n"                                     \
    "B.DATR %D2," BYTE_NAME "\n"                                       \
    "B.DIM %3, 0, ->LB0\n"                                         \
    "B.DIM %4, 0, ->LB1\n"                                         \
    "B.DIM zero, %c5, ->LB2\n"                                         \
    "B.IOT %6, %7, mask=1111, last, ->%0<%Z8>\n"                                \
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
    "BSTART.TLSU TLOAD, %D[__pto_SrcType]\n"
    "B.DATR ND2NZ, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=1111, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, last, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst1.GetValidCol()*2), [__pto_VROW]"r"(dst1.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*2),
      [__pto_GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD2_ND2ZN(tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Zn_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Zn ");
  asm volatile(
    "BSTART.TLSU TLOAD, %D[__pto_SrcType]\n"
    "B.DATR ND2ZN, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=1111, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, last, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst1.GetValidCol()*2), [__pto_VROW]"r"(dst1.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*2),
      [__pto_GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD2_DN2ZN(tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(!gm_shape::isRowMajor && is_Nz_layout<tile_shape>::value,
                    "GM_SHAPE should DN and TILE_SHAPE should be Zn ");
  asm volatile(
    "BSTART.TLSU TLOAD, %D[__pto_SrcType]\n"
    "B.DATR DN2ZN, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=1111, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, last, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst1.GetValidCol()), [__pto_VROW]"r"(dst1.GetValidRow()*2), [__pto_COL]"i"(tile_shape::Cols),
      [__pto_GmStride]"r"(src.GetStrideBytes(4))
      : "memory");
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TSTORE2_DN2DN(gm_shape &dst, tile_shape &src1, tile_shape &src0) {
  static_assert(!gm_shape::isRowMajor && !tile_shape::isRowMajor,
                    "GM_SHAPE should DN and TILE_SHAPE should be DN");
  asm volatile(
    "BSTART.TLSU TSTORE, %D[__pto_SrcType]\n"
    "B.DATR NORM, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT %[__pto_s0], %[s1], mask=1111, last\n"
    "B.IOR [%[__pto_d0],%[__pto_GmStride]], []\n"
    : 
    : [__pto_d0]"r"(dst.data()), [__pto_s0]"Tr"(src0.data()), [s1]"Tr"(src1.data()),
      [__pto_DstType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_VCOL]"r"(src0.GetValidRow()*2), [__pto_VROW]"r"(src0.GetValidCol()), [__pto_COL]"i"(tile_shape::Rows*2),
      [__pto_GmStride]"r"(dst.GetStrideBytes(4))
      : "memory");
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD4_ND2NZ(tile_shape &dst3, tile_shape &dst2, tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Nz_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Nz ");
  asm volatile(
    "BSTART.TLSU TLOAD, %D[__pto_SrcType]\n"
    "B.DATR ND2NZ, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=1111, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, 0, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, 0, ->%[d2]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, last, ->%[d3]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data()),[d2]"=Tr"(dst2.data()),[d3]"=Tr"(dst3.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst3.GetValidCol()*4), [__pto_VROW]"r"(dst3.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*4),
      [__pto_GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD4_ND2ZN(tile_shape &dst3, tile_shape &dst2, tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(gm_shape::isRowMajor && is_Zn_layout<tile_shape>::value,
                    "GM_SHAPE should ND and TILE_SHAPE should be Nz ");
  asm volatile(
    "BSTART.TLSU TLOAD, %D[__pto_SrcType]\n"
    "B.DATR ND2ZN, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=1111, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, 0, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, 0, ->%[d2]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, last, ->%[d3]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data()),[d2]"=Tr"(dst2.data()),[d3]"=Tr"(dst3.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst3.GetValidRow()*4), [__pto_VROW]"r"(dst3.GetValidCol()), [__pto_COL]"i"(tile_shape::Rows*4),
      [__pto_GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
}

template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
void TLOAD4_DN2ZN(tile_shape &dst3, tile_shape &dst2, tile_shape &dst1, tile_shape &dst0, gm_shape &src) {
  static_assert(!gm_shape::isRowMajor && is_Zn_layout<tile_shape>::value,
                    "GM_SHAPE should DN and TILE_SHAPE should be Zn ");
  asm volatile(
    "BSTART.TLSU TLOAD, %D[__pto_SrcType]\n"
    "B.DATR DN2ZN, %D[__pto_DstType], Null\n"
    "B.DIM %[__pto_VCOL], 0, ->lb0\n"
    "B.DIM %[__pto_VROW], 0, ->lb1\n"
    "B.DIM zero, %[__pto_COL], ->lb2\n"
    "B.IOT mask=1111, 0, ->%[__pto_d0]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, 0, ->%[__pto_d1]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, 0, ->%[d2]<%Z[__pto_TileSize]>\n"
    "B.IOT mask=1111, last, ->%[d3]<%Z[__pto_TileSize]>\n"
    "B.IOR [%[__pto_s0],%[__pto_GmStride]], []\n"
    : [__pto_d0]"=Tr"(dst0.data()),[__pto_d1]"=Tr"(dst1.data()),[d2]"=Tr"(dst2.data()),[d3]"=Tr"(dst3.data())
    : [__pto_s0]"r"(src.data()),
      [__pto_DstType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [__pto_SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [__pto_TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [__pto_VCOL]"r"(dst3.GetValidCol()*4), [__pto_VROW]"r"(dst3.GetValidRow()), [__pto_COL]"i"(tile_shape::Cols*4),
      [__pto_GmStride]"r"(src.GetStrideBytes(4))
      : "memory");
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
                "MGATHER dst logical Tile size must be 128 B..256 KiB (SizeCode=1..12) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MGATHER, %D[DataType]\n"
      "B.DATR Null\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[off], mask=1111, last, ->%[dst]<%Z[TileSize]>\n"
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
        [GmStride] "r"(src.GetStride(3))
      : "memory");
}

template <typename tile_shape_in, typename tile_shape_offset, typename gm_shape>
inline void MSCATTER(gm_shape &dst, const tile_shape_in &src,
                     const tile_shape_offset &offset) {
  static_assert(tile_shape_offset::ValidCol <= tile_shape_offset::Cols, "");
  static_assert(tile_type_traits<typename tile_shape_in::TileDType>::IsValidActiveSize,
                "MSCATTER src logical Tile size must be 128 B..256 KiB (SizeCode=1..12) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MSCATTER, %D[DataType]\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[src], %[off], mask=1111, last\n"
      "B.IOR [%[base], %[GmStride]], []\n"
      :
      : [base] "r"(dst.data()), [src] "Tr"(src.data()),
        [off] "Tr"(offset.data()),
        [DataType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [ValidCol] "r"(offset.GetValidCol()),
        [ValidRow] "r"(offset.GetValidRow()),
        [Col] "i"(tile_shape_offset::Cols),
        [GmStride] "r"(dst.GetStride(3))
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
                "MGATHER_MASK dst logical Tile size must be 128 B..256 KiB (SizeCode=1..12) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MGATHER.MASK, %D[DataType]\n"
      "B.DATR Null\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[off], mask=1111, 0, ->%[dst]<%Z[TileSize]>\n"
      "B.IOT %[mask], mask=1111, last\n"
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
        [GmStride] "r"(src.GetStride(3))
      : "memory");
}

template <typename tile_shape_in, typename tile_shape_offset,
          typename tile_shape_mask, typename gm_shape>
inline void MSCATTER_MASK(gm_shape &dst, const tile_shape_in &src,
                          const tile_shape_offset &offset,
                          const tile_shape_mask &mask) {
  static_assert(tile_shape_offset::ValidCol <= tile_shape_offset::Cols, "");
  static_assert(tile_type_traits<typename tile_shape_in::TileDType>::IsValidActiveSize,
                "MSCATTER_MASK src logical Tile size must be 128 B..256 KiB (SizeCode=1..12) "
                "per DavinciOO v5 B.IOT encoding");
  asm volatile(
      "BSTART.TLSU MSCATTER.MASK, %D[DataType]\n"
      "B.DIM %[ValidCol], 0, ->LB0\n"
      "B.DIM %[ValidRow], 0, ->LB1\n"
      "B.DIM zero, %c[Col], ->LB2\n"
      "B.IOT %[src], %[off], mask=1111, 0\n"
      "B.IOT %[mask], mask=1111, last\n"
      "B.IOR [%[base], %[GmStride]], []\n"
      :
      : [base] "r"(dst.data()), [src] "Tr"(src.data()),
        [off] "Tr"(offset.data()), [mask] "Tr"(mask.data()),
        [DataType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [ValidCol] "r"(offset.GetValidCol()),
        [ValidRow] "r"(offset.GetValidRow()),
        [Col] "i"(tile_shape_offset::Cols),
        [GmStride] "r"(dst.GetStride(3))
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
                           "RTM", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RDN && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTM", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTP", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_NORMAL(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC,                \
                           "RTP", "sat");                                     \
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
                           "RTM", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RDN && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTM", "sat");                                     \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_NOSAT) {        \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTP", "nosat");                                   \
    } else if constexpr ((RMODE) == LINX_RUP && (SAT) == LINX_SAT) {          \
      LINX_CVT_EMIT_PACKED(SRC_TYPE, DST_TYPE, SRC_REG, DST_REG,              \
                           DST_STORAGE, SRC_STORAGE, DST, SRC0, SRC1,         \
                           "RTP", "sat");                                     \
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
      EMIT(__VA_ARGS__, "RTM", "nosat");                                      \
    } else if constexpr ((RMODE) == 3 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RTM", "sat");                                        \
    } else if constexpr ((RMODE) == 4 && (SAT) == 0) {                        \
      EMIT(__VA_ARGS__, "RTP", "nosat");                                      \
    } else if constexpr ((RMODE) == 4 && (SAT) == 1) {                        \
      EMIT(__VA_ARGS__, "RTP", "sat");                                        \
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
// These map 1:1 to the PTO-ISA v0.58 (LinxISA) tile-operation catalog. The
// interface name IS the tileop name: programmers call TLOAD / TSTORE /
// MGATHER / ... and get the hand-written block assembly directly.
//
// Block-start function values per the pinned LinxISA v0.58 catalog
// (contracts/linxisa-v0.58-engine-ops.json):
//   named TLSU starts: BSTART.TLSU TLOAD/TSTORE/TMOV/TPREFETCH/MGATHER/MSCATTER
//                    MGATHER_MASK=6 MSCATTER_MASK=7 MGATHER_CAS=8 GMOV=13
//   named CUBE starts: TMATMUL, TMATMUL.BIAS, TMATMUL.ACC, TMATMULMX,
//                   TMATMULMX.BIAS=5, TMATMULMX.ACC=6,
//                   TGEMV=16, TGEMV.BIAS=17, TGEMV.ACC=18,
//                   TGEMVMX=20, TGEMVMX.BIAS=21, TGEMVMX.ACC=22
// Active inline assembly uses the exact named TLSU/CUBE starts. VEC/SFU names
// are rendered from the TEPL Mode/Function carrier without changing encoding.
// The historical `TMATMUL*.FIXP` suffix was an implementation-local name;
// PTO-ISA 0.58 carries post-processing through the B.FPATR attribute, so the
// canonical emission is `BSTART.CUBE TMATMUL` + `B.FPATR`.
// All variants below are the NORM (no layout conversion) generic form.
//===----------------------------------------------------------------------===//

// TLOAD: GM -> Local Tile (BSTART.TLSU TLOAD). dst[i,j] = src[r0+i, c0+j].
template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
  requires(!tile_shape::IsCubeLayout)
void TLOAD(tile_shape &dst, gm_shape &src) {
  static_assert(!is_subview_v<tile_shape>,
                "B.SUBVIEW is source-only and cannot wrap a TLOAD destination");
  static_assert(
      tile_type_traits<typename tile_shape::TileDType>::IsValidActiveSize,
      "TLOAD dst logical Tile size must be 128 B..256 KiB (SizeCode=1..12)");
  const size_t valid_col = dst.GetValidCol();
  const size_t valid_row = dst.GetValidRow();
  if constexpr (is_assemble_v<tile_shape>) {
    using ParentTile = typename tile_shape::ParentTile;
    if constexpr (is_shared_tile_v<ParentTile>) {
      static_assert(tile_type_traits<typename ParentTile::TileDType>::
                        IsValidSharedActiveSize,
                    "Shared B.ASSEMBLE destination size must be 128 B..256 KB");
      if constexpr (tile_shape::RegSrc == range::AutoRegSrc) {
        const uintptr_t range_base =
            static_cast<uintptr_t>(dst.GetRangeBase());
        asm volatile(
          "BSTART.TLSU TLOAD, %D[SrcType]\n"
          "B.DIM %[VCOL], 0, ->lb0\n"
          "B.DIM %[VROW], 0, ->lb1\n"
          "B.DIM zero, %c[COL], ->lb2\n"
          "B.IOS mask=1111, ->%S[d0]<%Z[TileSize]>\n"
          "B.ASSEMBLE %c[Init], %c[Last], %[RegSrc], %c[Off], %c[ParentSize]\n"
          "B.IOR [%[s0],%[GmStride]], []\n"
          : [d0]"=Sr"(dst.handle_ref())
          : [s0]"r"(src.data()),
            [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
            [TileSize]"i"(tile_type_traits<typename ParentTile::TileDType>::TilesizeCode),
            [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
            [COL]"i"(ParentTile::Cols),
            [GmStride]"r"(src.GetStrideBytes(3)),
            [Init]"i"(static_cast<int>(tile_shape::INIT)),
            [Last]"i"(static_cast<int>(tile_shape::LAST)),
            [RegSrc]"r"(range_base),
            [Off]"i"(tile_shape::OffsetUnits),
            [ParentSize]"i"(tile_shape::ParentSizeCode)
          : "memory");
      } else {
      #define PTO_SHARED_RANGE_ASSEMBLE_CASE(N) \
        if constexpr (tile_shape::RegSrc == N) { \
          register uintptr_t range_base asm("r" #N) = \
              static_cast<uintptr_t>(dst.GetRangeBase()); \
          asm volatile( \
            "BSTART.TLSU TLOAD, %D[SrcType]\n" \
            "B.DIM %[VCOL], 0, ->lb0\n" \
            "B.DIM %[VROW], 0, ->lb1\n" \
            "B.DIM zero, %c[COL], ->lb2\n" \
            "B.IOS mask=1111, ->%S[d0]<%Z[TileSize]>\n" \
            "B.ASSEMBLE %c[Init], %c[Last], r" #N ", %c[Off], %c[ParentSize]\n" \
            "B.IOR [%[s0],%[GmStride]], []\n" \
            : [d0]"=Sr"(dst.handle_ref()) \
            : [s0]"r"(src.data()), \
              [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode), \
              [TileSize]"i"(tile_type_traits<typename ParentTile::TileDType>::TilesizeCode), \
              [VCOL]"r"(valid_col), [VROW]"r"(valid_row), \
              [COL]"i"(ParentTile::Cols), \
              [GmStride]"r"(src.GetStrideBytes(3)), \
              [Init]"i"(static_cast<int>(tile_shape::INIT)), \
              [Last]"i"(static_cast<int>(tile_shape::LAST)), \
              [RegSrc]"r"(range_base), \
              [Off]"i"(tile_shape::OffsetUnits), \
              [ParentSize]"i"(tile_shape::ParentSizeCode) \
            : "memory"); \
        }
      PTO_SHARED_RANGE_ASSEMBLE_CASE(0) else PTO_SHARED_RANGE_ASSEMBLE_CASE(1) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(2) else PTO_SHARED_RANGE_ASSEMBLE_CASE(3) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(4) else PTO_SHARED_RANGE_ASSEMBLE_CASE(5) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(6) else PTO_SHARED_RANGE_ASSEMBLE_CASE(7) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(8) else PTO_SHARED_RANGE_ASSEMBLE_CASE(9) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(10) else PTO_SHARED_RANGE_ASSEMBLE_CASE(11) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(12) else PTO_SHARED_RANGE_ASSEMBLE_CASE(13) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(14) else PTO_SHARED_RANGE_ASSEMBLE_CASE(15) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(16) else PTO_SHARED_RANGE_ASSEMBLE_CASE(17) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(18) else PTO_SHARED_RANGE_ASSEMBLE_CASE(19) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(20) else PTO_SHARED_RANGE_ASSEMBLE_CASE(21) else
      PTO_SHARED_RANGE_ASSEMBLE_CASE(22) else PTO_SHARED_RANGE_ASSEMBLE_CASE(23)
      #undef PTO_SHARED_RANGE_ASSEMBLE_CASE
      }
    } else {
    // Destination wraps a B.ASSEMBLE range modifier attached to the B.IOT
    // destination binder (PTO-ISA 0.58.4 ADR-0098). INIT/LAST/Offset are
    // compile-time wrapper parameters so they satisfy the "i" constraint.
    if constexpr (tile_shape::RegSrc == range::AutoRegSrc) {
      const uintptr_t range_base =
          static_cast<uintptr_t>(dst.GetRangeBase());
      asm volatile(
        "BSTART.TLSU TLOAD, %D[SrcType]\n"
        "B.DIM %[VCOL], 0, ->lb0\n"
        "B.DIM %[VROW], 0, ->lb1\n"
        "B.DIM zero, %c[COL], ->lb2\n"
        "B.IOT mask=1111, last, ->%[d0]<%Z[TileSize]>\n"
        "B.ASSEMBLE %c[Init], %c[Last], %[RegSrc], %c[Off], %c[ParentSize]\n"
        "B.IOR [%[s0],%[GmStride]], []\n"
        : [d0]"=Tr"(dst.data())
        : [s0]"r"(src.data()),
          [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
          [TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
          [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
          [COL]"i"(tile_shape::Cols),
          [GmStride]"r"(src.GetStrideBytes(3)),
          [Init]"i"(static_cast<int>(tile_shape::INIT)),
          [Last]"i"(static_cast<int>(tile_shape::LAST)),
          [RegSrc]"r"(range_base),
          [Off]"i"(tile_shape::OffsetUnits),
          [ParentSize]"i"(tile_shape::ParentSizeCode)
        : "memory");
    } else {
    #define PTO_RANGE_ASSEMBLE_CASE(N) \
      if constexpr (tile_shape::RegSrc == N) { \
        register uintptr_t range_base asm("r" #N) = \
            static_cast<uintptr_t>(dst.GetRangeBase()); \
        asm volatile( \
          "BSTART.TLSU TLOAD, %D[SrcType]\n" \
          "B.DIM %[VCOL], 0, ->lb0\n" \
          "B.DIM %[VROW], 0, ->lb1\n" \
          "B.DIM zero, %c[COL], ->lb2\n" \
          "B.IOT mask=1111, last, ->%[d0]<%Z[TileSize]>\n" \
          "B.ASSEMBLE %c[Init], %c[Last], r" #N ", %c[Off], %c[ParentSize]\n" \
          "B.IOR [%[s0],%[GmStride]], []\n" \
          : [d0]"=Tr"(dst.data()) \
          : [s0]"r"(src.data()), \
            [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode), \
            [TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode), \
            [VCOL]"r"(valid_col), [VROW]"r"(valid_row), \
            [COL]"i"(tile_shape::Cols), \
            [GmStride]"r"(src.GetStrideBytes(3)), \
            [Init]"i"(static_cast<int>(tile_shape::INIT)), \
            [Last]"i"(static_cast<int>(tile_shape::LAST)), \
            [RegSrc]"r"(range_base), \
            [Off]"i"(tile_shape::OffsetUnits), \
            [ParentSize]"i"(tile_shape::ParentSizeCode) \
          : "memory"); \
      }
    PTO_RANGE_ASSEMBLE_CASE(0) else PTO_RANGE_ASSEMBLE_CASE(1) else
    PTO_RANGE_ASSEMBLE_CASE(2) else PTO_RANGE_ASSEMBLE_CASE(3) else
    PTO_RANGE_ASSEMBLE_CASE(4) else PTO_RANGE_ASSEMBLE_CASE(5) else
    PTO_RANGE_ASSEMBLE_CASE(6) else PTO_RANGE_ASSEMBLE_CASE(7) else
    PTO_RANGE_ASSEMBLE_CASE(8) else PTO_RANGE_ASSEMBLE_CASE(9) else
    PTO_RANGE_ASSEMBLE_CASE(10) else PTO_RANGE_ASSEMBLE_CASE(11) else
    PTO_RANGE_ASSEMBLE_CASE(12) else PTO_RANGE_ASSEMBLE_CASE(13) else
    PTO_RANGE_ASSEMBLE_CASE(14) else PTO_RANGE_ASSEMBLE_CASE(15) else
    PTO_RANGE_ASSEMBLE_CASE(16) else PTO_RANGE_ASSEMBLE_CASE(17) else
    PTO_RANGE_ASSEMBLE_CASE(18) else PTO_RANGE_ASSEMBLE_CASE(19) else
    PTO_RANGE_ASSEMBLE_CASE(20) else PTO_RANGE_ASSEMBLE_CASE(21) else
    PTO_RANGE_ASSEMBLE_CASE(22) else PTO_RANGE_ASSEMBLE_CASE(23)
    #undef PTO_RANGE_ASSEMBLE_CASE
    }
    }
  } else {
  asm volatile(
    "BSTART.TLSU TLOAD, %D[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOT mask=1111, last, ->%[d0]<%Z[TileSize]>\n"
    "B.IOR [%[s0],%[GmStride]], []\n"
    : [d0]"=Tr"(dst.data())
    : [s0]"r"(src.data()),
      [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [TileSize]"i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(tile_shape::Cols),
      [GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
  }
}

// TLOAD: GM -> Shared Tile (PTO v0.58 reissue). The destination is one
// absolute Core-local Shared register; B.IOS carries the per-PE size and PE
// mask. B.IOR carries only the GM address operands (RegDst is zero).
template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
PTO_SHARED_INLINE SharedTile<shp> TLOAD(const gm_shape &src) {
  using shp_dtype = typename shp::TileDType;
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "PEMask must be one of 1,2,4,8,12,14,15");
  static_assert(
      tile_type_traits<shp_dtype>::IsValidSharedActiveSize,
      "TLOAD Shared dst logical Tile size must be 128 B..256 KB (SizeCode=1..12)");
  SharedTile<shp> result;
  const size_t valid_col = result.GetValidCol();
  const size_t valid_row = result.GetValidRow();
  asm volatile(
    "BSTART.TLSU TLOAD, %D[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    PTO_PE_MASK_ASM("B.IOS mask=", ", ->%S[Shared]<%Z[TileSize]>\n")
    "B.IOR [%[s0],%[GmStride]], []\n"
    : [Shared] "=Sr"(result.handle_ref())
    : [s0]"r"(src.data()),
      [PEMask]"i"(PEMask),
      [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [TileSize]"i"(tile_type_traits<shp_dtype>::TilesizeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(shp::Cols),
      [GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
  return result;
}

template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
PTO_SHARED_INLINE void TLOAD(SharedTile<shp> &dst, const gm_shape &src) {
  using shp_dtype = typename shp::TileDType;
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "PEMask must be one of 1,2,4,8,12,14,15");
  static_assert(
      tile_type_traits<shp_dtype>::IsValidSharedActiveSize,
      "TLOAD Shared dst logical Tile size must be 128 B..256 KB (SizeCode=1..12)");
  const size_t valid_col = dst.GetValidCol();
  const size_t valid_row = dst.GetValidRow();
  asm volatile(
    "BSTART.TLSU TLOAD, %D[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    PTO_PE_MASK_ASM("B.IOS mask=", ", ->%S[Shared]<%Z[TileSize]>\n")
    "B.IOR [%[s0],%[GmStride]], []\n"
    : [Shared] "=Sr"(dst.handle_ref())
    : [s0]"r"(src.data()),
      [PEMask]"i"(PEMask),
      [SrcType]"i"(type_traits<typename gm_shape::DType>::TypeCode),
      [TileSize]"i"(tile_type_traits<shp_dtype>::TilesizeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(shp::Cols),
      [GmStride]"r"(src.GetStrideBytes(3))
      : "memory");
}

// TSTORE: Tile -> GM (BSTART.TLSU TSTORE). dst[r0+i, c0+j] = src[i,j].
template <is_global_data_v gm_shape, is_tile_data_v tile_shape>
  requires(!tile_shape::IsCubeLayout)
void TSTORE(gm_shape &dst, tile_shape &src) {
  static_assert(!is_assemble_v<tile_shape>,
                "B.ASSEMBLE is destination-only and cannot wrap a TSTORE source");
  static_assert(tile_type_traits<typename tile_shape::TileDType>::IsValidActiveSize,
                "TSTORE src logical Tile size must be 128 B..256 KiB (SizeCode=1..12) "
                "per DavinciOO v5 B.IOT encoding");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  if constexpr (is_subview_v<tile_shape>) {
    using ParentTile = typename tile_shape::ParentTile;
    if constexpr (is_shared_tile_v<ParentTile>) {
      static_assert(tile_type_traits<typename ParentTile::TileDType>::
                        IsValidSharedActiveSize,
                    "Shared B.SUBVIEW source size must be 128 B..256 KB");
      if constexpr (tile_shape::RegSrc == range::AutoRegSrc) {
        const uintptr_t range_base =
            static_cast<uintptr_t>(src.GetRangeBase());
        asm volatile(
          "BSTART.TLSU TSTORE, %D[SrcType]\n"
          "B.DIM %[VCOL], 0, ->lb0\n"
          "B.DIM %[VROW], 0, ->lb1\n"
          "B.DIM zero, %c[COL], ->lb2\n"
          "B.IOS %S[s0], mask=1111\n"
          "B.SUBVIEW %c[SrcSelect], %[RegSrc], %c[Off], %c[SubSize]\n"
          "B.IOR [%[d0],%[GmStride]], []\n"
          :
          : [d0]"r"(dst.data()), [s0]"Sr"(src.handle()),
            [SrcType]"i"(type_traits<typename ParentTile::DType>::TypeCode),
            [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
            [COL]"i"(ParentTile::Cols),
            [GmStride]"r"(dst.GetStrideBytes(3)),
            [SrcSelect]"i"(0), [RegSrc]"r"(range_base),
            [Off]"i"(tile_shape::OffsetUnits),
            [SubSize]"i"(tile_shape::SubviewSizeCode)
          : "memory");
      } else {
      #define PTO_SHARED_RANGE_SUBVIEW_CASE(N) \
        if constexpr (tile_shape::RegSrc == N) { \
          register uintptr_t range_base asm("r" #N) = \
              static_cast<uintptr_t>(src.GetRangeBase()); \
          asm volatile( \
            "BSTART.TLSU TSTORE, %D[SrcType]\n" \
            "B.DIM %[VCOL], 0, ->lb0\n" \
            "B.DIM %[VROW], 0, ->lb1\n" \
            "B.DIM zero, %c[COL], ->lb2\n" \
            "B.IOS %S[s0], mask=1111\n" \
            "B.SUBVIEW %c[SrcSelect], r" #N ", %c[Off], %c[SubSize]\n" \
            "B.IOR [%[d0],%[GmStride]], []\n" \
            : \
            : [d0]"r"(dst.data()), [s0]"Sr"(src.handle()), \
              [SrcType]"i"(type_traits<typename ParentTile::DType>::TypeCode), \
              [VCOL]"r"(valid_col), [VROW]"r"(valid_row), \
              [COL]"i"(ParentTile::Cols), \
              [GmStride]"r"(dst.GetStrideBytes(3)), \
              [SrcSelect]"i"(0), [RegSrc]"r"(range_base), \
              [Off]"i"(tile_shape::OffsetUnits), \
              [SubSize]"i"(tile_shape::SubviewSizeCode) \
            : "memory"); \
        }
      PTO_SHARED_RANGE_SUBVIEW_CASE(0) else PTO_SHARED_RANGE_SUBVIEW_CASE(1) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(2) else PTO_SHARED_RANGE_SUBVIEW_CASE(3) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(4) else PTO_SHARED_RANGE_SUBVIEW_CASE(5) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(6) else PTO_SHARED_RANGE_SUBVIEW_CASE(7) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(8) else PTO_SHARED_RANGE_SUBVIEW_CASE(9) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(10) else PTO_SHARED_RANGE_SUBVIEW_CASE(11) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(12) else PTO_SHARED_RANGE_SUBVIEW_CASE(13) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(14) else PTO_SHARED_RANGE_SUBVIEW_CASE(15) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(16) else PTO_SHARED_RANGE_SUBVIEW_CASE(17) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(18) else PTO_SHARED_RANGE_SUBVIEW_CASE(19) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(20) else PTO_SHARED_RANGE_SUBVIEW_CASE(21) else
      PTO_SHARED_RANGE_SUBVIEW_CASE(22) else PTO_SHARED_RANGE_SUBVIEW_CASE(23)
      #undef PTO_SHARED_RANGE_SUBVIEW_CASE
      }
    } else {
    // Source wraps a B.SUBVIEW range modifier attached to the B.IOT source
    // binder (PTO-ISA 0.58.4 ADR-0098).
    if constexpr (tile_shape::RegSrc == range::AutoRegSrc) {
      const uintptr_t range_base =
          static_cast<uintptr_t>(src.GetRangeBase());
      asm volatile(
        "BSTART.TLSU TSTORE, %D[SrcType]\n"
        "B.DIM %[VCOL], 0, ->lb0\n"
        "B.DIM %[VROW], 0, ->lb1\n"
        "B.DIM zero, %c[COL], ->lb2\n"
        "B.IOT %[s0], mask=1111, last\n"
        "B.SUBVIEW %c[SrcSelect], %[RegSrc], %c[Off], %c[SubSize]\n"
        "B.IOR [%[d0],%[GmStride]], []\n"
        :
        : [d0]"r"(dst.data()), [s0]"Tr"(src.data()),
          [SrcType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
          [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
          [COL]"i"(tile_shape::Cols),
          [GmStride]"r"(dst.GetStrideBytes(3)),
          [SrcSelect]"i"(0), [RegSrc]"r"(range_base),
          [Off]"i"(tile_shape::OffsetUnits),
          [SubSize]"i"(tile_shape::SubviewSizeCode)
        : "memory");
    } else {
    #define PTO_RANGE_SUBVIEW_CASE(N) \
      if constexpr (tile_shape::RegSrc == N) { \
        register uintptr_t range_base asm("r" #N) = \
            static_cast<uintptr_t>(src.GetRangeBase()); \
        asm volatile( \
          "BSTART.TLSU TSTORE, %D[SrcType]\n" \
          "B.DIM %[VCOL], 0, ->lb0\n" \
          "B.DIM %[VROW], 0, ->lb1\n" \
          "B.DIM zero, %c[COL], ->lb2\n" \
          "B.IOT %[s0], mask=1111, last\n" \
          "B.SUBVIEW %c[SrcSelect], r" #N ", %c[Off], %c[SubSize]\n" \
          "B.IOR [%[d0],%[GmStride]], []\n" \
          : \
          : [d0]"r"(dst.data()), [s0]"Tr"(src.data()), \
            [SrcType]"i"(type_traits<typename tile_shape::DType>::TypeCode), \
            [VCOL]"r"(valid_col), [VROW]"r"(valid_row), \
            [COL]"i"(tile_shape::Cols), \
            [GmStride]"r"(dst.GetStrideBytes(3)), \
            [SrcSelect]"i"(0), \
            [RegSrc]"r"(range_base), \
            [Off]"i"(tile_shape::OffsetUnits), \
            [SubSize]"i"(tile_shape::SubviewSizeCode) \
          : "memory"); \
      }
    PTO_RANGE_SUBVIEW_CASE(0) else PTO_RANGE_SUBVIEW_CASE(1) else
    PTO_RANGE_SUBVIEW_CASE(2) else PTO_RANGE_SUBVIEW_CASE(3) else
    PTO_RANGE_SUBVIEW_CASE(4) else PTO_RANGE_SUBVIEW_CASE(5) else
    PTO_RANGE_SUBVIEW_CASE(6) else PTO_RANGE_SUBVIEW_CASE(7) else
    PTO_RANGE_SUBVIEW_CASE(8) else PTO_RANGE_SUBVIEW_CASE(9) else
    PTO_RANGE_SUBVIEW_CASE(10) else PTO_RANGE_SUBVIEW_CASE(11) else
    PTO_RANGE_SUBVIEW_CASE(12) else PTO_RANGE_SUBVIEW_CASE(13) else
    PTO_RANGE_SUBVIEW_CASE(14) else PTO_RANGE_SUBVIEW_CASE(15) else
    PTO_RANGE_SUBVIEW_CASE(16) else PTO_RANGE_SUBVIEW_CASE(17) else
    PTO_RANGE_SUBVIEW_CASE(18) else PTO_RANGE_SUBVIEW_CASE(19) else
    PTO_RANGE_SUBVIEW_CASE(20) else PTO_RANGE_SUBVIEW_CASE(21) else
    PTO_RANGE_SUBVIEW_CASE(22) else PTO_RANGE_SUBVIEW_CASE(23)
    #undef PTO_RANGE_SUBVIEW_CASE
    }
    }
  } else {
  asm volatile(
    "BSTART.TLSU TSTORE, %D[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOT %[s0], mask=1111, last\n"
    "B.IOR [%[d0],%[GmStride]], []\n"
    :
    : [d0]"r"(dst.data()), [s0]"Tr"(src.data()),
      [SrcType]"i"(type_traits<typename tile_shape::DType>::TypeCode),
      [VCOL]"r"(valid_col), [VROW]"r"(valid_row),
      [COL]"i"(tile_shape::Cols),
      [GmStride]"r"(dst.GetStrideBytes(3))
      : "memory");
  }
}

// PTO ISA 0.58.3 GM -> persistent Local CUBE CELL transport.  The layout
// conversion is explicit, LB0/LB1 carry logical valid columns/rows, LB2 is
// absent, and SizeCode describes capacity rather than logical M/N/K.
template <is_local_tile_v cube_shape, is_global_data_v gm_shape>
  requires(cube_shape::IsCubeLayout)
void TLOAD_CUBE(cube_shape &dst, gm_shape &src) {
  static_assert(std::is_same_v<typename cube_shape::DType,
                               typename gm_shape::DType>,
                "TLOAD_CUBE requires matching GM and CUBE dtypes");
  static_assert(cube_shape::CubeRequiredBytes <= cube_shape::LogicalTileBytes,
                "TLOAD_CUBE CUBE CELL storage exceeds Local SizeCode capacity");
  static_assert(cube_shape::IsValidActiveSize,
                "TLOAD_CUBE Local CUBE capacity must be 128 B..256 KiB");
  const size_t valid_col = dst.GetValidCol();
  const size_t valid_row = dst.GetValidRow();
  // Persistent CUBE cell layout selects the canonical GM->Local transport
  // selector (PTO-ISA ADR-0070); the B.DATR line is fixed per layout class,
  // with load padding Zero. Selector text is constant, not an integer
  // immediate, so it assembles as the ISA mnemonic.
  if constexpr (cube_shape::BFractal == BLayout::CubeM32) {
  asm volatile(
      "BSTART.TLSU TLOAD, %D[DataType]\n"
      "B.DATR ND2M32.normal, Zero\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.IOT mask=1111, last, ->%[Dst]<%Z[SizeCode]>\n"
      "B.IOR [%[Base],%[RowStrideBytes]], []\n"
      : [Dst] "=Tr"(dst.data())
      : [Base] "r"(src.data()),
        [RowStrideBytes] "r"(src.GetStrideBytes(3)),
        [DataType] "i"(type_traits<typename cube_shape::DType>::TypeCode),
        [SizeCode] "i"(cube_shape::TilesizeCode),
        [VCOL] "r"(valid_col), [VROW] "r"(valid_row)
      : "memory");
  } else if constexpr (cube_shape::BFractal == BLayout::CubeM16) {
  asm volatile(
      "BSTART.TLSU TLOAD, %D[DataType]\n"
      "B.DATR ND2M16.normal, Zero\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.IOT mask=1111, last, ->%[Dst]<%Z[SizeCode]>\n"
      "B.IOR [%[Base],%[RowStrideBytes]], []\n"
      : [Dst] "=Tr"(dst.data())
      : [Base] "r"(src.data()),
        [RowStrideBytes] "r"(src.GetStrideBytes(3)),
        [DataType] "i"(type_traits<typename cube_shape::DType>::TypeCode),
        [SizeCode] "i"(cube_shape::TilesizeCode),
        [VCOL] "r"(valid_col), [VROW] "r"(valid_row)
      : "memory");
  } else { // CubeN8
  asm volatile(
      "BSTART.TLSU TLOAD, %D[DataType]\n"
      "B.DATR ND2N8.normal, Zero\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.IOT mask=1111, last, ->%[Dst]<%Z[SizeCode]>\n"
      "B.IOR [%[Base],%[RowStrideBytes]], []\n"
      : [Dst] "=Tr"(dst.data())
      : [Base] "r"(src.data()),
        [RowStrideBytes] "r"(src.GetStrideBytes(3)),
        [DataType] "i"(type_traits<typename cube_shape::DType>::TypeCode),
        [SizeCode] "i"(cube_shape::TilesizeCode),
        [VCOL] "r"(valid_col), [VROW] "r"(valid_row)
      : "memory");
  }
}

// PTO ISA 0.58.3 persistent Local CUBE CELL -> GM transport.  The source
// descriptor survives the operation and only its logical valid rectangle is
// exported.
template <is_global_data_v gm_shape, is_local_tile_v cube_shape>
  requires(cube_shape::IsCubeLayout)
void TSTORE_CUBE(gm_shape &dst, const cube_shape &src) {
  static_assert(std::is_same_v<typename cube_shape::DType,
                               typename gm_shape::DType>,
                "TSTORE_CUBE requires matching GM and CUBE dtypes");
  static_assert(cube_shape::IsValidActiveSize,
                "TSTORE_CUBE Local CUBE capacity must be 128 B..256 KiB");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  // Persistent CUBE cell layout selects the canonical Local->GM transport
  // selector (PTO-ISA ADR-0070); store padding is Null. Selector text is
  // constant per layout class, not an integer immediate.
  if constexpr (cube_shape::BFractal == BLayout::CubeM32) {
  asm volatile(
      "BSTART.TLSU TSTORE, %D[DataType]\n"
      "B.DATR M322ND.normal, Null\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.IOT %[Src], mask=1111, last\n"
      "B.IOR [%[Base],%[RowStrideBytes]], []\n"
      :
      : [Base] "r"(dst.data()), [Src] "Tr"(src.data()),
        [RowStrideBytes] "r"(dst.GetStrideBytes(3)),
        [DataType] "i"(type_traits<typename cube_shape::DType>::TypeCode),
        [VCOL] "r"(valid_col), [VROW] "r"(valid_row)
      : "memory");
  } else if constexpr (cube_shape::BFractal == BLayout::CubeM16) {
  asm volatile(
      "BSTART.TLSU TSTORE, %D[DataType]\n"
      "B.DATR M162ND.normal, Null\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.IOT %[Src], mask=1111, last\n"
      "B.IOR [%[Base],%[RowStrideBytes]], []\n"
      :
      : [Base] "r"(dst.data()), [Src] "Tr"(src.data()),
        [RowStrideBytes] "r"(dst.GetStrideBytes(3)),
        [DataType] "i"(type_traits<typename cube_shape::DType>::TypeCode),
        [VCOL] "r"(valid_col), [VROW] "r"(valid_row)
      : "memory");
  } else { // CubeN8
  asm volatile(
      "BSTART.TLSU TSTORE, %D[DataType]\n"
      "B.DATR N82ND.normal, Null\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.IOT %[Src], mask=1111, last\n"
      "B.IOR [%[Base],%[RowStrideBytes]], []\n"
      :
      : [Base] "r"(dst.data()), [Src] "Tr"(src.data()),
        [RowStrideBytes] "r"(dst.GetStrideBytes(3)),
        [DataType] "i"(type_traits<typename cube_shape::DType>::TypeCode),
        [VCOL] "r"(valid_col), [VROW] "r"(valid_row)
      : "memory");
  }
}

// Unified transport entry points. CUBE Tiles require the explicit ND<->CELL
// layout conversion implemented by TLOAD_CUBE/TSTORE_CUBE; dispatching from
// the Tile layout keeps the common kernel spelling uniform without removing
// the explicit expert interfaces.
template <is_tile_data_v cube_shape, is_global_data_v gm_shape>
  requires(cube_shape::IsCubeLayout)
void TLOAD(cube_shape &dst, gm_shape &src) {
  TLOAD_CUBE(dst, src);
}

template <is_global_data_v gm_shape, is_tile_data_v cube_shape>
  requires(cube_shape::IsCubeLayout)
void TSTORE(gm_shape &dst, const cube_shape &src) {
  TSTORE_CUBE(dst, src);
}

// TSTORE: Shared Tile -> GM (PTO ISA 0.58.3 TLSU Function 1 Shared form).
// Exactly one source B.IOS (PE_MASK=1111), no B.IOT, at most one B.IOR
// (GM base + logical row stride). Symmetric to Shared TLOAD.
template <is_global_data_v gm_shape, is_shared_tile_v SharedTileT>
PTO_SHARED_INLINE void TSTORE(gm_shape &dst, const SharedTileT &src) {
  using LocalType = typename SharedTileT::LocalTileType;
  static_assert(std::is_same_v<typename LocalType::DType,
                               typename gm_shape::DType>,
                "Shared TSTORE requires matching GM and Shared dtypes");
  static_assert(LocalType::isRowMajor && !LocalType::isBoxedLayout,
                "Shared TSTORE supports NORM/RowMajor Local sources only "
                "(no B.DATR Layout is emitted)");
  static_assert(tile_type_traits<typename LocalType::TileDType>::IsValidSharedActiveSize,
                "Shared TSTORE source size must be 128 B..256 KB (SizeCode=1..12)");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TLSU TSTORE, %D[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    "B.IOS %S[Shared], mask=1111\n"
    "B.IOR [%[d0],%[GmStride]], []\n"
    :
    : [d0] "r"(dst.data()), [Shared] "Sr"(src.handle()),
      [SrcType] "i"(type_traits<typename LocalType::DType>::TypeCode),
      [VCOL] "r"(valid_col), [VROW] "r"(valid_row),
      [COL] "i"(LocalType::Cols),
      [GmStride] "r"(dst.GetStrideBytes(3))
    : "memory");
}

// TSTORE.SPART: Shared Tile -> GM on an explicit nonzero PE subset
// (PTO ISA 0.58.3 TLSU Function 14). Exactly one source B.IOS with the caller's
// PE mask (any nonzero subset), no B.IOT; B.IOR carries base + stride.
template <int PEMask = 15, is_global_data_v gm_shape, is_shared_tile_v SharedTileT>
PTO_SHARED_INLINE void TSTORE_PART(gm_shape &dst, const SharedTileT &src) {
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "TSTORE.SPART PEMask must be one of 1,2,4,8,12,14,15");
  using LocalType = typename SharedTileT::LocalTileType;
  static_assert(std::is_same_v<typename LocalType::DType,
                               typename gm_shape::DType>,
                "Shared TSTORE.SPART requires matching GM and Shared dtypes");
  static_assert(LocalType::isRowMajor && !LocalType::isBoxedLayout,
                "Shared TSTORE.SPART supports NORM/RowMajor Local sources only "
                "(no B.DATR Layout is emitted)");
  static_assert(tile_type_traits<typename LocalType::TileDType>::IsValidSharedActiveSize,
                "Shared TSTORE.SPART source size must be 128 B..256 KB (SizeCode=1..12)");
  const size_t valid_col = src.GetValidCol();
  const size_t valid_row = src.GetValidRow();
  asm volatile(
    "BSTART.TLSU TSTORE.SPART, %D[SrcType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[COL], ->lb2\n"
    PTO_PE_MASK_ASM("B.IOS %S[Shared], mask=", "\n")
    "B.IOR [%[d0],%[GmStride]], []\n"
    :
    : [d0] "r"(dst.data()), [Shared] "Sr"(src.handle()),
      [PEMask] "i"(PEMask),
      [SrcType] "i"(type_traits<typename LocalType::DType>::TypeCode),
      [VCOL] "r"(valid_col), [VROW] "r"(valid_row),
      [COL] "i"(LocalType::Cols),
      [GmStride] "r"(dst.GetStrideBytes(3))
    : "memory");
}

// TPREFETCH: request GM lines into cache without a Tile destination (PTO
// 0.58.1 TLSU function 3). Implicit PE
// participation 1111, no B.IOT/B.IOS members. Omitted LB0/LB1 default to one
// and omitted LB2 to the resolved ValidCol; we pass the caller's valid shape
// and the GM row length (logical elements) through B.DIM, and the GM base +
// logical row stride through B.IOR.
template <is_global_data_v gm_shape>
void TPREFETCH(const gm_shape &src, uint32_t valid_col, uint32_t valid_row) {
  const size_t rowStride = src.GetStride(3);
  const size_t physicalCol =
      gm_shape::Cols == DYNAMIC ? rowStride : gm_shape::Cols;
  asm volatile(
    "BSTART.TLSU TPREFETCH, %D[DataType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM %[Col], 0, ->lb2\n"
    "B.IOR [%[Base], %[Stride]], []\n"
    :
    : [Base] "r"(src.data()), [Stride] "r"(rowStride),
      [DataType] "i"(type_traits<typename gm_shape::DType>::TypeCode),
      [VCOL] "r"(valid_col), [VROW] "r"(valid_row),
      [Col] "r"(physicalCol)
    : "memory");
}

// MGATHER_CAS: atomic compare-and-swap at byte displacements (PTO ISA 0.58.3
// TLSU function 8; canonical BSTART.MGATHER.CAS). Exactly two Local B.IOT
// bindings: IndexTile+ExpectedTile (TwoSrc_NoDst, no destination, L=0) then
// ReplacementTile+last ->DstTile (L=1); B.IOR carries only the byte-address
// base. Each lane atomically reads BaseGPR+displacement, compares with
// Expected, stores Replacement on match, and publishes the observed old value
// to the destination. The destination must be an early-clobbered output so
// the allocator keeps it distinct from the replacement source.
template <is_tile_data_v DstTile, is_tile_data_v IndexTile,
          is_tile_data_v ExpectedTile, is_tile_data_v ReplacementTile>
void MGATHER_CAS(DstTile &observedOld, uint64_t base,
                 IndexTile &byteDisplacements, ExpectedTile &expected,
                 ReplacementTile &replacement, uint32_t validCol,
                 uint32_t validRow = 1) {
  static_assert(std::is_same_v<typename ExpectedTile::DType,
                               typename ReplacementTile::DType> &&
                    std::is_same_v<typename ExpectedTile::DType,
                                   typename DstTile::DType>,
                "MGATHER_CAS expected/replacement/dst must share one transfer "
                "DataType");
  constexpr int IndexType = type_traits<typename IndexTile::DType>::TypeCode;
  constexpr int TransferType = type_traits<typename DstTile::DType>::TypeCode;
  static_assert(
      IndexType == __type_int4x2 || IndexType == __type_uint4x2 ||
          IndexType == __type_int8 || IndexType == __type_uint8 ||
          IndexType == __type_int16 || IndexType == __type_uint16 ||
          IndexType == __type_int32 || IndexType == __type_uint32 ||
          IndexType == __type_int64 || IndexType == __type_uint64,
      "MGATHER_CAS index tile must use an S/U 4X2, 8, 16, 32, or 64-bit "
      "integer byte-displacement type");
  static_assert(
      TransferType != __type_fp4_e2m1x2 &&
          TransferType != __type_fp4_e1m2x2 &&
          TransferType != __type_fp4_hif4x2 &&
          TransferType != __type_int4x2 && TransferType != __type_uint4x2,
      "MGATHER_CAS transfer DataType must not be a packed four-bit type");
  static_assert(IndexTile::Rows == ExpectedTile::Rows &&
                    IndexTile::Cols == ExpectedTile::Cols &&
                    DstTile::Rows == ExpectedTile::Rows &&
                    DstTile::Cols == ExpectedTile::Cols,
                "MGATHER_CAS tiles must match the resolved ValidRow x ValidCol");
  asm volatile(
    "BSTART.TLSU MGATHER.CAS, %D[DataType]\n"
    "B.DIM %[VCOL], 0, ->lb0\n"
    "B.DIM %[VROW], 0, ->lb1\n"
    "B.DIM zero, %c[Col], ->lb2\n"
    "B.IOT %[Idx], %[Exp], mask=1111\n"
    "B.IOT %[Rep], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
    "B.IOR [%[Base]], []\n"
    : [Dst] "=&Tr"(observedOld.data())
    : [Idx] "Tr"(byteDisplacements.data()), [Exp] "Tr"(expected.data()),
      [Rep] "Tr"(replacement.data()),
      [Base] "r"(base),
      [DataType] "i"(type_traits<typename DstTile::DType>::TypeCode),
      [VCOL] "r"(validCol), [VROW] "r"(validRow),
      [Col] "i"(DstTile::Cols),
      [DstSize] "i"(DstTile::TilesizeCode)
    : "memory");
}

// Low-level v5 GMOV. All four PEs must reach the same dynamic instance;
// PEMask only selects requesters and does not reduce the Core4 collective.
template <int PEMask = 15, is_tile_data_v tile_shape_dst,
          is_tile_data_v tile_shape_src>
void GMOV(tile_shape_dst &dst, uint64_t peer_tid, const tile_shape_src &src) {
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "GMOV PEMask must be one of 1,2,4,8,12,14,15");
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
      "GMOV logical Tile size must be 128 B..256 KiB (SizeCode=1..12)");
  static_assert(tile_shape_dst::LogicalTileBytes ==
                    tile_shape_src::LogicalTileBytes,
                "GMOV source and destination logical sizes must match");
  asm volatile(
      "BSTART.TLSU GMOV, %D[DataType]\n"
      PTO_PE_MASK_ASM("B.IOT %[src], mask=",
                      ", last, ->%[dst]<%Z[TileSize]>\n")
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
// (PTO_SHARED_INLINE is defined at the top of this header once.)
template <int PEMask = 15, is_tile_data_v tile_shape_src>
PTO_SHARED_INLINE void
TMOV_L2S_INSERT(SharedTile<tile_shape_src> &dst,
                const tile_shape_src &src) {
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "PEMask must be one of 1,2,4,8,12,14,15");
  static_assert(
      tile_type_traits<typename tile_shape_src::TileDType>::IsValidSharedActiveSize,
      "TMOV.L2S.INSERT logical Tile size must be 128 B..256 KB (SizeCode=1..12)");
  dst.SetValidShape(src);
  asm volatile(
      "BSTART.TLSU TMOV.L2S.INSERT, %D[DataType]\n"
      PTO_PE_MASK_ASM("B.IOS mask=", ", ->%S[Shared]<%Z[TileSize]>\n")
      PTO_PE_MASK_ASM("B.IOT %[src], mask=", ", last\n")
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
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "PEMask must be one of 1,2,4,8,12,14,15");
  static_assert(
      tile_type_traits<typename tile_shape_src::TileDType>::IsValidSharedActiveSize,
      "TMOV.L2S.PUBLISH logical Tile size must be 128 B..256 KB (SizeCode=1..12)");
  dst.SetValidShape(src);
  asm volatile(
      "BSTART.TLSU TMOV.L2S.PUBLISH, %D[DataType]\n"
      PTO_PE_MASK_ASM("B.IOS mask=", ", ->%S[Shared]<%Z[TileSize]>\n")
      PTO_PE_MASK_ASM("B.IOT %[src], mask=", ", last\n")
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
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "PEMask must be one of 1,2,4,8,12,14,15");
  static_assert(
      tile_type_traits<typename tile_shape_dst::TileDType>::IsValidActiveSize,
      "TMOV.S2L.BROADCAST logical Tile size must be 128 B..256 KiB (SizeCode=1..12)");
  asm volatile(
      "BSTART.TLSU TMOV.S2L.BROADCAST, %D[DataType]\n"
      PTO_PE_MASK_ASM("B.IOS %S[Shared], mask=", "\n")
      PTO_PE_MASK_ASM("B.IOT mask=", ", last, ->%[dst]<%Z[TileSize]>\n")
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
  static_assert(is_valid_pe_mask(PEMask) && PEMask != 0,
                "PEMask must be one of 1,2,4,8,12,14,15");
  static_assert(
      tile_type_traits<typename tile_shape_dst::TileDType>::IsValidActiveSize,
      "TMOV.S2L.EXTRACT logical Tile size must be 128 B..256 KiB (SizeCode=1..12)");
  asm volatile(
      "BSTART.TLSU TMOV.S2L.EXTRACT, %D[DataType]\n"
      PTO_PE_MASK_ASM("B.IOS %S[Shared], mask=", "\n")
      PTO_PE_MASK_ASM("B.IOT mask=", ", last, ->%[dst]<%Z[TileSize]>\n")
      : [dst] "=Tr"(dst.data())
      : [Shared] "Sr"(shared.handle()), [PEMask] "i"(PEMask),
        [DataType] "i"(type_traits<typename tile_shape_dst::DType>::TypeCode),
        [TileSize] "i"(
            tile_type_traits<typename tile_shape_dst::TileDType>::TilesizeCode)
      : "memory");
}

// ACCCVT was removed from PTO-ISA v0.58. Post-processed matrix operations
// write an ordinary Tile directly and are the supported replacement.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void ACCCVT(tile_shape_out &, tile_shape_in &) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "ACCCVT was removed from DavinciOO v5 and cannot export the "
                "implicit ACC; use the active TMATMUL operation with B.FPATR");
}

namespace pto_matmul_detail {

#define PTO_MATMUL_HEADER(OPCODE, EXTRA_ATTRS)                                  \
  "BSTART.CUBE " OPCODE ", %D[DataTypeA]\n"                                      \
  "B.DATR %D[DataTypeB], RNONE, NOSAT\n" EXTRA_ATTRS                     \
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

// PTO_FIXP_ATTR / PTO_FIXP_ATTR_INPUTS emit the B.FPATR line and its ten
// immediate operands. Every TMATMUL/TMATMULMX CUBE bundle carries exactly one
// B.FPATR after B.DATR, so these are shared by the whole family, not just the
// .FIXP variants. The macros reference the template parameter Attr, so the
// helper templates below take FixpAttr Attr as an NTTP. Defined here (before
// any helper that uses them) so the plain matmul free function can also use
// PTO_FIXP_ATTR.
// PTO-ISA v0.58 B.FPATR fields:
//   PreQuantMode(6b@26) ReluMode(3b@23) GroupNCode(4b@19, <=9)
//   RowMaxEn(1b@18) GroupMaxEn(1b@17) RowMaxInit(1b@16) MaxAbsEn(1b@15)
//   TransB(1b@8) TransA(1b@7) CScaleEn(1b@9), with bit10 reserved zero.
//   Transpose applies only when the corresponding primary matrix operand is
//   cooperative Shared storage. CScaleEn is an attribute bit; the CScale
//   operand/binder is a separate follow-up API.
#define PTO_FIXP_ATTR \
  "B.FPATR %c[PreQuant], %c[ReluMode], %c[GroupNCode], %c[RowMaxEn], " \
  "%c[GroupMaxEn], %c[RowMaxInit], %c[MaxAbsEn], %c[TransA], %c[TransB], " \
  "%c[CScaleEn]\n"

#define PTO_FIXP_ATTR_INPUTS \
  [PreQuant] "i"(static_cast<uint8_t>(Attr.PreQuant)), \
  [ReluMode] "i"(static_cast<uint8_t>(Attr.Relu)), \
  [GroupNCode] "i"(Attr.GroupNCode), [RowMaxEn] "i"(Attr.RowMaxEn), \
  [GroupMaxEn] "i"(Attr.GroupMaxEn), \
  [RowMaxInit] "i"(Attr.RowMaxInit), [MaxAbsEn] "i"(Attr.MaxAbsEn), \
  [TransA] "i"(Attr.TransA), [TransB] "i"(Attr.TransB), \
  [CScaleEn] "i"(Attr.CScaleEn)

#define PTO_MX_SCALE_INPUTS \
  [HasScaleA] "i"(HasScaleA), [HasScaleB] "i"(HasScaleB)

template <typename T>
inline constexpr bool is_cube_m_layout_v =
    T::BFractal == BLayout::CubeM16 || T::BFractal == BLayout::CubeM32;

constexpr int cooperative_group_m_rows_per_pe(int group_m) {
  return group_m <= 64 ? 16 : 32;
}

template <FixpAttr Attr, typename Dst, typename A, typename B, bool MX = false>
constexpr void validate_matrix_contract() {
  static_assert(matrix_input_pair_legal<A, B, MX>(),
                "Matrix A/B dtypes must be supported inputs from one numeric class");
  static_assert(matrix_accumulator_mode_legal<Attr, A, B, MX>(),
                "B.FPATR PreQuantMode is incompatible with the derived matrix accumulator type");
  static_assert(matrix_output_type_legal<Attr, A, B, Dst, MX>(),
                "Matrix D dtype must match the derived accumulator/output type");
  static_assert(Dst::IsCubeLayout && is_cube_m_layout_v<Dst>,
                "CUBE destination D must use CUBE_M16 or CUBE_M32 CELL layout");
  static_assert(!is_shared_tile_v<A> || is_shared_tile_v<B>,
                "Shared matmul A requires B to be Shared as well; a lone "
                "B.IOS binder denotes the existing Shared-Right form");
  if constexpr (!is_shared_tile_v<A>) {
    static_assert(A::IsCubeLayout && is_cube_m_layout_v<A>,
                  "Local matrix A must use CUBE_M16 or CUBE_M32 CELL layout");
    static_assert(A::BFractal == Dst::BFractal,
                  "Local matrix A and destination D must use the same CUBE_M layout");
  } else {
    static_assert(A::BFractal == BLayout::RowMajor &&
                      A::SFractal == SLayout::NoneBox,
                  "Shared matrix A must be an ordinary RowMajor rectangle");
  }
  if constexpr (!is_shared_tile_v<B>) {
    static_assert(B::IsCubeLayout && B::BFractal == BLayout::CubeN8,
                  "Local matrix B must use CUBE_N8 CELL layout");
  } else {
    static_assert(B::BFractal == BLayout::RowMajor &&
                      B::SFractal == SLayout::NoneBox,
                  "Shared matrix B must be an ordinary RowMajor rectangle");
  }
  static_assert(!Attr.TransA || is_shared_tile_v<A>,
                "B.FPATR TransA requires a cooperative Shared A primary");
  static_assert(!Attr.TransB || is_shared_tile_v<B>,
                "B.FPATR TransB requires a cooperative Shared B primary");
  static_assert(A::ValidRow != DYNAMIC && A::ValidCol != DYNAMIC &&
                    B::ValidRow != DYNAMIC && B::ValidCol != DYNAMIC &&
                    Dst::ValidRow != DYNAMIC && Dst::ValidCol != DYNAMIC,
                "Matrix dynamic valid shapes are not supported");
  constexpr int AValidRows = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidCol : A::ValidRow;
  constexpr int AValidCols = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidRow : A::ValidCol;
  constexpr int BValidRows = is_shared_tile_v<B> && Attr.TransB
      ? B::ValidCol : B::ValidRow;
  constexpr int BValidCols = is_shared_tile_v<B> && Attr.TransB
      ? B::ValidRow : B::ValidCol;
  static_assert(AValidCols == BValidRows,
                "Matrix effective valid K dimensions must match");
  if constexpr (is_shared_tile_v<A> && is_shared_tile_v<B>) {
    static_assert(AValidRows >= 1 && AValidRows <= 128,
                  "Cooperative Matrix group_M must be in the range 1..128");
    constexpr int RowsPerPE = cooperative_group_m_rows_per_pe(AValidRows);
    static_assert(Dst::ValidRow == RowsPerPE &&
                      Dst::ValidCol == BValidCols,
                  "Cooperative Matrix D valid shape must match the per-PE "
                  "M block (16 rows for group_M<=64, otherwise 32) x N");
  } else {
    static_assert(Dst::ValidRow == AValidRows &&
                      Dst::ValidCol == BValidCols,
                  "Matrix D valid shape must match effective M x N");
  }
}

template <FixpAttr Attr, typename Dst, typename Acc, typename A, typename B,
          bool MX = false>
constexpr void validate_matrix_accumulator_contract() {
  static_assert(matrix_accumulator_type_legal<A, B, Acc, MX>(),
                "Matrix accumulator C dtype must match the derived accumulator type");
  static_assert(Acc::IsCubeLayout && is_cube_m_layout_v<Acc>,
                "Matrix accumulator C must use CUBE_M16 or CUBE_M32 CELL layout");
  static_assert(Acc::BFractal == Dst::BFractal,
                "Matrix accumulator C and destination D must use the same CUBE_M layout");
  if constexpr (Acc::ValidRow != DYNAMIC && Dst::ValidRow != DYNAMIC)
    static_assert(Acc::ValidRow == Dst::ValidRow,
                  "Matrix accumulator C and destination D valid rows must match");
  if constexpr (Acc::ValidCol != DYNAMIC && Dst::ValidCol != DYNAMIC)
    static_assert(Acc::ValidCol == Dst::ValidCol,
                  "Matrix accumulator C and destination D valid columns must match");
  if constexpr (Attr.PreQuant == FixpPreQuantMode::None)
    static_assert(Acc::LogicalTileBytes == Dst::LogicalTileBytes,
                  "Unconverted matrix C and D capacities must match");
}

template <FixpAttr Attr, typename Acc, typename CScale>
constexpr void validate_cscale_contract() {
  if constexpr (Attr.CScaleEn) {
    static_assert(type_traits<typename Acc::DType>::TypeCode == __type_fp32,
                  "CScaleEn requires an FP32 accumulator C");
    static_assert(!is_shared_tile_v<CScale>,
                  "CScale must be a Local mathematical source");
    static_assert(type_traits<typename CScale::DType>::TypeCode == __type_uint8,
                  "CScale dtype must be U8");
    static_assert(CScale::IsCubeLayout &&
                      CScale::BFractal == BLayout::CubeM32,
                  "CScale must use the CUBE_M32 CELL layout");
    static_assert(CScale::ValidRow == Acc::ValidRow &&
                      CScale::ValidCol == 1,
                  "CScale valid shape must be M x 1");
  }
}

template <FixpAttr Attr, typename Bias, typename A, typename B, bool MX = false>
constexpr void validate_matrix_bias_contract() {
  static_assert(matrix_accumulator_type_legal<A, B, Bias, MX>(),
                "Matrix Bias dtype must match the derived accumulator type");
  static_assert(Bias::BFractal == BLayout::RowMajor &&
                    Bias::SFractal == SLayout::NoneBox,
                "Matrix Bias must use ordinary RowMajor layout");
  static_assert(Bias::ValidRow != DYNAMIC && Bias::ValidCol != DYNAMIC &&
                    B::ValidRow != DYNAMIC && B::ValidCol != DYNAMIC,
                "Matrix Bias dynamic valid shapes are not supported");
  constexpr int N = is_shared_tile_v<B> && Attr.TransB
      ? B::ValidRow : B::ValidCol;
  static_assert(Bias::ValidRow == 1 && Bias::ValidCol == N,
                "Matrix Bias valid shape must be 1 x N");
}

template <FixpAttr Attr, bool HasScaleA, bool HasScaleB, typename ScaleA,
          typename A, typename ScaleB, typename B>
constexpr void validate_matrix_scale_contract() {
  constexpr int ACode = type_traits<typename A::DType>::TypeCode;
  constexpr int BCode = type_traits<typename B::DType>::TypeCode;
  static_assert(HasScaleA == matrix_mx_input_needs_scale(ACode),
                "ScaleA presence must match the PTO MX type contract");
  static_assert(HasScaleB == matrix_mx_input_needs_scale(BCode),
                "ScaleB presence must match the PTO MX type contract");
  static_assert(A::ValidRow != DYNAMIC && A::ValidCol != DYNAMIC &&
                    B::ValidCol != DYNAMIC,
                "MX dynamic primary valid shapes are not supported");
  constexpr int M = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidCol : A::ValidRow;
  constexpr int K = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidRow : A::ValidCol;
  constexpr int N = is_shared_tile_v<B> && Attr.TransB
      ? B::ValidRow : B::ValidCol;
  constexpr int KBlocks = (K + 31) / 32;
  if constexpr (HasScaleA) {
    static_assert(type_traits<typename ScaleA::DType>::TypeCode == __type_fp8_e8m0,
                  "MX ScaleA dtype must be E8M0");
    static_assert(ScaleA::BFractal == BLayout::RowMajor &&
                      ScaleA::SFractal == SLayout::NoneBox,
                  "MX ScaleA must use ordinary RowMajor layout");
    static_assert(is_shared_tile_v<ScaleA> == is_shared_tile_v<A>,
                  "MX ScaleA storage must match A storage");
    static_assert(ScaleA::ValidRow == M && ScaleA::ValidCol == KBlocks,
                  "MX ScaleA valid shape must be M x ceil(K/32)");
  }
  if constexpr (HasScaleB) {
    static_assert(type_traits<typename ScaleB::DType>::TypeCode == __type_fp8_e8m0,
                  "MX ScaleB dtype must be E8M0");
    static_assert(ScaleB::BFractal == BLayout::RowMajor &&
                      ScaleB::SFractal == SLayout::NoneBox,
                  "MX ScaleB must use ordinary RowMajor layout");
    static_assert(is_shared_tile_v<ScaleB> == is_shared_tile_v<B>,
                  "MX ScaleB storage must match B storage");
    static_assert(ScaleB::ValidRow == KBlocks && ScaleB::ValidCol == N,
                  "MX ScaleB valid shape must be ceil(K/32) x N");
  }
}

template <FixpAttr Attr, int SrcMask, int OutMask, typename A, typename B,
          typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut, bool MX = false>
constexpr void validate_matrix_postprocess_contract() {
  constexpr int AccCode = MX
      ? __type_fp32
      : matrix_accumulator_type_code(
            type_traits<typename A::DType>::TypeCode);
  constexpr int M = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidCol : A::ValidRow;
  constexpr int N = is_shared_tile_v<B> && Attr.TransB
      ? B::ValidRow : B::ValidCol;
  if constexpr ((OutMask & 1) != 0) {
    static_assert(type_traits<typename RowOut::DType>::TypeCode == AccCode,
                  "RowMaxOut dtype must match the derived accumulator type");
    static_assert(RowOut::ValidRow == M && RowOut::ValidCol == 1,
                  "RowMaxOut valid shape must be M x 1");
  }
  if constexpr ((SrcMask & 1) != 0) {
    static_assert(type_traits<typename RowIn::DType>::TypeCode == AccCode,
                  "RowMaxIn dtype must match the derived accumulator type");
    static_assert(RowIn::ValidRow == M && RowIn::ValidCol == 1,
                  "RowMaxIn valid shape must be M x 1");
  }
  if constexpr ((OutMask & 2) != 0) {
    constexpr int GroupN = fixp::group_n_from_code(Attr.GroupNCode);
    static_assert(type_traits<typename GroupOut::DType>::TypeCode == AccCode,
                  "GroupMaxOut dtype must match the derived accumulator type");
    static_assert(GroupOut::ValidRow == M &&
                      GroupOut::ValidCol == (N + GroupN - 1) / GroupN,
                  "GroupMaxOut valid shape must be M x ceil(N/GroupN)");
  }
  if constexpr ((SrcMask & 2) != 0) {
    static_assert(type_traits<typename QuantTile::DType>::TypeCode == __type_uint64,
                  "Vector quant parameter dtype must be U64");
    static_assert(QuantTile::ValidRow == 1 && QuantTile::ValidCol == N,
                  "Vector quant parameter valid shape must be 1 x N");
  }
  if constexpr ((SrcMask & 4) != 0) {
    static_assert(type_traits<typename ReluTile::DType>::TypeCode == __type_uint64,
                  "Vector PReLU parameter dtype must be U64");
    static_assert(ReluTile::ValidRow == 1 && ReluTile::ValidCol == N,
                  "Vector PReLU parameter valid shape must be 1 x N");
  }
}

template <FixpAttr Attr, typename Dst, typename Vec, typename Mtx,
          bool MX = false>
constexpr void validate_gemv_contract() {
  validate_matrix_contract<Attr, Dst, Vec, Mtx, MX>();
  static_assert(Vec::ValidRow != DYNAMIC && Vec::ValidCol != DYNAMIC &&
                    Mtx::ValidRow != DYNAMIC && Mtx::ValidCol != DYNAMIC &&
                    Dst::ValidRow != DYNAMIC && Dst::ValidCol != DYNAMIC,
                "TGEMV dynamic valid shapes are not supported");
  static_assert(Vec::ValidRow == 1 && Dst::ValidRow == 1,
                "TGEMV requires Vec.ValidRow and D.ValidRow to equal one");
  static_assert(Vec::ValidCol == Mtx::ValidRow,
                "TGEMV valid K mismatch: Vec.ValidCol must equal Mtx.ValidRow");
  static_assert(Dst::ValidCol == Mtx::ValidCol,
                "TGEMV D.ValidCol must equal Mtx.ValidCol");
}

// These helpers centralize the encoded M/N/K decision so basic/ACC/BIAS/MX
// and options overloads cannot diverge. LB0/M always follows the effective
// row count of input A, for both Local and cooperative Shared A/B operands.
struct MatmulShape {
  size_t M;
  size_t N;
  size_t K;
  bool group;
};

template <FixpAttr Attr, typename C, typename A, typename B>
constexpr MatmulShape resolve_matmul_shape() {
  static_assert(A::ValidRow != DYNAMIC && A::ValidCol != DYNAMIC &&
                    B::ValidRow != DYNAMIC && B::ValidCol != DYNAMIC &&
                    C::ValidRow != DYNAMIC && C::ValidCol != DYNAMIC,
                "Matrix dynamic valid shapes are not supported");
  constexpr bool IsGroup = is_shared_tile_v<A> && is_shared_tile_v<B>;
  constexpr size_t M = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidCol : A::ValidRow;
  constexpr size_t K = is_shared_tile_v<A> && Attr.TransA
      ? A::ValidRow : A::ValidCol;
  constexpr size_t N = is_shared_tile_v<B> && Attr.TransB
      ? B::ValidRow : B::ValidCol;
  return MatmulShape{M, N, K, IsGroup};
}

// The public Matrix surface rejects dynamic valid shapes, so resolution is
// compile-time exact even though the values are passed through GPR operands.
template <FixpAttr Attr, typename C, typename A, typename B>
inline MatmulShape resolve_matmul_shape_runtime(const C &c, const A &a,
                                                const B &b) {
  (void)c;
  (void)a;
  (void)b;
  return resolve_matmul_shape<Attr, C, A, B>();
}

template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename B>
PTO_SHARED_INLINE void matmul(Dst &dst, A &a, B &b, size_t M, size_t N,
                              size_t K) {
  validate_matrix_contract<Attr, Dst, A, B>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOT %[A], %[B], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [A] "Tr"(a.data()), [B] "Tr"(b.data()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOS %S[SharedA], mask=1111\n"
        "B.IOT %[B], mask=1111\n"
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)
        "B.IOS %S[SharedB], mask=1111\n"
        "B.IOT %[A], mask=1111\n"
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
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
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
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
  validate_matrix_contract<Attr, Dst, A, B>();                                            \
  validate_matrix_bias_contract<Attr, Extra, A, B>();                                           \
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {                 \
    asm volatile(                                                                \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                                \
        "B.IOT %[A], %[B], mask=1111\n"                                         \
        "B.IOT %[Extra], mask=1111\n"                                                    \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                       \
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
        "B.IOT %[B], mask=1111\n"                                                       \
        "B.IOT %[Extra], mask=1111\n"                                                    \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                       \
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
        "B.IOT %[A], mask=1111\n"                                                       \
        "B.IOT %[Extra], mask=1111\n"                                                    \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                       \
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
        "B.IOT %[Extra], mask=1111\n"                                                    \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                       \
        : [Dst] "=&Tr"(dst.data())                                             \
        : [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()),             \
          [Extra] "Tr"(extra.data()),                                          \
          PTO_FIXP_ATTR_INPUTS,                                                 \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                          \
        : "memory");                                                           \
  }                                                                              \
}

PTO_DEFINE_MATMUL_3SRC_HELPER(matmul_bias, "TMATMUL.BIAS")

template <FixpAttr Attr = FixpAttr{}, typename Dst, typename C, typename A,
          typename B>
PTO_SHARED_INLINE void matmul_acc(Dst &dst, C &c, A &a, B &b, size_t M,
                                  size_t N, size_t K) {
  validate_matrix_contract<Attr, Dst, A, B>();
  validate_matrix_accumulator_contract<Attr, Dst, C, A, B>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)
        "B.IOT %[C], mask=1111\n"
        "B.IOT %[A], %[B], mask=1111\n"
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [C] "Tr"(c.data()), [A] "Tr"(a.data()), [B] "Tr"(b.data()),
          PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)
        "B.IOT %[C], mask=1111\n"
        "B.IOS %S[SharedA], mask=1111\n"
        "B.IOT %[B], mask=1111\n"
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [C] "Tr"(c.data()), [SharedA] "Sr"(a.handle()),
          [B] "Tr"(b.data()), PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)
        "B.IOT %[C], mask=1111\n"
        "B.IOT %[A], mask=1111\n"
        "B.IOS %S[SharedB], mask=1111\n"
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [C] "Tr"(c.data()), [A] "Tr"(a.data()),
          [SharedB] "Sr"(b.handle()), PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  } else {
    asm volatile(
        PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)
        "B.IOT %[C], mask=1111\n"
        "B.IOS %S[SharedA], mask=1111\n"
        "B.IOS %S[SharedB], mask=1111\n"
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
        : [Dst] "=&Tr"(dst.data())
        : [C] "Tr"(c.data()), [SharedA] "Sr"(a.handle()),
          [SharedB] "Sr"(b.handle()), PTO_FIXP_ATTR_INPUTS,
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
        : "memory");
  }
}


// Shared PostProcess auxiliary-source lines (RowMaxIn / QuantTile / ReluTile).
// These are appended after the operation's mathematical sources and are shared
// by the TMATMUL / TMATMUL_ACC / TMATMUL_BIAS / TMATMUL_MX emitters so the
// PostProcess masking logic is written once. The math-source-specific macros
// (PTO_FIXP_SRC_*, PTO_FIXP_ACC_SRC_*, ...) place the math operand lines
// first and then reference the matching PTO_FIXP_PPSRC_<mask>.
#define PTO_FIXP_PPSRC_0
#define PTO_FIXP_PPSRC_1 \
  "B.IOT %[RowIn], mask=1111\n"
#define PTO_FIXP_PPSRC_2 \
  "B.IOT %[QuantTile], mask=1111\n"
#define PTO_FIXP_PPSRC_3 \
  "B.IOT %[RowIn], %[QuantTile], mask=1111\n"
#define PTO_FIXP_PPSRC_4 \
  "B.IOT %[ReluTile], mask=1111\n"
#define PTO_FIXP_PPSRC_5 \
  "B.IOT %[RowIn], %[ReluTile], mask=1111\n"
#define PTO_FIXP_PPSRC_6 \
  "B.IOT %[QuantTile], %[ReluTile], mask=1111\n"
#define PTO_FIXP_PPSRC_7 \
  "B.IOT %[RowIn], %[QuantTile], mask=1111\n" \
  "B.IOT %[ReluTile], mask=1111\n"

#define PTO_FIXP_ACC_CSCALE \
  ".if %c[CScaleEn]\n" \
  "B.IOT %[CScaleOperand], mask=1111\n" \
  ".endif\n"

#define PTO_FIXP_ACC_PPSRC_0 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_0
#define PTO_FIXP_ACC_PPSRC_1 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_1
#define PTO_FIXP_ACC_PPSRC_2 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_2
#define PTO_FIXP_ACC_PPSRC_3 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_3
#define PTO_FIXP_ACC_PPSRC_4 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_4
#define PTO_FIXP_ACC_PPSRC_5 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_5
#define PTO_FIXP_ACC_PPSRC_6 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_6
#define PTO_FIXP_ACC_PPSRC_7 PTO_FIXP_ACC_CSCALE PTO_FIXP_PPSRC_7

#define PTO_FIXP_ACC_L_SRC_0 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_ACC_L_SRC_1 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_ACC_L_SRC_2 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_ACC_L_SRC_3 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_ACC_L_SRC_4 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_ACC_L_SRC_5 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_ACC_L_SRC_6 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_ACC_L_SRC_7 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[A], %[B], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_ACC_SB_SRC_0 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_ACC_SB_SRC_1 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_ACC_SB_SRC_2 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_ACC_SB_SRC_3 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_ACC_SB_SRC_4 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_ACC_SB_SRC_5 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_ACC_SB_SRC_6 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_ACC_SB_SRC_7 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_ACC_SA_SRC_0 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_ACC_SA_SRC_1 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_ACC_SA_SRC_2 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_ACC_SA_SRC_3 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_ACC_SA_SRC_4 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_ACC_SA_SRC_5 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_ACC_SA_SRC_6 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_ACC_SA_SRC_7 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_ACC_SAB_SRC_0 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_ACC_SAB_SRC_1 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_ACC_SAB_SRC_2 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_ACC_SAB_SRC_3 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_ACC_SAB_SRC_4 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_ACC_SAB_SRC_5 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_ACC_SAB_SRC_6 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_ACC_SAB_SRC_7 \
  "B.IOT %[C], mask=1111\n" "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_BIAS_L_SRC_0 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_BIAS_L_SRC_1 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_BIAS_L_SRC_2 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_BIAS_L_SRC_3 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_BIAS_L_SRC_4 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_BIAS_L_SRC_5 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_BIAS_L_SRC_6 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_BIAS_L_SRC_7 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_BIAS_SB_SRC_0 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_BIAS_SB_SRC_1 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_BIAS_SB_SRC_2 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_BIAS_SB_SRC_3 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_BIAS_SB_SRC_4 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_BIAS_SB_SRC_5 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_BIAS_SB_SRC_6 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_BIAS_SB_SRC_7 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_BIAS_SA_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_BIAS_SA_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_BIAS_SA_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_BIAS_SA_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_BIAS_SA_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_BIAS_SA_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_BIAS_SA_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_BIAS_SA_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B]\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_BIAS_SAB_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_BIAS_SAB_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_BIAS_SAB_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_BIAS_SAB_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_BIAS_SAB_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_BIAS_SAB_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_BIAS_SAB_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_BIAS_SAB_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_L_SRC_0 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_L_SRC_1 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_L_SRC_2 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_L_SRC_3 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_L_SRC_4 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_L_SRC_5 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_L_SRC_6 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_L_SRC_7 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_SB_SRC_0 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_SB_SRC_1 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_SB_SRC_2 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_SB_SRC_3 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_SB_SRC_4 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_SB_SRC_5 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_SB_SRC_6 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_SB_SRC_7 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_SA_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_SA_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_SA_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_SA_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_SA_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_SA_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_SA_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_SA_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_SAB_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_SAB_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_SAB_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_SAB_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_SAB_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_SAB_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_SAB_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_SAB_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_7

// CScale-aware MX ACC source streams keep the CScale source after all matrix and MX scale sources.
#define PTO_FIXP_MX_ACC_L_SRC_0 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_MX_ACC_L_SRC_1 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_MX_ACC_L_SRC_2 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_MX_ACC_L_SRC_3 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_MX_ACC_L_SRC_4 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_MX_ACC_L_SRC_5 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_MX_ACC_L_SRC_6 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_MX_ACC_L_SRC_7 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_MX_ACC_SB_SRC_0 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_MX_ACC_SB_SRC_1 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_MX_ACC_SB_SRC_2 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_MX_ACC_SB_SRC_3 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_MX_ACC_SB_SRC_4 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_MX_ACC_SB_SRC_5 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_MX_ACC_SB_SRC_6 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_MX_ACC_SB_SRC_7 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_MX_ACC_SA_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_MX_ACC_SA_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_MX_ACC_SA_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_MX_ACC_SA_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_MX_ACC_SA_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_MX_ACC_SA_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_MX_ACC_SA_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_MX_ACC_SA_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_7

#define PTO_FIXP_MX_ACC_SAB_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_0

#define PTO_FIXP_MX_ACC_SAB_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_1

#define PTO_FIXP_MX_ACC_SAB_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_2

#define PTO_FIXP_MX_ACC_SAB_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_3

#define PTO_FIXP_MX_ACC_SAB_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_4

#define PTO_FIXP_MX_ACC_SAB_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_5

#define PTO_FIXP_MX_ACC_SAB_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_6

#define PTO_FIXP_MX_ACC_SAB_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" \
  PTO_FIXP_ACC_PPSRC_7


#define PTO_FIXP_MX_BIAS_L_SRC_0 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_BIAS_L_SRC_1 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_BIAS_L_SRC_2 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_BIAS_L_SRC_3 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_BIAS_L_SRC_4 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_BIAS_L_SRC_5 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_BIAS_L_SRC_6 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_BIAS_L_SRC_7 \
  "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" \
  "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_BIAS_SB_SRC_0 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_BIAS_SB_SRC_1 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_BIAS_SB_SRC_2 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_BIAS_SB_SRC_3 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_BIAS_SB_SRC_4 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_BIAS_SB_SRC_5 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_BIAS_SB_SRC_6 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_BIAS_SB_SRC_7 \
  "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_BIAS_SA_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_BIAS_SA_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_BIAS_SA_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_BIAS_SA_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_BIAS_SA_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_BIAS_SA_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_BIAS_SA_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_BIAS_SA_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_MX_BIAS_SAB_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_MX_BIAS_SAB_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_MX_BIAS_SAB_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_MX_BIAS_SAB_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_MX_BIAS_SAB_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_MX_BIAS_SAB_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_MX_BIAS_SAB_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_MX_BIAS_SAB_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOS %S[ScaleA], mask=1111\n" ".endif\n" "B.IOS %S[SharedB], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOS %S[ScaleB], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

// TGEMV (Function 16-18, 20-22) math source streams. Local-only per
// handoff Sec 1.5 (any B.IOS is illegal). A=1xK vector (M=1), B=KxN
// matrix, C=1xN output. Scales follow their matrix/vector shape.
#define PTO_FIXP_GV_GV_L_SRC_0 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_GV_GV_L_SRC_1 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_GV_GV_L_SRC_2 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_GV_GV_L_SRC_3 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_GV_GV_L_SRC_4 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_GV_GV_L_SRC_5 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_GV_GV_L_SRC_6 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_GV_GV_L_SRC_7 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_GV_GVB_L_SRC_0 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_GV_GVB_L_SRC_1 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_GV_GVB_L_SRC_2 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_GV_GVB_L_SRC_3 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_GV_GVB_L_SRC_4 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_GV_GVB_L_SRC_5 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_GV_GVB_L_SRC_6 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_GV_GVB_L_SRC_7 \
  "B.IOT %[Vec], %[Mtx], mask=1111\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_GV_GVA_L_SRC_0 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_GV_GVA_L_SRC_1 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_GV_GVA_L_SRC_2 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_GV_GVA_L_SRC_3 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_GV_GVA_L_SRC_4 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_GV_GVA_L_SRC_5 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_GV_GVA_L_SRC_6 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_GV_GVA_L_SRC_7 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], %[Mtx], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_GV_GVMX_L_SRC_0 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_GV_GVMX_L_SRC_1 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_GV_GVMX_L_SRC_2 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_GV_GVMX_L_SRC_3 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_GV_GVMX_L_SRC_4 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_GV_GVMX_L_SRC_5 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_GV_GVMX_L_SRC_6 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_GV_GVMX_L_SRC_7 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_GV_GVMXB_L_SRC_0 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_GV_GVMXB_L_SRC_1 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_GV_GVMXB_L_SRC_2 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_GV_GVMXB_L_SRC_3 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_GV_GVMXB_L_SRC_4 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_GV_GVMXB_L_SRC_5 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_GV_GVMXB_L_SRC_6 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_GV_GVMXB_L_SRC_7 \
  "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" "B.IOT %[Bias], mask=1111\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_GV_GVMXA_L_SRC_0 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_0

#define PTO_FIXP_GV_GVMXA_L_SRC_1 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_1

#define PTO_FIXP_GV_GVMXA_L_SRC_2 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_2

#define PTO_FIXP_GV_GVMXA_L_SRC_3 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_3

#define PTO_FIXP_GV_GVMXA_L_SRC_4 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_4

#define PTO_FIXP_GV_GVMXA_L_SRC_5 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_5

#define PTO_FIXP_GV_GVMXA_L_SRC_6 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_6

#define PTO_FIXP_GV_GVMXA_L_SRC_7 \
  "B.IOT %[C], mask=1111\n" "B.IOT %[Vec], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleVec], mask=1111\n" ".endif\n" "B.IOT %[Mtx], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleMtx], mask=1111\n" ".endif\n" \
  PTO_FIXP_PPSRC_7

#define PTO_FIXP_SRC_0 \
  "B.IOT %[A], %[B], mask=1111\n"
#define PTO_FIXP_SRC_1 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[RowIn], mask=1111\n"
#define PTO_FIXP_SRC_2 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[QuantTile], mask=1111\n"
#define PTO_FIXP_SRC_3 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[RowIn], %[QuantTile], mask=1111\n"
#define PTO_FIXP_SRC_4 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[ReluTile], mask=1111\n"
#define PTO_FIXP_SRC_5 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[RowIn], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SRC_6 \
  "B.IOT %[A], %[B], mask=1111\n" "B.IOT %[QuantTile], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SRC_7 \
  "B.IOT %[A], %[B], mask=1111\n" \
  "B.IOT %[RowIn], %[QuantTile], mask=1111\n" \
  "B.IOT %[ReluTile], mask=1111\n"

#define PTO_FIXP_SHARED_B_SRC_0 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_1 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[RowIn], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_2 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[QuantTile], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_3 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[RowIn], %[QuantTile], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_4 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_5 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[RowIn], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_6 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[QuantTile], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_B_SRC_7 \
  "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n" \
  "B.IOT %[RowIn], %[QuantTile], mask=1111\n" "B.IOT %[ReluTile], mask=1111\n"

#define PTO_FIXP_SHARED_A_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_1 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_2 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[QuantTile], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_3 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn], mask=1111\n" \
  "B.IOT %[QuantTile], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_4 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_5 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn], mask=1111\n" \
  "B.IOT %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_6 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[QuantTile], mask=1111\n" \
  "B.IOT %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_A_SRC_7 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[RowIn], mask=1111\n" \
  "B.IOT %[QuantTile], %[ReluTile], mask=1111\n"

#define PTO_FIXP_SHARED_AB_SRC_0 \
  "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_1 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_2 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[QuantTile], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_3 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], %[QuantTile], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_4 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_5 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_6 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[QuantTile], %[ReluTile], mask=1111\n"
#define PTO_FIXP_SHARED_AB_SRC_7 \
  PTO_FIXP_SHARED_AB_SRC_0 "B.IOT %[RowIn], %[QuantTile], mask=1111\n" \
  "B.IOT %[ReluTile], mask=1111\n"

#define PTO_FIXP_IOR_0 ""
#define PTO_FIXP_IOR_1 "B.IOR [%[QuantGpr]],[]\n"
#define PTO_FIXP_IOR_2 "B.IOR [zero,%[LReluGpr]],[]\n"
#define PTO_FIXP_IOR_3 "B.IOR [%[QuantGpr],%[LReluGpr]],[]\n"

#define PTO_FIXP_OUT_0 \
  "B.IOT mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
// Multi-output bundles publish each non-final destination with its canonical
// angle SizeCode; only the final destination carries `last`.
#define PTO_FIXP_OUT_1 \
  "B.IOT mask=1111, ->%[Dst]<%Z[DstSize]>\n" \
  "B.IOT mask=1111, last, ->%[RowOut]<%Z[RowSize]>\n"
#define PTO_FIXP_OUT_2 \
  "B.IOT mask=1111, ->%[Dst]<%Z[DstSize]>\n" \
  "B.IOT mask=1111, last, ->%[GroupOut]<%Z[GroupSize]>\n"
#define PTO_FIXP_OUT_3 \
  "B.IOT mask=1111, ->%[Dst]<%Z[DstSize]>\n" \
  "B.IOT mask=1111, ->%[RowOut]<%Z[RowSize]>\n" \
  "B.IOT mask=1111, last, ->%[GroupOut]<%Z[GroupSize]>\n"

// Every destination uses an early-clobber "=&Tr" constraint. This gives the
// read-old/write-new contract required by handoff Sec 6.2/5:
//   * D == C (ACC variants) reads the old C tile, writes the new D tile;
//   * D / RowMaxOut / GroupMaxOut are distinct outputs that must not alias
//     (each is bound to its own early-clobbered register).
// A caller passing the same Tile object for two outputs (or for an output and
// an input the bundle must read) is a programming error; the asm constraint
// does not attempt to detect it.
#define PTO_FIXP_OUT_DECL_0 [Dst] "=&Tr"(dst.data())
#define PTO_FIXP_OUT_DECL_1 \
  [Dst] "=&Tr"(dst.data()), [RowOut] "=&Tr"(row_out.data())
#define PTO_FIXP_OUT_DECL_2 \
  [Dst] "=&Tr"(dst.data()), [GroupOut] "=&Tr"(group_out.data())
#define PTO_FIXP_OUT_DECL_3 \
  [Dst] "=&Tr"(dst.data()), [RowOut] "=&Tr"(row_out.data()), \
  [GroupOut] "=&Tr"(group_out.data())

#define PTO_PP_INPUTS_0
#define PTO_PP_INPUTS_1 [RowIn] "Tr"(row_in.data()),
#define PTO_PP_INPUTS_2 [QuantTile] "Tr"(quant_tile.data()),
#define PTO_PP_INPUTS_3 [RowIn] "Tr"(row_in.data()), [QuantTile] "Tr"(quant_tile.data()),
#define PTO_PP_INPUTS_4 [ReluTile] "Tr"(relu_tile.data()),
#define PTO_PP_INPUTS_5 [RowIn] "Tr"(row_in.data()), [ReluTile] "Tr"(relu_tile.data()),
#define PTO_PP_INPUTS_6 [QuantTile] "Tr"(quant_tile.data()), [ReluTile] "Tr"(relu_tile.data()),
#define PTO_PP_INPUTS_7 [RowIn] "Tr"(row_in.data()), [QuantTile] "Tr"(quant_tile.data()), [ReluTile] "Tr"(relu_tile.data()),

// ACC PostProcess emitters: same OUT/IOR handling as the base emitter, but the
// math source stream starts with the explicit C accumulator tile (order C,A,B).
#define PTO_FIXP_ACC_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)                          \
      PTO_FIXP_ACC_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [A] "Tr"(a.data()), [B] "Tr"(b.data()),         \
        [CScaleOperand] "Tr"(cscale.data()), PTO_PP_INPUTS_##SRC \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),               \
        PTO_FIXP_ATTR_INPUTS,                                                   \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                          \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_ACC_EMIT_SHARED_B(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)                          \
      PTO_FIXP_ACC_SB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR          \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle()), \
        [CScaleOperand] "Tr"(cscale.data()), PTO_PP_INPUTS_##SRC \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),               \
        PTO_FIXP_ATTR_INPUTS,                                                   \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                          \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_ACC_EMIT_SHARED_A(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)                          \
      PTO_FIXP_ACC_SA_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR          \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()), \
        [CScaleOperand] "Tr"(cscale.data()), PTO_PP_INPUTS_##SRC \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),               \
        PTO_FIXP_ATTR_INPUTS,                                                   \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                          \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_ACC_EMIT_SHARED_AB(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.ACC", PTO_FIXP_ATTR)                          \
      PTO_FIXP_ACC_SAB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR         \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()), \
        [CScaleOperand] "Tr"(cscale.data()), PTO_PP_INPUTS_##SRC \
        [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr),               \
        PTO_FIXP_ATTR_INPUTS,                                                   \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K),                          \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_BIAS_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.BIAS", PTO_FIXP_ATTR)                          \
      PTO_FIXP_BIAS_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [A] "Tr"(a.data()), [B] "Tr"(b.data()), [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_BIAS_EMIT_SHARED_B(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.BIAS", PTO_FIXP_ATTR)                          \
      PTO_FIXP_BIAS_SB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle()), [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_BIAS_EMIT_SHARED_A(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.BIAS", PTO_FIXP_ATTR)                          \
      PTO_FIXP_BIAS_SA_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()), [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_BIAS_EMIT_SHARED_AB(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMUL.BIAS", PTO_FIXP_ATTR)                          \
      PTO_FIXP_BIAS_SAB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()), [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)                          \
      PTO_FIXP_MX_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR       \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_EMIT_SHARED_B(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)                          \
      PTO_FIXP_MX_SB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR      \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_EMIT_SHARED_A(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)                          \
      PTO_FIXP_MX_SA_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR      \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_EMIT_SHARED_AB(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)                          \
      PTO_FIXP_MX_SAB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR          \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_ACC_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.ACC", PTO_FIXP_ATTR)                          \
      "B.IOT %[C], mask=1111\n"                                                      \
      PTO_FIXP_MX_ACC_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [CScaleOperand] "Tr"(cscale.data()), [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_ACC_EMIT_SHARED_B(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.ACC", PTO_FIXP_ATTR)                          \
      "B.IOT %[C], mask=1111\n"                                                      \
      PTO_FIXP_MX_ACC_SB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [CScaleOperand] "Tr"(cscale.data()), [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_ACC_EMIT_SHARED_A(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.ACC", PTO_FIXP_ATTR)                          \
      "B.IOT %[C], mask=1111\n"                                                      \
      PTO_FIXP_MX_ACC_SA_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR           \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [CScaleOperand] "Tr"(cscale.data()), [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_ACC_EMIT_SHARED_AB(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.ACC", PTO_FIXP_ATTR)                          \
      "B.IOT %[C], mask=1111\n"                                                      \
      PTO_FIXP_MX_ACC_SAB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR     \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [CScaleOperand] "Tr"(cscale.data()), [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle()), PTO_MX_SCALE_INPUTS,\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_BIAS_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.BIAS", PTO_FIXP_ATTR)                        \
      PTO_FIXP_MX_BIAS_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data()), PTO_MX_SCALE_INPUTS, [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_BIAS_EMIT_SHARED_B(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.BIAS", PTO_FIXP_ATTR)                        \
      PTO_FIXP_MX_BIAS_SB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle()), PTO_MX_SCALE_INPUTS, [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_BIAS_EMIT_SHARED_A(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.BIAS", PTO_FIXP_ATTR)                        \
      PTO_FIXP_MX_BIAS_SA_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data()), PTO_MX_SCALE_INPUTS, [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_MX_BIAS_EMIT_SHARED_AB(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TMATMULMX.BIAS", PTO_FIXP_ATTR)                        \
      PTO_FIXP_MX_BIAS_SAB_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle()), PTO_MX_SCALE_INPUTS, [Bias] "Tr"(bias.data()),\
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

// Optional MX scale paths must omit absent operands from the LLVM IR inline-
// asm constraint list.  These fragments describe exactly the architectural
// source stream for ScaleMask 0/1/2; ScaleMask 3 uses the established emitters.
#define PTO_MX_L_SRC_0 "B.IOT %[A], mask=1111\n" "B.IOT %[B], mask=1111\n"
#define PTO_MX_L_SRC_1 "B.IOT %[A], %[ScaleA], mask=1111\n" "B.IOT %[B], mask=1111\n"
#define PTO_MX_L_SRC_2 "B.IOT %[A], mask=1111\n" "B.IOT %[B], %[ScaleB], mask=1111\n"
#define PTO_MX_L_INPUTS_0 [A] "Tr"(a.data()), [B] "Tr"(b.data())
#define PTO_MX_L_INPUTS_1 [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [B] "Tr"(b.data())
#define PTO_MX_L_INPUTS_2 [A] "Tr"(a.data()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data())

#define PTO_MX_SB_SRC_0 "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], mask=1111\n"
#define PTO_MX_SB_SRC_1 "B.IOS %S[SharedB], mask=1111\n" "B.IOT %[A], %[ScaleA], mask=1111\n"
#define PTO_MX_SB_SRC_2 "B.IOS %S[SharedB], mask=1111\n" "B.IOS %S[ScaleB], mask=1111\n" "B.IOT %[A], mask=1111\n"
#define PTO_MX_SB_INPUTS_0 [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle())
#define PTO_MX_SB_INPUTS_1 [A] "Tr"(a.data()), [ScaleA] "Tr"(ascale.data()), [SharedB] "Sr"(b.handle())
#define PTO_MX_SB_INPUTS_2 [A] "Tr"(a.data()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle())

#define PTO_MX_SA_SRC_0 "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], mask=1111\n"
#define PTO_MX_SA_SRC_1 "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[ScaleA], mask=1111\n" "B.IOT %[B], mask=1111\n"
#define PTO_MX_SA_SRC_2 "B.IOS %S[SharedA], mask=1111\n" "B.IOT %[B], %[ScaleB], mask=1111\n"
#define PTO_MX_SA_INPUTS_0 [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data())
#define PTO_MX_SA_INPUTS_1 [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [B] "Tr"(b.data())
#define PTO_MX_SA_INPUTS_2 [SharedA] "Sr"(a.handle()), [B] "Tr"(b.data()), [ScaleB] "Tr"(bscale.data())

#define PTO_MX_SAB_SRC_0 "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n"
#define PTO_MX_SAB_SRC_1 "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[ScaleA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n"
#define PTO_MX_SAB_SRC_2 "B.IOS %S[SharedA], mask=1111\n" "B.IOS %S[SharedB], mask=1111\n" "B.IOS %S[ScaleB], mask=1111\n"
#define PTO_MX_SAB_INPUTS_0 [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle())
#define PTO_MX_SAB_INPUTS_1 [SharedA] "Sr"(a.handle()), [ScaleA] "Sr"(ascale.handle()), [SharedB] "Sr"(b.handle())
#define PTO_MX_SAB_INPUTS_2 [SharedA] "Sr"(a.handle()), [SharedB] "Sr"(b.handle()), [ScaleB] "Sr"(bscale.handle())

#define PTO_MX_EXTRA_NONE
#define PTO_MX_EXTRA_ACC [C] "Tr"(c.data()),
#define PTO_MX_EXTRA_BIAS [Bias] "Tr"(bias.data()),
#define PTO_MX_OPTIONAL_EMIT(OPCODE, STORAGE, PREFIX, SUFFIX, EXTRA, MASK, SRC, OUT, IOR) \
  asm volatile(PTO_MATMUL_HEADER(OPCODE, PTO_FIXP_ATTR) PREFIX PTO_MX_##STORAGE##_SRC_##MASK SUFFIX \
      PTO_FIXP_PPSRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT \
      : PTO_MX_EXTRA_##EXTRA PTO_MX_##STORAGE##_INPUTS_##MASK, \
        PTO_PP_INPUTS_##SRC \
        [QuantGpr] "r"(quant_gpr), \
        [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, \
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K), \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_MX_OPT_PLAIN_L(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX",L,"","",NONE,M,S,O,I)
#define PTO_MX_OPT_PLAIN_SB(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX",SB,"","",NONE,M,S,O,I)
#define PTO_MX_OPT_PLAIN_SA(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX",SA,"","",NONE,M,S,O,I)
#define PTO_MX_OPT_PLAIN_SAB(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX",SAB,"","",NONE,M,S,O,I)
#define PTO_MX_OPT_ACC_L(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.ACC",L,"B.IOT %[C], mask=1111\n","",ACC,M,S,O,I)
#define PTO_MX_OPT_ACC_SB(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.ACC",SB,"B.IOT %[C], mask=1111\n","",ACC,M,S,O,I)
#define PTO_MX_OPT_ACC_SA(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.ACC",SA,"B.IOT %[C], mask=1111\n","",ACC,M,S,O,I)
#define PTO_MX_OPT_ACC_SAB(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.ACC",SAB,"B.IOT %[C], mask=1111\n","",ACC,M,S,O,I)
#define PTO_MX_OPT_BIAS_L(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.BIAS",L,"","B.IOT %[Bias], mask=1111\n",BIAS,M,S,O,I)
#define PTO_MX_OPT_BIAS_SB(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.BIAS",SB,"","B.IOT %[Bias], mask=1111\n",BIAS,M,S,O,I)
#define PTO_MX_OPT_BIAS_SA(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.BIAS",SA,"","B.IOT %[Bias], mask=1111\n",BIAS,M,S,O,I)
#define PTO_MX_OPT_BIAS_SAB(M,S,O,I) PTO_MX_OPTIONAL_EMIT("TMATMULMX.BIAS",SAB,"","B.IOT %[Bias], mask=1111\n",BIAS,M,S,O,I)
#define PTO_MX_OPT_PLAIN_L_0(S,O,I) PTO_MX_OPT_PLAIN_L(0,S,O,I)
#define PTO_MX_OPT_PLAIN_L_1(S,O,I) PTO_MX_OPT_PLAIN_L(1,S,O,I)
#define PTO_MX_OPT_PLAIN_L_2(S,O,I) PTO_MX_OPT_PLAIN_L(2,S,O,I)
#define PTO_MX_OPT_PLAIN_SB_0(S,O,I) PTO_MX_OPT_PLAIN_SB(0,S,O,I)
#define PTO_MX_OPT_PLAIN_SB_1(S,O,I) PTO_MX_OPT_PLAIN_SB(1,S,O,I)
#define PTO_MX_OPT_PLAIN_SB_2(S,O,I) PTO_MX_OPT_PLAIN_SB(2,S,O,I)
#define PTO_MX_OPT_PLAIN_SA_0(S,O,I) PTO_MX_OPT_PLAIN_SA(0,S,O,I)
#define PTO_MX_OPT_PLAIN_SA_1(S,O,I) PTO_MX_OPT_PLAIN_SA(1,S,O,I)
#define PTO_MX_OPT_PLAIN_SA_2(S,O,I) PTO_MX_OPT_PLAIN_SA(2,S,O,I)
#define PTO_MX_OPT_PLAIN_SAB_0(S,O,I) PTO_MX_OPT_PLAIN_SAB(0,S,O,I)
#define PTO_MX_OPT_PLAIN_SAB_1(S,O,I) PTO_MX_OPT_PLAIN_SAB(1,S,O,I)
#define PTO_MX_OPT_PLAIN_SAB_2(S,O,I) PTO_MX_OPT_PLAIN_SAB(2,S,O,I)
#define PTO_MX_OPT_ACC_L_0(S,O,I) PTO_MX_OPT_ACC_L(0,S,O,I)
#define PTO_MX_OPT_ACC_L_1(S,O,I) PTO_MX_OPT_ACC_L(1,S,O,I)
#define PTO_MX_OPT_ACC_L_2(S,O,I) PTO_MX_OPT_ACC_L(2,S,O,I)
#define PTO_MX_OPT_ACC_SB_0(S,O,I) PTO_MX_OPT_ACC_SB(0,S,O,I)
#define PTO_MX_OPT_ACC_SB_1(S,O,I) PTO_MX_OPT_ACC_SB(1,S,O,I)
#define PTO_MX_OPT_ACC_SB_2(S,O,I) PTO_MX_OPT_ACC_SB(2,S,O,I)
#define PTO_MX_OPT_ACC_SA_0(S,O,I) PTO_MX_OPT_ACC_SA(0,S,O,I)
#define PTO_MX_OPT_ACC_SA_1(S,O,I) PTO_MX_OPT_ACC_SA(1,S,O,I)
#define PTO_MX_OPT_ACC_SA_2(S,O,I) PTO_MX_OPT_ACC_SA(2,S,O,I)
#define PTO_MX_OPT_ACC_SAB_0(S,O,I) PTO_MX_OPT_ACC_SAB(0,S,O,I)
#define PTO_MX_OPT_ACC_SAB_1(S,O,I) PTO_MX_OPT_ACC_SAB(1,S,O,I)
#define PTO_MX_OPT_ACC_SAB_2(S,O,I) PTO_MX_OPT_ACC_SAB(2,S,O,I)
#define PTO_MX_OPT_BIAS_L_0(S,O,I) PTO_MX_OPT_BIAS_L(0,S,O,I)
#define PTO_MX_OPT_BIAS_L_1(S,O,I) PTO_MX_OPT_BIAS_L(1,S,O,I)
#define PTO_MX_OPT_BIAS_L_2(S,O,I) PTO_MX_OPT_BIAS_L(2,S,O,I)
#define PTO_MX_OPT_BIAS_SB_0(S,O,I) PTO_MX_OPT_BIAS_SB(0,S,O,I)
#define PTO_MX_OPT_BIAS_SB_1(S,O,I) PTO_MX_OPT_BIAS_SB(1,S,O,I)
#define PTO_MX_OPT_BIAS_SB_2(S,O,I) PTO_MX_OPT_BIAS_SB(2,S,O,I)
#define PTO_MX_OPT_BIAS_SA_0(S,O,I) PTO_MX_OPT_BIAS_SA(0,S,O,I)
#define PTO_MX_OPT_BIAS_SA_1(S,O,I) PTO_MX_OPT_BIAS_SA(1,S,O,I)
#define PTO_MX_OPT_BIAS_SA_2(S,O,I) PTO_MX_OPT_BIAS_SA(2,S,O,I)
#define PTO_MX_OPT_BIAS_SAB_0(S,O,I) PTO_MX_OPT_BIAS_SAB(0,S,O,I)
#define PTO_MX_OPT_BIAS_SAB_1(S,O,I) PTO_MX_OPT_BIAS_SAB(1,S,O,I)
#define PTO_MX_OPT_BIAS_SAB_2(S,O,I) PTO_MX_OPT_BIAS_SAB(2,S,O,I)
#define PTO_MX_DISPATCH_OPTIONAL(BASE)                                         \
  if constexpr (ScaleMask == 0) { PTO_FIXP_DISPATCH(BASE##_0); }              \
  else if constexpr (ScaleMask == 1) { PTO_FIXP_DISPATCH(BASE##_1); }         \
  else { PTO_FIXP_DISPATCH(BASE##_2); }

#define PTO_GV_MX_SRC_0 "B.IOT %[Vec], mask=1111\n" "B.IOT %[Mtx], mask=1111\n"
#define PTO_GV_MX_SRC_1 "B.IOT %[Vec], %[ScaleVec], mask=1111\n" "B.IOT %[Mtx], mask=1111\n"
#define PTO_GV_MX_SRC_2 "B.IOT %[Vec], mask=1111\n" "B.IOT %[Mtx], %[ScaleMtx], mask=1111\n"
#define PTO_GV_MX_INPUTS_0 [Vec] "Tr"(vec.data()), [Mtx] "Tr"(mtx.data())
#define PTO_GV_MX_INPUTS_1 [Vec] "Tr"(vec.data()), [ScaleVec] "Tr"(svec.data()), [Mtx] "Tr"(mtx.data())
#define PTO_GV_MX_INPUTS_2 [Vec] "Tr"(vec.data()), [Mtx] "Tr"(mtx.data()), [ScaleMtx] "Tr"(smtx.data())
#define PTO_GV_MX_OPTIONAL_EMIT(OPCODE, PREFIX, SUFFIX, EXTRA, MASK, SRC, OUT, IOR) \
  asm volatile(PTO_MATMUL_HEADER(OPCODE, PTO_FIXP_ATTR) PREFIX PTO_GV_MX_SRC_##MASK SUFFIX \
      PTO_FIXP_PPSRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT \
      : PTO_MX_EXTRA_##EXTRA PTO_GV_MX_INPUTS_##MASK, \
        PTO_PP_INPUTS_##SRC \
        [QuantGpr] "r"(quant_gpr), \
        [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, \
        PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), \
        [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), \
        [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), \
        [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")
#define PTO_GV_OPT_PLAIN(M,S,O,I) PTO_GV_MX_OPTIONAL_EMIT("TGEMVMX","","",NONE,M,S,O,I)
#define PTO_GV_OPT_ACC(M,S,O,I) PTO_GV_MX_OPTIONAL_EMIT("TGEMVMX.ACC","B.IOT %[C], mask=1111\n","",ACC,M,S,O,I)
#define PTO_GV_OPT_BIAS(M,S,O,I) PTO_GV_MX_OPTIONAL_EMIT("TGEMVMX.BIAS","","B.IOT %[Bias], mask=1111\n",BIAS,M,S,O,I)
#define PTO_GV_OPT_PLAIN_0(S,O,I) PTO_GV_OPT_PLAIN(0,S,O,I)
#define PTO_GV_OPT_PLAIN_1(S,O,I) PTO_GV_OPT_PLAIN(1,S,O,I)
#define PTO_GV_OPT_PLAIN_2(S,O,I) PTO_GV_OPT_PLAIN(2,S,O,I)
#define PTO_GV_OPT_ACC_0(S,O,I) PTO_GV_OPT_ACC(0,S,O,I)
#define PTO_GV_OPT_ACC_1(S,O,I) PTO_GV_OPT_ACC(1,S,O,I)
#define PTO_GV_OPT_ACC_2(S,O,I) PTO_GV_OPT_ACC(2,S,O,I)
#define PTO_GV_OPT_BIAS_0(S,O,I) PTO_GV_OPT_BIAS(0,S,O,I)
#define PTO_GV_OPT_BIAS_1(S,O,I) PTO_GV_OPT_BIAS(1,S,O,I)
#define PTO_GV_OPT_BIAS_2(S,O,I) PTO_GV_OPT_BIAS(2,S,O,I)

#define PTO_FIXP_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                               \
      PTO_MATMUL_HEADER("TMATMUL", PTO_FIXP_ATTR)                    \
      PTO_FIXP_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR                                \
      : PTO_FIXP_OUT_DECL_##OUT                                              \
      : [A] "Tr"(a.data()), [B] "Tr"(b.data()),                            \
        PTO_PP_INPUTS_##SRC \
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
        PTO_PP_INPUTS_##SRC \
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
        PTO_PP_INPUTS_##SRC \
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
        PTO_PP_INPUTS_##SRC \
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
  validate_matrix_contract<Attr, Dst, A, B>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, A, B,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut>();
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

// Forward declaration: defined later in this namespace at the shared
// select_fixp_operand definition. Needed because the ACC/BIAS/MX entry
// points (which live outside pto_matmul_detail) call it here.
template <bool Use, typename Pointer, typename Dummy>
decltype(auto) select_fixp_operand(Pointer *PointerValue, Dummy &DummyValue);

// TMATMUL.ACC full-PostProcess emitter: math source stream is C,A,B (explicit
// C accumulator read first), then the shared PostProcess auxiliary sources and
// outputs (RowMaxIn/QuantTile/PReLU/RowMaxOut/GroupMaxOut + scalar B.IOR).
template <FixpAttr Attr, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename C_, typename A, typename B, typename RowIn,
          typename QuantTile,
          typename ReluTile, typename RowOut, typename GroupOut,
          typename CScale>
PTO_SHARED_INLINE void emit_matmul_acc_fixp(
    Dst &dst, C_ &c, A &a, B &b, CScale &cscale, RowIn &row_in,
    QuantTile &quant_tile,
    ReluTile &relu_tile, RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  validate_matrix_contract<Attr, Dst, A, B>();
  validate_matrix_accumulator_contract<Attr, Dst, C_, A, B>();
  validate_cscale_contract<Attr, C_, CScale>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, A, B,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_ACC_EMIT_LOCAL);
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_ACC_EMIT_SHARED_A);
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_ACC_EMIT_SHARED_B);
  } else {
    PTO_FIXP_DISPATCH(PTO_FIXP_ACC_EMIT_SHARED_AB);
  }
}

// TMATMUL.BIAS full-PostProcess emitter: math source A,B,Bias, then PP aux.
template <FixpAttr Attr, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename A, typename B, typename BiasT, typename RowIn,
          typename QuantTile,
          typename ReluTile, typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_matmul_bias_fixp(
    Dst &dst, A &a, B &b, BiasT &bias, RowIn &row_in, QuantTile &quant_tile,
    ReluTile &relu_tile, RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  validate_matrix_contract<Attr, Dst, A, B>();
  validate_matrix_bias_contract<Attr, BiasT, A, B>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, A, B,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_BIAS_EMIT_LOCAL);
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_BIAS_EMIT_SHARED_A);
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    PTO_FIXP_DISPATCH(PTO_FIXP_BIAS_EMIT_SHARED_B);
  } else {
    PTO_FIXP_DISPATCH(PTO_FIXP_BIAS_EMIT_SHARED_AB);
  }
}

// TMATMULMX full-PostProcess emitter: math source A,ScaleA,B,ScaleB.
template <FixpAttr Attr, int ScaleMask, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename A, typename ScaleA, typename B,
          typename ScaleB, typename RowIn, typename QuantTile,
          typename ReluTile, typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_matmul_mx_fixp(
    Dst &dst, A &a, ScaleA &ascale, B &b, ScaleB &bscale,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  static_assert(ScaleMask >= 0 && ScaleMask <= 3,
                "MX scale mask contains only independent A/B presence bits");
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;
  validate_matrix_contract<Attr, Dst, A, B, true>();
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB,
      ScaleA, A, ScaleB, B>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, A, B,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut, true>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_EMIT_LOCAL); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_PLAIN_L); }
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_EMIT_SHARED_A); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_PLAIN_SA); }
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_EMIT_SHARED_B); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_PLAIN_SB); }
  } else {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_EMIT_SHARED_AB); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_PLAIN_SAB); }
  }
}

// TMATMULMX.ACC full-PostProcess emitter: math source C,A,ScaleA,B,ScaleB.
template <FixpAttr Attr, int ScaleMask, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename C_, typename A, typename ScaleA, typename B,
          typename ScaleB, typename RowIn, typename QuantTile,
          typename ReluTile, typename RowOut, typename GroupOut,
          typename CScale>
PTO_SHARED_INLINE void emit_matmul_mx_acc_fixp(
    Dst &dst, C_ &c, A &a, ScaleA &ascale, B &b, ScaleB &bscale,
    CScale &cscale,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  static_assert(ScaleMask >= 0 && ScaleMask <= 3,
                "MX scale mask contains only independent A/B presence bits");
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;
  validate_matrix_contract<Attr, Dst, A, B, true>();
  validate_matrix_accumulator_contract<Attr, Dst, C_, A, B, true>();
  validate_cscale_contract<Attr, C_, CScale>();
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB,
      ScaleA, A, ScaleB, B>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, A, B,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut, true>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_ACC_EMIT_LOCAL); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_ACC_L); }
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_ACC_EMIT_SHARED_A); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_ACC_SA); }
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_ACC_EMIT_SHARED_B); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_ACC_SB); }
  } else {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_ACC_EMIT_SHARED_AB); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_ACC_SAB); }
  }
}

// TMATMULMX.BIAS full-PostProcess emitter: math source
// A,ScaleA,B,ScaleB,Bias, then PP aux.
template <FixpAttr Attr, int ScaleMask, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename A, typename ScaleA, typename B,
          typename ScaleB, typename BiasT, typename RowIn, typename QuantTile,
          typename ReluTile, typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_matmul_mx_bias_fixp(
    Dst &dst, A &a, ScaleA &ascale, B &b, ScaleB &bscale, BiasT &bias,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  static_assert(ScaleMask >= 0 && ScaleMask <= 3,
                "MX scale mask contains only independent A/B presence bits");
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;
  validate_matrix_contract<Attr, Dst, A, B, true>();
  validate_matrix_bias_contract<Attr, BiasT, A, B, true>();
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB,
      ScaleA, A, ScaleB, B>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, A, B,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut, true>();
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_BIAS_EMIT_LOCAL); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_BIAS_L); }
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_BIAS_EMIT_SHARED_A); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_BIAS_SA); }
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_BIAS_EMIT_SHARED_B); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_BIAS_SB); }
  } else {
    if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_MX_BIAS_EMIT_SHARED_AB); }
    else { PTO_MX_DISPATCH_OPTIONAL(PTO_MX_OPT_BIAS_SAB); }
  }
}

// TGEMV/TGEMVMX are Local-only (handoff Sec 1.5: any B.IOS is
// illegal), so each variant has a single EMIT_LOCAL using the
// canonical Local math-source stream + shared PP aux/OUT macros.
#define PTO_FIXP_GV_GV_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TGEMV", PTO_FIXP_ATTR)                         \
      PTO_FIXP_GV_GV_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [Mtx] "Tr"(mtx.data()), [Vec] "Tr"(vec.data()),                                                          \
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_GV_GVB_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TGEMV.BIAS", PTO_FIXP_ATTR)                         \
      PTO_FIXP_GV_GVB_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [Mtx] "Tr"(mtx.data()), [Vec] "Tr"(vec.data()), [Bias] "Tr"(bias.data()),                                                          \
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_GV_GVA_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TGEMV.ACC", PTO_FIXP_ATTR)                         \
      PTO_FIXP_GV_GVA_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [Mtx] "Tr"(mtx.data()), [Vec] "Tr"(vec.data()),                                                          \
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_GV_GVMX_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TGEMVMX", PTO_FIXP_ATTR)                         \
      PTO_FIXP_GV_GVMX_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [Mtx] "Tr"(mtx.data()), [ScaleMtx] "Tr"(smtx.data()), [Vec] "Tr"(vec.data()), [ScaleVec] "Tr"(svec.data()), PTO_MX_SCALE_INPUTS,                                                          \
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_GV_GVMXB_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TGEMVMX.BIAS", PTO_FIXP_ATTR)                         \
      PTO_FIXP_GV_GVMXB_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [Mtx] "Tr"(mtx.data()), [ScaleMtx] "Tr"(smtx.data()), [Vec] "Tr"(vec.data()), [ScaleVec] "Tr"(svec.data()), PTO_MX_SCALE_INPUTS, [Bias] "Tr"(bias.data()),                                                          \
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

#define PTO_FIXP_GV_GVMXA_EMIT_LOCAL(SRC, OUT, IOR) \
  asm volatile(                                                              \
      PTO_MATMUL_HEADER("TGEMVMX.ACC", PTO_FIXP_ATTR)                         \
      PTO_FIXP_GV_GVMXA_L_SRC_##SRC PTO_FIXP_OUT_##OUT PTO_FIXP_IOR_##IOR \
      : PTO_FIXP_OUT_DECL_##OUT                                               \
      : [C] "Tr"(c.data()), [Mtx] "Tr"(mtx.data()), [ScaleMtx] "Tr"(smtx.data()), [Vec] "Tr"(vec.data()), [ScaleVec] "Tr"(svec.data()), PTO_MX_SCALE_INPUTS,                                                          \
        PTO_PP_INPUTS_##SRC [QuantGpr] "r"(quant_gpr), [LReluGpr] "r"(lrelu_gpr), PTO_FIXP_ATTR_INPUTS, PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx, M, N, K), [DstSize] "i"(tile_type_traits<typename Dst::TileDType>::TilesizeCode), [RowSize] "i"(tile_type_traits<typename RowOut::TileDType>::TilesizeCode), [GroupSize] "i"(tile_type_traits<typename GroupOut::TileDType>::TilesizeCode) \
      : "memory")

// emit_gemv_fixp: mtx / vec -> dst, Local-only.
template <FixpAttr Attr, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename Mtx, typename Vec, typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_gemv_fixp(
    Dst &dst,
    Mtx &mtx,
    Vec &vec,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
  uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  validate_gemv_contract<Attr, Dst, Vec, Mtx>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, Vec, Mtx,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut>();
  PTO_FIXP_DISPATCH(PTO_FIXP_GV_GV_EMIT_LOCAL);
}

// emit_gemv_bias_fixp: mtx / vec / bias -> dst, Local-only.
template <FixpAttr Attr, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename Mtx, typename Vec, typename BiasT, typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_gemv_bias_fixp(
    Dst &dst,
    Mtx &mtx,
    Vec &vec,
    BiasT &bias,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
  uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  validate_gemv_contract<Attr, Dst, Vec, Mtx>();
  validate_matrix_bias_contract<Attr, BiasT, Vec, Mtx>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, Vec, Mtx,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut>();
  PTO_FIXP_DISPATCH(PTO_FIXP_GV_GVB_EMIT_LOCAL);
}

// emit_gemv_acc_fixp: c / mtx / vec -> dst, Local-only.
template <FixpAttr Attr, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename C, typename Mtx, typename Vec, typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_gemv_acc_fixp(
    Dst &dst,
    C &c,
    Mtx &mtx,
    Vec &vec,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  validate_gemv_contract<Attr, Dst, Vec, Mtx>();
  validate_matrix_accumulator_contract<Attr, Dst, C, Vec, Mtx>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, Vec, Mtx,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut>();
  PTO_FIXP_DISPATCH(PTO_FIXP_GV_GVA_EMIT_LOCAL);
}

// emit_gemv_mx_fixp: mtx / smtx / vec / svec -> dst, Local-only.
template <FixpAttr Attr, int ScaleMask, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename Mtx, typename ScaleMtx, typename Vec, typename ScaleVec, typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_gemv_mx_fixp(
    Dst &dst,
    Mtx &mtx,
    ScaleMtx &smtx,
    Vec &vec,
    ScaleVec &svec,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  static_assert(ScaleMask >= 0 && ScaleMask <= 3,
                "MX scale mask contains only independent A/B presence bits");
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;
  validate_gemv_contract<Attr, Dst, Vec, Mtx, true>();
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB,
      ScaleVec, Vec, ScaleMtx, Mtx>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, Vec, Mtx,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut, true>();
  if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_GV_GVMX_EMIT_LOCAL); }
  else { PTO_MX_DISPATCH_OPTIONAL(PTO_GV_OPT_PLAIN); }
}

// emit_gemv_mx_bias_fixp: mtx / smtx / vec / svec / bias -> dst, Local-only.
template <FixpAttr Attr, int ScaleMask, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename Mtx, typename ScaleMtx, typename Vec, typename ScaleVec, typename BiasT, typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_gemv_mx_bias_fixp(
    Dst &dst,
    Mtx &mtx,
    ScaleMtx &smtx,
    Vec &vec,
    ScaleVec &svec,
    BiasT &bias,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  static_assert(ScaleMask >= 0 && ScaleMask <= 3,
                "MX scale mask contains only independent A/B presence bits");
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;
  validate_gemv_contract<Attr, Dst, Vec, Mtx, true>();
  validate_matrix_bias_contract<Attr, BiasT, Vec, Mtx, true>();
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB,
      ScaleVec, Vec, ScaleMtx, Mtx>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, Vec, Mtx,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut, true>();
  if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_GV_GVMXB_EMIT_LOCAL); }
  else { PTO_MX_DISPATCH_OPTIONAL(PTO_GV_OPT_BIAS); }
}

// emit_gemv_mx_acc_fixp: c / mtx / smtx / vec / svec -> dst, Local-only.
template <FixpAttr Attr, int ScaleMask, int SrcMask, int OutMask, int IorMode,
          typename Dst, typename C, typename Mtx, typename ScaleMtx, typename Vec, typename ScaleVec, typename RowIn, typename QuantTile, typename ReluTile,
          typename RowOut, typename GroupOut>
PTO_SHARED_INLINE void emit_gemv_mx_acc_fixp(
    Dst &dst,
    C &c,
    Mtx &mtx,
    ScaleMtx &smtx,
    Vec &vec,
    ScaleVec &svec,
    RowIn &row_in, QuantTile &quant_tile, ReluTile &relu_tile,
    RowOut &row_out, GroupOut &group_out,
    uint64_t quant_gpr, uint64_t lrelu_gpr, size_t M, size_t N, size_t K) {
  static_assert(ScaleMask >= 0 && ScaleMask <= 3,
                "MX scale mask contains only independent A/B presence bits");
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;
  validate_gemv_contract<Attr, Dst, Vec, Mtx, true>();
  validate_matrix_accumulator_contract<Attr, Dst, C, Vec, Mtx, true>();
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB,
      ScaleVec, Vec, ScaleMtx, Mtx>();
  validate_matrix_postprocess_contract<Attr, SrcMask, OutMask, Vec, Mtx,
      RowIn, QuantTile, ReluTile, RowOut, GroupOut, true>();
  if constexpr (ScaleMask == 3) { PTO_FIXP_DISPATCH(PTO_FIXP_GV_GVMXA_EMIT_LOCAL); }
  else { PTO_MX_DISPATCH_OPTIONAL(PTO_GV_OPT_ACC); }
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
  constexpr bool HasScaleA = true;                                               \
  constexpr bool HasScaleB = true;                                               \
  validate_matrix_contract<Attr, Dst, A, B, true>();                                      \
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB, ScaleA, A, ScaleB, B>();      \
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {                \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n"                                    \
        "B.IOT %[B], %[ScaleB], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"     \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[B], mask=1111\n"                                   \
        "B.IOT %[ScaleB], mask=1111\n"                                                  \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n"                                   \
        "B.IOT %[ScaleB], mask=1111\n"                                                  \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else {                                                                      \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[ScaleB], mask=1111\n"                              \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  }                                                                             \
}

PTO_DEFINE_MATMUL_MX_HELPER(matmul_mx, "TMATMULMX")

template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename B>
PTO_SHARED_INLINE void matmul_mx_no_scale_local(
    Dst &dst, A &a, B &b, size_t M, size_t N, size_t K) {
  static_assert(!is_shared_tile_v<A> && !is_shared_tile_v<B>);
  validate_matrix_contract<Attr, Dst, A, B, true>();
  validate_matrix_scale_contract<Attr, false, false, A, A, B, B>();
  asm volatile(
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)
      "B.IOT %[A], %[B], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [A] "Tr"(a.data()), [B] "Tr"(b.data()), PTO_FIXP_ATTR_INPUTS,
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
      : "memory");
}

template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A,
          typename ScaleA, typename B>
PTO_SHARED_INLINE void matmul_mx_scale_a_local(
    Dst &dst, A &a, ScaleA &scale_a, B &b, size_t M, size_t N, size_t K) {
  static_assert(!is_shared_tile_v<A> && !is_shared_tile_v<B>);
  validate_matrix_contract<Attr, Dst, A, B, true>();
  validate_matrix_scale_contract<Attr, true, false, ScaleA, A, B, B>();
  asm volatile(
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)
      "B.IOT %[A], %[ScaleA], mask=1111\n"
      "B.IOT %[B], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),
        [B] "Tr"(b.data()), PTO_FIXP_ATTR_INPUTS,
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
      : "memory");
}

template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename B,
          typename ScaleB>
PTO_SHARED_INLINE void matmul_mx_scale_b_local(
    Dst &dst, A &a, B &b, ScaleB &scale_b, size_t M, size_t N, size_t K) {
  static_assert(!is_shared_tile_v<A> && !is_shared_tile_v<B>);
  validate_matrix_contract<Attr, Dst, A, B, true>();
  validate_matrix_scale_contract<Attr, false, true, A, A, ScaleB, B>();
  asm volatile(
      PTO_MATMUL_HEADER("TMATMULMX", PTO_FIXP_ATTR)
      "B.IOT %[A], mask=1111\n"
      "B.IOT %[B], %[ScaleB], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [A] "Tr"(a.data()), [B] "Tr"(b.data()),
        [ScaleB] "Tr"(scale_b.data()), PTO_FIXP_ATTR_INPUTS,
        PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)
      : "memory");
}

#define PTO_DEFINE_MATMUL_MX_5SRC_HELPER(Name, Opcode, IsAcc)                   \
template <FixpAttr Attr = FixpAttr{}, typename Dst, typename A, typename ScaleA,  \
          typename B, typename ScaleB, typename Extra>                           \
PTO_SHARED_INLINE void Name(Dst &dst, A &a, ScaleA &scale_a, B &b,             \
                            ScaleB &scale_b, Extra &extra, size_t M, size_t N,   \
                            size_t K) {                                          \
  constexpr bool HasScaleA = true;                                               \
  constexpr bool HasScaleB = true;                                               \
  validate_matrix_contract<Attr, Dst, A, B, true>();                                      \
  validate_matrix_scale_contract<Attr, HasScaleA, HasScaleB, ScaleA, A, ScaleB, B>();      \
  if constexpr (IsAcc)                                                                    \
    validate_matrix_accumulator_contract<Attr, Dst, Extra, A, B, true>();                       \
  else                                                                                    \
    validate_matrix_bias_contract<Attr, Extra, A, B, true>();                                   \
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B>) {                \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n"                                    \
        "B.IOT %[B], mask=1111\n" ".if %c[HasScaleB]\n" "B.IOT %[ScaleB], mask=1111\n" ".endif\n"                                   \
        "B.IOT %[Extra], mask=1111\n"                                                   \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (is_shared_tile_v<A> && !is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[B], mask=1111\n"                                   \
        "B.IOT %[ScaleB], %[Extra], mask=1111\n"                               \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [B] "Tr"(b.data()), [ScaleB] "Tr"(scale_b.data()),                 \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else if constexpr (!is_shared_tile_v<A> && is_shared_tile_v<B>) {          \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[A], mask=1111\n" ".if %c[HasScaleA]\n" "B.IOT %[ScaleA], mask=1111\n" ".endif\n"                                   \
        "B.IOT %[ScaleB], %[Extra], mask=1111\n"                               \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [A] "Tr"(a.data()), [ScaleA] "Tr"(scale_a.data()),                 \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  } else {                                                                      \
    asm volatile(                                                               \
        PTO_MATMUL_HEADER(Opcode, PTO_FIXP_ATTR)                               \
        "B.IOS %S[SharedA], mask=1111\n"                                              \
        "B.IOS %S[SharedB], mask=1111\n"                                              \
        "B.IOT %[ScaleA], %[ScaleB], mask=1111\n"                              \
        "B.IOT %[Extra], mask=1111\n"                                                   \
        "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"                      \
        : [Dst] "=&Tr"(dst.data())                                            \
        : [SharedA] "Sr"(a.handle()), [ScaleA] "Tr"(scale_a.data()),         \
          [SharedB] "Sr"(b.handle()), [ScaleB] "Tr"(scale_b.data()),         \
          [Extra] "Tr"(extra.data()),                                         \
          PTO_FIXP_ATTR_INPUTS, PTO_MX_SCALE_INPUTS,                                                \
          PTO_MATMUL_COMMON_INPUTS(Dst, A, B, M, N, K)                         \
        : "memory");                                                          \
  }                                                                             \
}

PTO_DEFINE_MATMUL_MX_5SRC_HELPER(matmul_mx_bias, "TMATMULMX.BIAS", false)
PTO_DEFINE_MATMUL_MX_5SRC_HELPER(matmul_mx_acc, "TMATMULMX.ACC", true)

#undef PTO_DEFINE_MATMUL_MX_5SRC_HELPER
#undef PTO_DEFINE_MATMUL_MX_HELPER
#undef PTO_MX_SCALE_INPUTS
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
                "require the overload taking fixp::Options");
  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(c, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;
  pto_matmul_detail::matmul<Attr>(c, a, b, M, N, K);
}


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
                "require the overload taking fixp::Options");
  static_assert(!Attr.CScaleEn,
                "CScale requires the ACC overload with a CScale tile");
  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;
  pto_matmul_detail::matmul_acc<Attr>(d, c, a, b, M, N, K);
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
  static_assert(is_valid_fixp_attr(Attr), "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_d::DType>(),
                "TMATMUL_ACC destination dtype does not match PreQuantMode");

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
  constexpr int EffectiveM = is_shared_tile_v<tile_shape_a> && Attr.TransA
      ? tile_shape_a::ValidCol : tile_shape_a::ValidRow;
  constexpr int EffectiveN = is_shared_tile_v<tile_shape_b> && Attr.TransB
      ? tile_shape_b::ValidRow : tile_shape_b::ValidCol;

  static_assert(HasVectorQuant ==
                    !std::is_same_v<typename Options::QuantTile,
                                    fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu ==
                    !std::is_same_v<typename Options::ReluTile,
                                    fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn ==
                    !std::is_same_v<typename Options::RowMaxIn,
                                    fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut ==
                    !std::is_same_v<typename Options::RowMaxOut,
                                    fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut ==
                    !std::is_same_v<typename Options::GroupMaxOut,
                                    fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);
  auto &cscale = pto_matmul_detail::select_fixp_operand<Attr.CScaleEn>(
      options.CScale, c);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_matmul_acc_fixp<Attr, SrcMask, OutMask, IorMode>(
      d, c, a, b, cscale, row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}


namespace pto_matmul_detail {

struct NoScaleOperand {};

template <bool Use, typename Pointer, typename Dummy>
decltype(auto) select_fixp_operand(Pointer *PointerValue, Dummy &DummyValue) {
  if constexpr (Use)
    return *PointerValue;
  else
    return (DummyValue);
}

} // namespace pto_matmul_detail

// TMATMUL(D, A, B, options): carries the full PostProcess capability
// (quant/PReLU/RowMax/GroupMax) via a compile-time-shaped options object;
// scalar descriptors remain runtime GPR values and tile operands remain
// runtime tile registers.
template <is_tile_data_v tile_shape_d, is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b, fixp::is_options_v Options>
__attribute__((always_inline)) inline void
TMATMUL(tile_shape_d &d, tile_shape_a &a,
                                    tile_shape_b &b, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_valid_fixp_attr(Attr),
                "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_d::DType>(),
                "TMATMUL destination dtype does not match PreQuantMode");
  static_assert(tile_role_v<tile_shape_a> == Location::Left,
                "TMATMUL input A must be Location::Left");
  static_assert(tile_role_v<tile_shape_b> == Location::Right,
                "TMATMUL input B must be a Right tile");
  // Physical and valid M/N/K contracts, including cooperative Shared inputs,
  // are centralized in resolve_matmul_shape_runtime below.
  static_assert(tile_type_traits<typename tile_shape_d::TileDType>::IsValidActiveSize,
                "TMATMUL output logical Tile size must be 128 B..256 KiB (SizeCode=1..12)");

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
  constexpr int EffectiveM = is_shared_tile_v<tile_shape_a> && Attr.TransA
      ? tile_shape_a::ValidCol : tile_shape_a::ValidRow;
  constexpr int EffectiveN = is_shared_tile_v<tile_shape_b> && Attr.TransB
      ? tile_shape_b::ValidRow : tile_shape_b::ValidCol;

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
        "TMATMUL quant parameter Tile must occupy 128 B..256 KiB (SizeCode=1..12); pad the "
        "physical Tile and keep ValidRow=1, ValidCol=N when necessary");
    static_assert(QuantTile::ValidRow == -1 || QuantTile::ValidRow == 1,
                  "TMATMUL vector quant parameter must have ValidRow=1");
    static_assert(QuantTile::ValidCol == EffectiveN,
                  "TMATMUL vector quant parameter must have ValidCol=N");
  }
  if constexpr (HasPRelu) {
    using ReluTile = typename Options::ReluTile;
    static_assert(
        tile_type_traits<typename ReluTile::TileDType>::IsValidActiveSize,
        "TMATMUL PReLU parameter Tile must occupy 128 B..256 KiB (SizeCode=1..12); pad the "
        "physical Tile and keep ValidRow=1, ValidCol=N when necessary");
    static_assert(ReluTile::ValidRow == -1 || ReluTile::ValidRow == 1,
                  "TMATMUL PReLU parameter must have ValidRow=1");
    static_assert(ReluTile::ValidCol == EffectiveN,
                  "TMATMUL PReLU parameter must have ValidCol=N");
  }
  if constexpr (HasRowOut) {
    using RowOut = typename Options::RowMaxOut;
    static_assert(RowOut::ValidRow == EffectiveM,
                  "TMATMUL RowMaxOut must have ValidRow=M");
    static_assert(RowOut::ValidCol == -1 || RowOut::ValidCol == 1,
                  "TMATMUL RowMaxOut must have ValidCol=1");
    static_assert(matrix_accumulator_type_legal<tile_shape_a, tile_shape_b,
                                                RowOut>(),
                  "TMATMUL RowMaxOut dtype must match FP32/S32/U32 AccType");
    static_assert(
        tile_type_traits<typename RowOut::TileDType>::IsValidActiveSize,
        "TMATMUL RowMaxOut physical Tile must occupy 128 B..256 KiB (SizeCode=1..12)");
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
                  "TMATMUL RowMaxIn/RowMaxOut valid shapes must match");
    static_assert(std::is_same_v<typename RowIn::DType,
                                 typename RowOut::DType>,
                  "TMATMUL RowMaxIn/RowMaxOut dtypes must match");
    static_assert(
        tile_type_traits<typename RowIn::TileDType>::IsValidActiveSize,
        "TMATMUL RowMaxIn physical Tile must occupy 128 B..256 KiB (SizeCode=1..12)");
  }
  if constexpr (HasGroupOut) {
    using GroupOut = typename Options::GroupMaxOut;
    constexpr int GroupN = fixp::group_n_from_code(Attr.GroupNCode);
    constexpr int ExpectedCols =
        (EffectiveN + GroupN - 1) / GroupN;
    static_assert(GroupOut::ValidRow == EffectiveM,
                  "TMATMUL GroupMaxOut must have ValidRow=M");
    static_assert(GroupOut::ValidCol == -1 || ExpectedCols == -1 ||
                      GroupOut::ValidCol == ExpectedCols,
                  "TMATMUL GroupMaxOut must have ValidCol=ceil(N/GroupN)");
    static_assert(matrix_accumulator_type_legal<tile_shape_a, tile_shape_b,
                                                GroupOut>(),
                  "TMATMUL GroupMaxOut dtype must match FP32/S32/U32 AccType");
    static_assert(
        tile_type_traits<typename GroupOut::TileDType>::IsValidActiveSize,
        "TMATMUL GroupMaxOut physical Tile must occupy 128 B..256 KiB (SizeCode=1..12)");
  }

  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;

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
                "require the overload taking fixp::Options");
  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(c, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;
  pto_matmul_detail::matmul_bias<Attr>(c, a, b, bias, M, N, K);
}

template <is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          is_local_or_shared_right tile_shape_b,
          is_tile_data_v tile_shape_bias, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_BIAS(tile_shape_c &c, tile_shape_a &a, tile_shape_b &b,
                  tile_shape_bias &bias, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_valid_fixp_attr(Attr), "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_c::DType>(),
                "TMATMUL_BIAS destination dtype does not match PreQuantMode");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(c, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, c);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, c);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, c);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, c);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, c);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_matmul_bias_fixp<Attr, SrcMask, OutMask, IorMode>(c, a, b, bias, row_in, quant_tile, relu_tile, row_out, group_out, quant_gpr, lrelu_gpr, M, N, K);
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
                "require the overload taking fixp::Options");
  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(c, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;
  pto_matmul_detail::matmul_mx<Attr>(c, a, ascale, b, bscale, M, N, K);
}

template <int ScaleMask = 3, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a,
          typename tile_shape_ascale,
          is_local_or_shared_right tile_shape_b,
          typename tile_shape_bscale, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX(tile_shape_c &c, tile_shape_a &a, tile_shape_ascale &ascale,
                tile_shape_b &b, tile_shape_bscale &bscale,
                const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_valid_fixp_attr(Attr), "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_c::DType>(),
                "TMATMUL_MX destination dtype does not match PreQuantMode");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(c, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, c);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, c);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, c);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, c);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, c);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_matmul_mx_fixp<Attr, ScaleMask, SrcMask, OutMask, IorMode>(c, a, ascale, b, bscale, row_in, quant_tile, relu_tile, row_out, group_out, quant_gpr, lrelu_gpr, M, N, K);
}

template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d,
          is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a, is_tile_data_v tile_shape_sa,
          is_local_or_shared_right tile_shape_b, is_tile_data_v tile_shape_sb>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                    tile_shape_sa &scale_a, tile_shape_b &b,
                    tile_shape_sb &scale_b) {
  pto_matmul_detail::validate_matrix_accumulator_contract<Attr,
      tile_shape_d, tile_shape_c, tile_shape_a, tile_shape_b, true>();
  static_assert(is_basic_fixp_attr(Attr),
                "TMATMUL_MX_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the overload taking fixp::Options");
  static_assert(!Attr.CScaleEn,
                "CScale requires the MX_ACC overload with a CScale tile");
  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;
  pto_matmul_detail::matmul_mx_acc<Attr>(d, a, scale_a, b, scale_b, c, M, N, K);
}

template <int ScaleMask = 3, is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a, typename tile_shape_sa,
          is_local_or_shared_right tile_shape_b, typename tile_shape_sb,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                    tile_shape_sa &scale_a, tile_shape_b &b,
                    tile_shape_sb &scale_b, const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  pto_matmul_detail::validate_matrix_accumulator_contract<Attr,
      tile_shape_d, tile_shape_c, tile_shape_a, tile_shape_b, true>();
  static_assert(is_valid_fixp_attr(Attr), "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_d::DType>(),
                "TMATMUL_MX_ACC destination dtype does not match PreQuantMode");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);
  auto &cscale = pto_matmul_detail::select_fixp_operand<Attr.CScaleEn>(
      options.CScale, c);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_matmul_mx_acc_fixp<Attr, ScaleMask, SrcMask, OutMask, IorMode>(
      d, c, a, scale_a, b, scale_b, cscale, row_in, quant_tile, relu_tile,
      row_out, group_out, quant_gpr, lrelu_gpr, M, N, K);
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
                "require the overload taking fixp::Options");
  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;
  pto_matmul_detail::matmul_mx_bias<Attr>(d, a, scale_a, b, scale_b, bias, M, N, K);
}

template <int ScaleMask = 3, is_tile_data_v tile_shape_d,
          is_local_or_shared_left tile_shape_a,
          typename tile_shape_sa,
          is_local_or_shared_right tile_shape_b,
          typename tile_shape_sb, is_tile_data_v tile_shape_bias,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(tile_shape_d &d, tile_shape_a &a,
                     tile_shape_sa &scale_a, tile_shape_b &b,
                     tile_shape_sb &scale_b, tile_shape_bias &bias,
                     const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(is_valid_fixp_attr(Attr), "invalid B.FPATR configuration");
  static_assert(is_fixp_output_type<Attr, typename tile_shape_d::DType>(),
                "TMATMUL_MX_BIAS destination dtype does not match PreQuantMode");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  pto_matmul_detail::MatmulShape __shape =
      pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
  size_t M = __shape.M;
  size_t N = __shape.N;
  size_t K = __shape.K;

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_matmul_mx_bias_fixp<Attr, ScaleMask, SrcMask, OutMask, IorMode>(d, a, scale_a, b, scale_b, bias, row_in, quant_tile, relu_tile, row_out, group_out, quant_gpr, lrelu_gpr, M, N, K);
}

// MX scale presence is owned independently by each input side.  The primary
// itself is passed as an unused compiler operand for an absent scale; the
// constant `.if` in the inline assembly removes that binder completely.
template <is_tile_data_v D, is_local_or_shared_left A,
          is_local_or_shared_right B, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX(D &d, A &a, B &b,
                                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B> &&
                is_basic_fixp_attr(Attr)) {
    auto shape = pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
    pto_matmul_detail::matmul_mx_no_scale_local<Attr>(
        d, a, b, shape.M, shape.N, shape.K);
  } else {
    pto_matmul_detail::NoScaleOperand no_scale;
    TMATMUL_MX<0>(d, a, no_scale, b, no_scale, options);
  }
}
template <is_tile_data_v D, is_local_or_shared_left A,
          is_any_tile_data_v SA, is_local_or_shared_right B,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX(D &d, A &a, SA &sa, B &b,
                                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<SA> &&
                !is_shared_tile_v<B> && is_basic_fixp_attr(Attr)) {
    auto shape = pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
    pto_matmul_detail::matmul_mx_scale_a_local<Attr>(
        d, a, sa, b, shape.M, shape.N, shape.K);
  } else {
    pto_matmul_detail::NoScaleOperand no_scale;
    TMATMUL_MX<1>(d, a, sa, b, no_scale, options);
  }
}
template <is_tile_data_v D, is_local_or_shared_left A,
          is_local_or_shared_right B, is_any_tile_data_v SB,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX(D &d, A &a, B &b, SB &sb,
                                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  if constexpr (!is_shared_tile_v<A> && !is_shared_tile_v<B> &&
                !is_shared_tile_v<SB> && is_basic_fixp_attr(Attr)) {
    auto shape = pto_matmul_detail::resolve_matmul_shape_runtime<Attr>(d, a, b);
    pto_matmul_detail::matmul_mx_scale_b_local<Attr>(
        d, a, b, sb, shape.M, shape.N, shape.K);
  } else {
    pto_matmul_detail::NoScaleOperand no_scale;
    TMATMUL_MX<2>(d, a, no_scale, b, sb, options);
  }
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_or_shared_left A, is_local_or_shared_right B>
PTO_SHARED_INLINE void TMATMUL_MX(D &d, A &a, B &b) {
  TMATMUL_MX(d, a, b, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_or_shared_left A, is_any_tile_data_v SA,
          is_local_or_shared_right B>
PTO_SHARED_INLINE void TMATMUL_MX(D &d, A &a, SA &sa, B &b) {
  TMATMUL_MX(d, a, sa, b, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_or_shared_left A, is_local_or_shared_right B,
          is_any_tile_data_v SB>
PTO_SHARED_INLINE void TMATMUL_MX(D &d, A &a, B &b, SB &sb) {
  TMATMUL_MX(d, a, b, sb, fixp::Options<Attr>{});
}

template <is_tile_data_v D, is_tile_data_v C,
          is_local_or_shared_left A, is_local_or_shared_right B,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b,
                                      const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TMATMUL_MX_ACC<0>(d, c, a, no_scale, b, no_scale, options);
}
template <is_tile_data_v D, is_tile_data_v C,
          is_local_or_shared_left A, is_any_tile_data_v SA,
          is_local_or_shared_right B, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, SA &sa, B &b,
                                      const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TMATMUL_MX_ACC<1>(d, c, a, sa, b, no_scale, options);
}
template <is_tile_data_v D, is_tile_data_v C,
          is_local_or_shared_left A, is_local_or_shared_right B,
          is_any_tile_data_v SB, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b, SB &sb,
                                      const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TMATMUL_MX_ACC<2>(d, c, a, no_scale, b, sb, options);
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D, is_tile_data_v C,
          is_local_or_shared_left A, is_local_or_shared_right B>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b) {
  TMATMUL_MX_ACC(d, c, a, b, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D, is_tile_data_v C,
          is_local_or_shared_left A, is_any_tile_data_v SA,
          is_local_or_shared_right B>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, SA &sa, B &b) {
  TMATMUL_MX_ACC(d, c, a, sa, b, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D, is_tile_data_v C,
          is_local_or_shared_left A, is_local_or_shared_right B,
          is_any_tile_data_v SB>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b, SB &sb) {
  TMATMUL_MX_ACC(d, c, a, b, sb, fixp::Options<Attr>{});
}

template <is_tile_data_v D, is_local_or_shared_left A,
          is_local_or_shared_right B, is_tile_data_v Bias,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(D &d, A &a, B &b, Bias &bias,
                                       const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TMATMUL_MX_BIAS<0>(d, a, no_scale, b, no_scale, bias, options);
}
template <is_tile_data_v D, is_local_or_shared_left A,
          is_any_tile_data_v SA, is_local_or_shared_right B,
          is_tile_data_v Bias, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(D &d, A &a, SA &sa, B &b, Bias &bias,
                                       const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TMATMUL_MX_BIAS<1>(d, a, sa, b, no_scale, bias, options);
}
template <is_tile_data_v D, is_local_or_shared_left A,
          is_local_or_shared_right B, is_any_tile_data_v SB,
          is_tile_data_v Bias, fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(D &d, A &a, B &b, SB &sb, Bias &bias,
                                       const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TMATMUL_MX_BIAS<2>(d, a, no_scale, b, sb, bias, options);
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_or_shared_left A, is_local_or_shared_right B,
          is_tile_data_v Bias>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(D &d, A &a, B &b, Bias &bias) {
  TMATMUL_MX_BIAS(d, a, b, bias, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_or_shared_left A, is_any_tile_data_v SA,
          is_local_or_shared_right B, is_tile_data_v Bias>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(D &d, A &a, SA &sa, B &b, Bias &bias) {
  TMATMUL_MX_BIAS(d, a, sa, b, bias, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_or_shared_left A, is_local_or_shared_right B,
          is_any_tile_data_v SB, is_tile_data_v Bias>
PTO_SHARED_INLINE void TMATMUL_MX_BIAS(D &d, A &a, B &b, SB &sb, Bias &bias) {
  TMATMUL_MX_BIAS(d, a, b, sb, bias, fixp::Options<Attr>{});
}

// ---- TGEMV family (Function 16-18, 20-22) ----
template <is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV(tile_shape_d &d, tile_shape_mtx &mtx,
                             tile_shape_vec &vec, const Options &options);

template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec>
PTO_SHARED_INLINE void TGEMV(tile_shape_d &d, tile_shape_mtx &mtx,
                 tile_shape_vec &vec) {
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");
  static_assert(is_basic_fixp_attr(Attr),
                "TGEMV supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the Options overload");
  fixp::Options<Attr> canonical;
  TGEMV(d, mtx, vec, canonical);
}


template <is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV(tile_shape_d &d, tile_shape_mtx &mtx,
                 tile_shape_vec &vec,
                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  // TGEMV is M=1: vec is a 1-row Left tile; mtx is KxN.
  size_t M = 1;
  size_t N = pto_matmul_detail::matrix_valid_col(mtx);
  size_t K = pto_matmul_detail::matrix_valid_row(mtx);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_gemv_fixp<Attr, SrcMask, OutMask, IorMode>(
      d, mtx, vec,
      row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}


template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec,
          is_tile_data_v tile_shape_bias>
PTO_SHARED_INLINE void TGEMV_BIAS(tile_shape_d &d, tile_shape_mtx &mtx,
                 tile_shape_vec &vec,
                 tile_shape_bias &bias) {
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV_BIAS requires vec=Left (1xK) and mtx=Right (KxN)");
  static_assert(is_basic_fixp_attr(Attr),
                "TGEMV_BIAS supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the Options overload");
  fixp::Options<Attr> canonical;
  TGEMV_BIAS(d, mtx, vec, bias, canonical);
}


template <is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec,
          is_tile_data_v tile_shape_bias, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_BIAS(tile_shape_d &d, tile_shape_mtx &mtx,
                 tile_shape_vec &vec,
                 tile_shape_bias &bias,
                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  // TGEMV is M=1: vec is a 1-row Left tile; mtx is KxN.
  size_t M = 1;
  size_t N = pto_matmul_detail::matrix_valid_col(mtx);
  size_t K = pto_matmul_detail::matrix_valid_row(mtx);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_gemv_bias_fixp<Attr, SrcMask, OutMask, IorMode>(
      d, mtx, vec, bias,
      row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}


template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec>
PTO_SHARED_INLINE void TGEMV_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_mtx &mtx,
                 tile_shape_vec &vec) {
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV_ACC requires vec=Left (1xK) and mtx=Right (KxN)");
  static_assert(is_basic_fixp_attr(Attr),
                "TGEMV_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the Options overload");
  fixp::Options<Attr> canonical;
  TGEMV_ACC(d, c, mtx, vec, canonical);
}


template <is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_vec, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_mtx &mtx,
                 tile_shape_vec &vec,
                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  // TGEMV is M=1: vec is a 1-row Left tile; mtx is KxN.
  size_t M = 1;
  size_t N = pto_matmul_detail::matrix_valid_col(mtx);
  size_t K = pto_matmul_detail::matrix_valid_row(mtx);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_gemv_acc_fixp<Attr, SrcMask, OutMask, IorMode>(
      d, c, mtx, vec,
      row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}


template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_smtx,
          is_local_tile_v tile_shape_vec,
          is_local_tile_v tile_shape_svec>
PTO_SHARED_INLINE void TGEMV_MX(tile_shape_d &d, tile_shape_mtx &mtx, tile_shape_smtx &smtx,
                 tile_shape_vec &vec, tile_shape_svec &svec) {
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV_MX requires vec=Left (1xK) and mtx=Right (KxN)");
  static_assert(is_basic_fixp_attr(Attr),
                "TGEMV_MX supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the Options overload");
  fixp::Options<Attr> canonical;
  TGEMV_MX(d, mtx, smtx, vec, svec, canonical);
}


template <int ScaleMask = 3, is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          typename tile_shape_smtx,
          is_local_tile_v tile_shape_vec,
          typename tile_shape_svec, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_MX(tile_shape_d &d, tile_shape_mtx &mtx, tile_shape_smtx &smtx,
                 tile_shape_vec &vec, tile_shape_svec &svec,
                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  // TGEMV is M=1: vec is a 1-row Left tile; mtx is KxN.
  size_t M = 1;
  size_t N = pto_matmul_detail::matrix_valid_col(mtx);
  size_t K = pto_matmul_detail::matrix_valid_row(mtx);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_gemv_mx_fixp<Attr, ScaleMask, SrcMask, OutMask, IorMode>(
      d, mtx, smtx, vec, svec,
      row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}


template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_smtx,
          is_local_tile_v tile_shape_vec,
          is_local_tile_v tile_shape_svec,
          is_tile_data_v tile_shape_bias>
PTO_SHARED_INLINE void TGEMV_MX_BIAS(tile_shape_d &d, tile_shape_mtx &mtx, tile_shape_smtx &smtx,
                 tile_shape_vec &vec, tile_shape_svec &svec,
                 tile_shape_bias &bias) {
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV_MX_BIAS requires vec=Left (1xK) and mtx=Right (KxN)");
  static_assert(is_basic_fixp_attr(Attr),
                "TGEMV_MX_BIAS supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the Options overload");
  fixp::Options<Attr> canonical;
  TGEMV_MX_BIAS(d, mtx, smtx, vec, svec, bias, canonical);
}


template <int ScaleMask = 3, is_tile_data_v tile_shape_d, is_local_tile_v tile_shape_mtx,
          typename tile_shape_smtx,
          is_local_tile_v tile_shape_vec,
          typename tile_shape_svec,
          is_tile_data_v tile_shape_bias, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_MX_BIAS(tile_shape_d &d, tile_shape_mtx &mtx, tile_shape_smtx &smtx,
                 tile_shape_vec &vec, tile_shape_svec &svec,
                 tile_shape_bias &bias,
                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  // TGEMV is M=1: vec is a 1-row Left tile; mtx is KxN.
  size_t M = 1;
  size_t N = pto_matmul_detail::matrix_valid_col(mtx);
  size_t K = pto_matmul_detail::matrix_valid_row(mtx);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_gemv_mx_bias_fixp<Attr, ScaleMask, SrcMask, OutMask, IorMode>(
      d, mtx, smtx, vec, svec, bias,
      row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}


template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_tile_v tile_shape_mtx,
          is_local_tile_v tile_shape_smtx,
          is_local_tile_v tile_shape_vec,
          is_local_tile_v tile_shape_svec>
PTO_SHARED_INLINE void TGEMV_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_mtx &mtx, tile_shape_smtx &smtx,
                 tile_shape_vec &vec, tile_shape_svec &svec) {
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV_MX_ACC requires vec=Left (1xK) and mtx=Right (KxN)");
  static_assert(is_basic_fixp_attr(Attr),
                "TGEMV_MX_ACC supports only parameter-free FPATR options "
                "(keep_acc/f16/bf16/relu); quant, PReLU, RowMax and GroupMax "
                "require the Options overload");
  fixp::Options<Attr> canonical;
  TGEMV_MX_ACC(d, c, mtx, smtx, vec, svec, canonical);
}


template <int ScaleMask = 3, is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_tile_v tile_shape_mtx,
          typename tile_shape_smtx,
          is_local_tile_v tile_shape_vec,
          typename tile_shape_svec, fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_mtx &mtx, tile_shape_smtx &smtx,
                 tile_shape_vec &vec, tile_shape_svec &svec,
                  const Options &options) {
  constexpr FixpAttr Attr = Options::Attr;
  static_assert(tile_role_v<tile_shape_vec> == Location::Left &&
                    tile_role_v<tile_shape_mtx> == Location::Right,
                "TGEMV requires vec=Left (1xK) and mtx=Right (KxN)");

  constexpr bool HasVectorQuant = is_vector_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasScalarQuant = is_scalar_fixp_pre_quant(Attr.PreQuant);
  constexpr bool HasRowIn = Attr.RowMaxInit;
  constexpr bool HasRowOut = Attr.RowMaxEn;
  constexpr bool HasGroupOut = Attr.GroupMaxEn;
  constexpr bool HasPRelu = Attr.Relu == FixpReluMode::PRelu;
  constexpr int SrcMask = (HasRowIn ? 1 : 0) | (HasVectorQuant ? 2 : 0) | (HasPRelu ? 4 : 0);
  constexpr int OutMask = (HasRowOut ? 1 : 0) | (HasGroupOut ? 2 : 0);
  constexpr int IorMode = (HasScalarQuant ? 1 : 0) | (Attr.Relu == FixpReluMode::LRelu ? 2 : 0);

  static_assert(HasVectorQuant == !std::is_same_v<typename Options::QuantTile, fixp::NoOperand>,
                "vector PreQuant mode requires a quant parameter Tile");
  static_assert(HasPRelu == !std::is_same_v<typename Options::ReluTile, fixp::NoOperand>,
                "PRelu mode requires a PReLU parameter Tile");
  static_assert(HasRowIn == !std::is_same_v<typename Options::RowMaxIn, fixp::NoOperand>,
                "RowMaxInit requires a RowMaxIn Tile");
  static_assert(HasRowOut == !std::is_same_v<typename Options::RowMaxOut, fixp::NoOperand>,
                "RowMaxEn requires a RowMaxOut Tile");
  static_assert(HasGroupOut == !std::is_same_v<typename Options::GroupMaxOut, fixp::NoOperand>,
                "GroupMaxEn requires a GroupMaxOut Tile");

  // TGEMV is M=1: vec is a 1-row Left tile; mtx is KxN.
  size_t M = 1;
  size_t N = pto_matmul_detail::matrix_valid_col(mtx);
  size_t K = pto_matmul_detail::matrix_valid_row(mtx);

  auto &row_in = pto_matmul_detail::select_fixp_operand<HasRowIn>(options.RowIn, d);
  auto &quant_tile = pto_matmul_detail::select_fixp_operand<HasVectorQuant>(options.Quant, d);
  auto &relu_tile = pto_matmul_detail::select_fixp_operand<HasPRelu>(options.Relu, d);
  auto &row_out = pto_matmul_detail::select_fixp_operand<HasRowOut>(options.RowOut, d);
  auto &group_out = pto_matmul_detail::select_fixp_operand<HasGroupOut>(options.GroupOut, d);

  volatile uint64_t quant_gpr = options.QuantDescriptor;
  volatile uint64_t lrelu_gpr = options.LReluDescriptor;
  pto_matmul_detail::emit_gemv_mx_acc_fixp<Attr, ScaleMask, SrcMask, OutMask, IorMode>(
      d, c, mtx, smtx, vec, svec,
      row_in, quant_tile, relu_tile, row_out, group_out,
      quant_gpr, lrelu_gpr, M, N, K);
}

template <is_tile_data_v D, is_local_tile_v Mtx, is_local_tile_v Vec,
          fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, Vec &vec,
                                const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX<0>(d, mtx, no_scale, vec, no_scale, options);
}
template <is_tile_data_v D, is_local_tile_v Mtx, is_local_tile_v Vec,
          is_local_tile_v ScaleVec, fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, Vec &vec, ScaleVec &scale_vec,
                                const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX<1>(d, mtx, no_scale, vec, scale_vec, options);
}
template <is_tile_data_v D, is_local_tile_v Mtx, is_local_tile_v ScaleMtx,
          is_local_tile_v Vec, fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, ScaleMtx &scale_mtx, Vec &vec,
                                const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX<2>(d, mtx, scale_mtx, vec, no_scale, options);
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_tile_v Mtx, is_local_tile_v Vec>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, Vec &vec) {
  TGEMV_MX(d, mtx, vec, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_tile_v Mtx, is_local_tile_v Vec,
          is_local_tile_v ScaleVec>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, Vec &vec,
                                ScaleVec &scale_vec) {
  TGEMV_MX(d, mtx, vec, scale_vec, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_tile_v Mtx, is_local_tile_v ScaleMtx,
          is_local_tile_v Vec>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, ScaleMtx &scale_mtx,
                                Vec &vec) {
  TGEMV_MX(d, mtx, scale_mtx, vec, fixp::Options<Attr>{});
}

template <is_tile_data_v D, is_tile_data_v C, is_local_tile_v Mtx,
          is_local_tile_v Vec, fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_ACC(D &d, C &c, Mtx &mtx, Vec &vec,
                                    const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX_ACC<0>(d, c, mtx, no_scale, vec, no_scale, options);
}
template <is_tile_data_v D, is_tile_data_v C, is_local_tile_v Mtx,
          is_local_tile_v Vec, is_local_tile_v ScaleVec,
          fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_ACC(D &d, C &c, Mtx &mtx, Vec &vec,
                                    ScaleVec &scale_vec,
                                    const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX_ACC<1>(d, c, mtx, no_scale, vec, scale_vec, options);
}
template <is_tile_data_v D, is_tile_data_v C, is_local_tile_v Mtx,
          is_local_tile_v ScaleMtx, is_local_tile_v Vec,
          fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_ACC(D &d, C &c, Mtx &mtx,
                                    ScaleMtx &scale_mtx, Vec &vec,
                                    const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX_ACC<2>(d, c, mtx, scale_mtx, vec, no_scale, options);
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D, is_tile_data_v C,
          is_local_tile_v Mtx, is_local_tile_v Vec>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_ACC(D &d, C &c, Mtx &mtx, Vec &vec) {
  TGEMV_MX_ACC(d, c, mtx, vec, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D, is_tile_data_v C,
          is_local_tile_v Mtx, is_local_tile_v Vec,
          is_local_tile_v ScaleVec>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_ACC(D &d, C &c, Mtx &mtx, Vec &vec,
                                    ScaleVec &scale_vec) {
  TGEMV_MX_ACC(d, c, mtx, vec, scale_vec, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D, is_tile_data_v C,
          is_local_tile_v Mtx, is_local_tile_v ScaleMtx,
          is_local_tile_v Vec>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_ACC(D &d, C &c, Mtx &mtx,
                                    ScaleMtx &scale_mtx, Vec &vec) {
  TGEMV_MX_ACC(d, c, mtx, scale_mtx, vec, fixp::Options<Attr>{});
}

template <is_tile_data_v D, is_local_tile_v Mtx, is_local_tile_v Vec,
          is_tile_data_v Bias, fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, Vec &vec, Bias &bias,
                                     const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX_BIAS<0>(d, mtx, no_scale, vec, no_scale, bias, options);
}
template <is_tile_data_v D, is_local_tile_v Mtx, is_local_tile_v Vec,
          is_local_tile_v ScaleVec, is_tile_data_v Bias,
          fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, Vec &vec,
                                     ScaleVec &scale_vec, Bias &bias,
                                     const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX_BIAS<1>(d, mtx, no_scale, vec, scale_vec, bias, options);
}
template <is_tile_data_v D, is_local_tile_v Mtx,
          is_local_tile_v ScaleMtx, is_local_tile_v Vec, is_tile_data_v Bias,
          fixp::is_options_v Options>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, ScaleMtx &scale_mtx,
                                     Vec &vec, Bias &bias,
                                     const Options &options) {
  pto_matmul_detail::NoScaleOperand no_scale;
  TGEMV_MX_BIAS<2>(d, mtx, scale_mtx, vec, no_scale, bias, options);
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_tile_v Mtx, is_local_tile_v Vec, is_tile_data_v Bias>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, Vec &vec, Bias &bias) {
  TGEMV_MX_BIAS(d, mtx, vec, bias, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_tile_v Mtx, is_local_tile_v Vec,
          is_local_tile_v ScaleVec, is_tile_data_v Bias>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, Vec &vec,
                                     ScaleVec &scale_vec, Bias &bias) {
  TGEMV_MX_BIAS(d, mtx, vec, scale_vec, bias, fixp::Options<Attr>{});
}
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v D,
          is_local_tile_v Mtx, is_local_tile_v ScaleMtx,
          is_local_tile_v Vec, is_tile_data_v Bias>
  requires(tile_role_v<Mtx> == Location::Right &&
           tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, ScaleMtx &scale_mtx,
                                     Vec &vec, Bias &bias) {
  TGEMV_MX_BIAS(d, mtx, scale_mtx, vec, bias, fixp::Options<Attr>{});
}


#undef PTO_SHARED_INLINE


//===--- TEPL Mode 0: tile-tile elementwise ops (BSTART.TEPL) ---===//
// opcode = Mode(0) * 32 + Function. One-layer inline-asm, no __vec__ kernel.

// TADD: dst = src0 + src1
template <is_tile_data_v tile_shape>
void TADD(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 0, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 1, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 2, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 3, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 4, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TFMOD is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TAND: dst = src0 & src1
template <is_tile_data_v tile_shape>
void TAND(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  asm volatile(
    "BSTART.TEPL 6, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 7, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 8, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 9, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 10, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 11, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 12, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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

// TCMP: compare src0 and src1, write packed predicate. The comparison mode is
// a compile-time template parameter encoded into B.DATR CMode[31:29]; the
// zero-arg form is kept as a deprecated EQ default (PTO 0.58).
template <CmpMode Mode, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void TCMP(tile_shape_out &dst, tile_shape_in &src0, tile_shape_in &src1) {
  static_assert(is_valid_cmp_mode(Mode), "TCMP requires a valid CmpMode");
  static_assert(tile_shape_in::Rows == tile_shape_out::Rows &&
                    tile_shape_in::Cols == tile_shape_out::Cols,
                "TCMP output shape must match input shape");
  if constexpr (Mode == CmpMode::EQ) {
    asm volatile(
      "BSTART.TEPL 13, %D[TCode]\n"
      "B.DATR Zero, EQ\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S0], %[S1], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src0.GetValidCol()),
        [VROW] "r"(src0.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S0] "Tr"(src0.data()),
        [S1] "Tr"(src1.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
    );
  } else if constexpr (Mode == CmpMode::NE) {
    asm volatile(
      "BSTART.TEPL 13, %D[TCode]\n"
      "B.DATR Zero, NE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S0], %[S1], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src0.GetValidCol()),
        [VROW] "r"(src0.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S0] "Tr"(src0.data()),
        [S1] "Tr"(src1.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
    );
  } else if constexpr (Mode == CmpMode::LT) {
    asm volatile(
      "BSTART.TEPL 13, %D[TCode]\n"
      "B.DATR Zero, LT\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S0], %[S1], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src0.GetValidCol()),
        [VROW] "r"(src0.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S0] "Tr"(src0.data()),
        [S1] "Tr"(src1.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
    );
  } else if constexpr (Mode == CmpMode::GT) {
    asm volatile(
      "BSTART.TEPL 13, %D[TCode]\n"
      "B.DATR Zero, GT\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S0], %[S1], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src0.GetValidCol()),
        [VROW] "r"(src0.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S0] "Tr"(src0.data()),
        [S1] "Tr"(src1.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
    );
  } else if constexpr (Mode == CmpMode::LE) {
    asm volatile(
      "BSTART.TEPL 13, %D[TCode]\n"
      "B.DATR Zero, LE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S0], %[S1], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src0.GetValidCol()),
        [VROW] "r"(src0.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S0] "Tr"(src0.data()),
        [S1] "Tr"(src1.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
    );
  } else if constexpr (Mode == CmpMode::GE) {
    asm volatile(
      "BSTART.TEPL 13, %D[TCode]\n"
      "B.DATR Zero, GE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S0], %[S1], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src0.GetValidCol()),
        [VROW] "r"(src0.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S0] "Tr"(src0.data()),
        [S1] "Tr"(src1.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
    );
  }
}

// Deprecated EQ-default form retained for old callers.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCMP(tile_shape_out &dst, tile_shape_in &src0, tile_shape_in &src1) {
  TCMP<CmpMode::EQ>(dst, src0, src1);
}

// TPRELU: parametric ReLU with per-element slope
template <is_tile_data_v tile_shape>
void TPRELU(tile_shape &dst, tile_shape &src0, tile_shape &src1) {
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TPRELU is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TSEL: select true_src where mask is set, otherwise preserve the prior dst.
// PTO requires the false source to be bound explicitly.  Model the public
// in-place API as two B.IOT bindings and keep dst read/write so the second
// binding snapshots its old value before publishing the new destination.
template <is_tile_data_v tile_shape>
void TSEL(tile_shape &dst, tile_shape &mask, tile_shape &true_src) {
  asm volatile(
    "BSTART.TEPL 26, %D[DataType]\n"
    "B.DIM %[ValidCol], 0, ->lb0\n"
    "B.DIM %[ValidRow], 0, ->lb1\n"
    "B.DIM zero, %c[Cols], ->lb2\n"
    "B.IOT %[Mask], %[True], mask=1111\n"
    "B.IOT %[Prior], mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
    ""
    : [Dst] "=Tr"(dst.data())
    : [Prior] "0"(dst.data()),
      [DataType] "i"(type_traits<typename tile_shape::DType>::TypeCode),
      [ValidCol] "r"(mask.GetValidCol()),
      [ValidRow] "r"(mask.GetValidRow()),
      [Cols] "i"(tile_shape::Cols),
      [Mask] "Tr"(mask.data()),
      [True] "Tr"(true_src.data()),
      [TileSize] "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode)
  );
}

// TABS: dst = |src|
template <is_tile_data_v tile_shape>
void TABS(tile_shape &dst, tile_shape &src) {
  asm volatile(
    "BSTART.TEPL 15, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 16, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 17, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 18, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 19, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 20, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 21, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 22, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 23, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TADDC is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TSUBC: dst = src0 - src1 + src2
template <is_tile_data_v tile_shape>
void TSUBC(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2) {
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TSUBC is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
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
    "BSTART.TEPL 32, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 33, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 34, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 35, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 36, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TFMODS is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TANDS: dst = src & scalar
template <is_tile_data_v tile_shape>
void TANDS(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 38, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 39, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 40, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 41, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 42, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 43, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 44, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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

// TCMPS: compare src with scalar. The comparison mode is a compile-time
// template parameter encoded into B.DATR CMode[31:29]; scalar travels via the
// canonical B.IOR slot, never as a Tile source (PTO 0.58).
template <CmpMode Mode, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void TCMPS(tile_shape_out &dst, tile_shape_in &src,
           typename tile_shape_in::DType s) {
  static_assert(is_valid_cmp_mode(Mode), "TCMPS requires a valid CmpMode");
  static_assert(tile_shape_in::Rows == tile_shape_out::Rows &&
                    tile_shape_in::Cols == tile_shape_out::Cols,
                "TCMPS output shape must match input shape");
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape_in::DType sv = s;
  if constexpr (Mode == CmpMode::EQ) {
    asm volatile(
      "BSTART.TEPL 45, %D[TCode]\n"
      "B.DATR Zero, EQ\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      "B.IOR [%[Scalar]],[]\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()),
        [VROW] "r"(src.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S] "Tr"(src.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [Scalar] "r"(sv)
    );
  } else if constexpr (Mode == CmpMode::NE) {
    asm volatile(
      "BSTART.TEPL 45, %D[TCode]\n"
      "B.DATR Zero, NE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      "B.IOR [%[Scalar]],[]\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()),
        [VROW] "r"(src.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S] "Tr"(src.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [Scalar] "r"(sv)
    );
  } else if constexpr (Mode == CmpMode::LT) {
    asm volatile(
      "BSTART.TEPL 45, %D[TCode]\n"
      "B.DATR Zero, LT\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      "B.IOR [%[Scalar]],[]\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()),
        [VROW] "r"(src.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S] "Tr"(src.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [Scalar] "r"(sv)
    );
  } else if constexpr (Mode == CmpMode::GT) {
    asm volatile(
      "BSTART.TEPL 45, %D[TCode]\n"
      "B.DATR Zero, GT\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      "B.IOR [%[Scalar]],[]\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()),
        [VROW] "r"(src.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S] "Tr"(src.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [Scalar] "r"(sv)
    );
  } else if constexpr (Mode == CmpMode::LE) {
    asm volatile(
      "BSTART.TEPL 45, %D[TCode]\n"
      "B.DATR Zero, LE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      "B.IOR [%[Scalar]],[]\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()),
        [VROW] "r"(src.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S] "Tr"(src.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [Scalar] "r"(sv)
    );
  } else if constexpr (Mode == CmpMode::GE) {
    asm volatile(
      "BSTART.TEPL 45, %D[TCode]\n"
      "B.DATR Zero, GE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Cols], ->lb2\n"
      "B.IOT %[S], mask=1111, last, ->%[D]<%Z[TSize]>\n"
      "B.IOR [%[Scalar]],[]\n"
      ""
      : [D] "=Tr"(dst.data())
      : [TCode] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()),
        [VROW] "r"(src.GetValidRow()),
        [Cols] "i"(tile_shape_in::Cols),
        [S] "Tr"(src.data()),
        [TSize] "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode),
        [Scalar] "r"(sv)
    );
  }
}

// Deprecated EQ-default form retained for old callers.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TCMPS(tile_shape_out &dst, tile_shape_in &src,
           typename tile_shape_in::DType s) {
  TCMPS<CmpMode::EQ>(dst, src, s);
}

// TLRELU: leaky ReLU with scalar slope
template <is_tile_data_v tile_shape>
void TLRELU(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TLRELU is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TAXPY: AXPY-style fused update (DavinciOO ext)
template <is_tile_data_v tile_shape>
void TAXPY(tile_shape &dst, tile_shape &src, typename tile_shape::DType s) {
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TAXPY is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TADDSC: dst = src0 + scalar + src1
template <is_tile_data_v tile_shape>
void TADDSC(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1) {
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TADDSC is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TSUBSC: dst = src0 - scalar + src1
template <is_tile_data_v tile_shape>
void TSUBSC(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1) {
  static_assert(pto_dependent_false_v<tile_shape, tile_shape>,
                "TSUBSC is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TSELS: select between src tile and scalar using mask
template <is_tile_data_v tile_shape>
void TSELS(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1) {
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType sv = s;
  asm volatile(
    "BSTART.TEPL 58, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 59, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT mask=1111, last, ->%0<%Z5>\n"
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
    "BSTART.TEPL 28, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111\n"
    "B.IOT %7, mask=1111, last, ->%0<%Z8>\n"
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
    "BSTART.TEPL 98, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 99, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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

// TIMG2COL: image-to-column with feature-map posM/posK (PTO ISA 0.58.3 TEPL
// Mode3 Fn4 / selector 0x064). B.IOR carries PosMGPR, PosKGPR (low 16 bits
// each per B4; the spec's optional B.IOR defaults to posM=posK=0). The
// source's feature-map descriptor (NC1HWC0 / NDC1HWC0 layout, filter,
// stride, dilation, padding) is a property of the persistent Matrix-location
// source tile; TileOP exposes the position selectors via B.IOR.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TIMG2COL(tile_shape_out &dst, tile_shape_in &src, uint32_t posM = 0,
              uint32_t posK = 0) {
  // low 16 bits of each position selector (B4: only low 16 bits encoded).
  volatile uint32_t posM_v = posM & 0xffffu;
  volatile uint32_t posK_v = posK & 0xffffu;
  asm volatile(
    "BSTART.TEPL 100, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOR [%5, %6], []\n"
    "B.IOT %7, mask=1111, last, ->%0<%Z8>\n"
    ""
    : "=Tr"(dst.data())
    : "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
      "r"(src.GetValidCol()),
      "r"(src.GetValidRow()),
      "i"(tile_shape_in::Cols),
      "r"(posM_v), "r"(posK_v),
      "Tr"(src.data()),
      "i"(tile_type_traits<typename tile_shape_out::TileDType>::TilesizeCode)
  );
}

// TFILLPAD: copy valid region and fill padding
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TFILLPAD(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 101, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
template <is_tile_data_v tile_shape, typename T, int descending = 0>
void TCI(tile_shape &dst, T s) {
  static_assert(std::is_same<typename tile_shape::DType, T>::value,
                "TCI destination and start must have the same type");
  static_assert(descending == 0 || descending == 1,
                "TCI direction must be ascending (0) or descending (1)");
  static_assert(tile_shape::Loc == Location::Vec,
                "TCI requires a Local vector tile");
  static_assert(tile_shape::isRowMajor && !tile_shape::isBoxedLayout,
                "TCI requires an unboxed RowMajor tile");
  static_assert(tile_shape::ValidRow == 1,
                "TCI requires ValidRow == 1");
  static_assert(tile_shape::ValidCol > 0 &&
                    tile_shape::Cols >= tile_shape::ValidCol,
                "TCI requires 0 < ValidCol <= Cols");
  static_assert(std::is_same<T, int32_t>::value ||
                    std::is_same<T, int16_t>::value ||
                    std::is_same<T, uint32_t>::value ||
                    std::is_same<T, uint16_t>::value,
                "TCI supports only S32, S16, U32, and U16");
  // Anti-fold: keep a compile-time-constant scalar (e.g. 0) off the zero
  // register so B.IOR [zero],[] still matches an instruction.
  volatile typename tile_shape::DType startValue = s;
  volatile uint32_t directionValue = descending;
  asm volatile(
    "BSTART.TEPL 102, %D[DataType]\n"
    "B.DIM %[ValidCol], 0, ->lb0\n"
    "B.DIM zero, %c[PhysicalCol], ->lb2\n"
    "B.IOR [%[Start],%[Direction]],[]\n"
    "B.IOT mask=1111, last, ->%[Dst]<%Z[TileSize]>\n"
    : [Dst] "=Tr"(dst.data())
    : [DataType] "i"(type_traits<typename tile_shape::DType>::TypeCode),
      [ValidCol] "r"(dst.GetValidCol()),
      [PhysicalCol] "i"(tile_shape::Cols),
      [TileSize] "i"(tile_type_traits<typename tile_shape::TileDType>::TilesizeCode),
      [Start] "r"(startValue),
      [Direction] "r"(directionValue)
  );
}

// TTRI: triangular mask generation
template <is_tile_data_v tile_shape>
void TTRI(tile_shape &dst) {
  asm volatile(
    "BSTART.TEPL 103, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT mask=1111, last, ->%0<%Z5>\n"
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
  static_assert(pto_dependent_false_v<tile_shape>,
                "TRANDOM is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


// TQUANT: FP32 -> S8/U8 quantization with B.DATR RMode/Sat and B.IOR
// multiplier/zero-point (PTO ISA 0.58.3 TEPL Mode3 Fn10 / selector 0x06A).
// RMode and Sat are B.DATR fields: RMode accepts a numeric immediate (the
// parser maps mnemonic or numeric), Sat only the NOSAT/SAT token, so Sat is
// selected with if constexpr. The multiplier travels as its raw FP32 bits in
// a GPR and zeroPoint in another.
template <RoundMode Mode = RoundMode::RNE, bool Saturate = false,
          is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TQUANT(tile_shape_out &dst, tile_shape_in &src, float multiplier = 1.0f,
            int32_t zeroPoint = 0) {
  static_assert(is_valid_round_mode(Mode), "TQUANT invalid rounding mode");
  static_assert(type_traits<typename tile_shape_in::DType>::TypeCode ==
                        __type_fp32,
                "TQUANT source must be FP32");
  static_assert(type_traits<typename tile_shape_out::DType>::TypeCode ==
                            __type_int8 ||
                    type_traits<typename tile_shape_out::DType>::TypeCode ==
                        __type_uint8,
                "TQUANT destination must be S8 or U8");
  static_assert(tile_shape_out::Loc == Location::Vec &&
                    tile_shape_in::Loc == Location::Vec &&
                    tile_shape_out::isRowMajor &&
                    tile_shape_in::isRowMajor &&
                    !tile_shape_out::isBoxedLayout &&
                    !tile_shape_in::isBoxedLayout,
                "TQUANT operands must be Local RowMajor numeric Tiles");
  static_assert(
      tile_shape_out::Rows == tile_shape_in::Rows &&
          tile_shape_out::Cols == tile_shape_in::Cols &&
          (tile_shape_out::ValidRow == DYNAMIC ||
           tile_shape_in::ValidRow == DYNAMIC ||
           tile_shape_out::ValidRow == tile_shape_in::ValidRow) &&
          (tile_shape_out::ValidCol == DYNAMIC ||
           tile_shape_in::ValidCol == DYNAMIC ||
           tile_shape_out::ValidCol == tile_shape_in::ValidCol),
      "TQUANT source and destination logical shapes must match");
  uint32_t multiplierBits;
  __builtin_memcpy(&multiplierBits, &multiplier, sizeof(multiplier));
  volatile uint32_t mult = multiplierBits;
  volatile int32_t zp = zeroPoint;
  if constexpr (Mode == RoundMode::RNE && Saturate) {
    asm volatile(
      "BSTART.TEPL 106, %D[SType]\n"
      // LLVM currently names encoded RMode zero RNONE. PTO ISA 0.58.3 defines
      // that encoding as the operation default, which is RNE for TQUANT.
      "B.DATR %D[__pto_DstType], RNONE, sat\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Col], ->lb2\n"
      "B.IOR [%[Mult], %[ZP]], []\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [SType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [__pto_DstType] "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()), [VROW] "r"(src.GetValidRow()),
        [Col] "i"(tile_shape_in::Cols),
        [Mult] "r"(mult), [ZP] "r"(zp),
        [DstSize] "i"(tile_shape_out::TilesizeCode)
    );
  } else if constexpr (Mode == RoundMode::RNE) {
    asm volatile(
      "BSTART.TEPL 106, %D[SType]\n"
      "B.DATR %D[__pto_DstType], RNONE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Col], ->lb2\n"
      "B.IOR [%[Mult], %[ZP]], []\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [SType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [__pto_DstType] "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        [VCOL] "r"(src.GetValidCol()), [VROW] "r"(src.GetValidRow()),
        [Col] "i"(tile_shape_in::Cols),
        [Mult] "r"(mult), [ZP] "r"(zp),
        [DstSize] "i"(tile_shape_out::TilesizeCode)
    );
  } else if constexpr (Saturate) {
    asm volatile(
      "BSTART.TEPL 106, %D[SType]\n"
      PTO_RMODE_DATR_ASM(", sat\n")
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Col], ->lb2\n"
      "B.IOR [%[Mult], %[ZP]], []\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [SType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [__pto_DstType] "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        [RMode] "i"(static_cast<unsigned>(Mode)),
        [VCOL] "r"(src.GetValidCol()), [VROW] "r"(src.GetValidRow()),
        [Col] "i"(tile_shape_in::Cols),
        [Mult] "r"(mult), [ZP] "r"(zp),
        [DstSize] "i"(tile_shape_out::TilesizeCode)
    );
  } else {
    asm volatile(
      "BSTART.TEPL 106, %D[SType]\n"
      PTO_RMODE_DATR_ASM("\n")
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Col], ->lb2\n"
      "B.IOR [%[Mult], %[ZP]], []\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [SType] "i"(type_traits<typename tile_shape_in::DType>::TypeCode),
        [__pto_DstType] "i"(type_traits<typename tile_shape_out::DType>::TypeCode),
        [RMode] "i"(static_cast<unsigned>(Mode)),
        [VCOL] "r"(src.GetValidCol()), [VROW] "r"(src.GetValidRow()),
        [Col] "i"(tile_shape_in::Cols),
        [Mult] "r"(mult), [ZP] "r"(zp),
        [DstSize] "i"(tile_shape_out::TilesizeCode)
    );
  }
}

// TDEQUANT: S8/U8 -> FP32 dequantization (TEPL Mode3 Fn11 / 0x06B).
// B.DATR carries only FP32 + RMode (Sat is always false per spec).
template <RoundMode Mode = RoundMode::RNE, is_tile_data_v tile_shape_out,
          is_tile_data_v tile_shape_in>
void TDEQUANT(tile_shape_out &dst, tile_shape_in &src, float multiplier = 1.0f,
              int32_t zeroPoint = 0) {
  static_assert(is_valid_round_mode(Mode), "TDEQUANT invalid rounding mode");
  static_assert(type_traits<typename tile_shape_out::DType>::TypeCode ==
                        __type_fp32,
                "TDEQUANT destination must be FP32");
  static_assert(type_traits<typename tile_shape_in::DType>::TypeCode ==
                            __type_int8 ||
                    type_traits<typename tile_shape_in::DType>::TypeCode ==
                        __type_uint8,
                "TDEQUANT source must be S8 or U8");
  static_assert(tile_shape_out::Loc == Location::Vec &&
                    tile_shape_in::Loc == Location::Vec &&
                    tile_shape_out::isRowMajor &&
                    tile_shape_in::isRowMajor &&
                    !tile_shape_out::isBoxedLayout &&
                    !tile_shape_in::isBoxedLayout,
                "TDEQUANT operands must be Local RowMajor numeric Tiles");
  static_assert(
      tile_shape_out::Rows == tile_shape_in::Rows &&
          tile_shape_out::Cols == tile_shape_in::Cols &&
          (tile_shape_out::ValidRow == DYNAMIC ||
           tile_shape_in::ValidRow == DYNAMIC ||
           tile_shape_out::ValidRow == tile_shape_in::ValidRow) &&
          (tile_shape_out::ValidCol == DYNAMIC ||
           tile_shape_in::ValidCol == DYNAMIC ||
           tile_shape_out::ValidCol == tile_shape_in::ValidCol),
      "TDEQUANT source and destination logical shapes must match");
  uint32_t multiplierBits;
  __builtin_memcpy(&multiplierBits, &multiplier, sizeof(multiplier));
  volatile uint32_t mult = multiplierBits;
  volatile int32_t zp = zeroPoint;
  if constexpr (Mode == RoundMode::RNE) {
    asm volatile(
      "BSTART.TEPL 107, %D[SType]\n"
      "B.DATR FP32, RNONE\n"
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Col], ->lb2\n"
      "B.IOR [%[Mult], %[ZP]], []\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [SType] "i"(
            type_traits<typename tile_shape_in::DType>::TypeCode == __type_int8
                ? __type_int8 : __type_uint8),
        [VCOL] "r"(src.GetValidCol()), [VROW] "r"(src.GetValidRow()),
        [Col] "i"(tile_shape_in::Cols),
        [Mult] "r"(mult), [ZP] "r"(zp),
        [DstSize] "i"(tile_shape_out::TilesizeCode)
    );
  } else {
    asm volatile(
      "BSTART.TEPL 107, %D[SType]\n"
      PTO_RMODE_DATR_ASM("\n")
      "B.DIM %[VCOL], 0, ->lb0\n"
      "B.DIM %[VROW], 0, ->lb1\n"
      "B.DIM zero, %c[Col], ->lb2\n"
      "B.IOR [%[Mult], %[ZP]], []\n"
      "B.IOT %[Src], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
      : [Dst] "=&Tr"(dst.data())
      : [Src] "Tr"(src.data()),
        [SType] "i"(
            type_traits<typename tile_shape_in::DType>::TypeCode == __type_int8
                ? __type_int8 : __type_uint8),
        [__pto_DstType] "i"(__type_fp32),
        [RMode] "i"(static_cast<unsigned>(Mode)),
        [VCOL] "r"(src.GetValidCol()), [VROW] "r"(src.GetValidRow()),
        [Col] "i"(tile_shape_in::Cols),
        [Mult] "r"(mult), [ZP] "r"(zp),
        [DstSize] "i"(tile_shape_out::TilesizeCode)
    );
  }
}

// TDEQUANT: profile-defined dequantization
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TDEQUANT(tile_shape_out &dst, tile_shape_in &src) {
  TDEQUANT<RoundMode::RNE>(dst, src);
}

// TSORT: stable per-row group sort producing sorted values (FP16/FP32) and
// group-local original indices (U32). PTO ISA 0.58.3: TEPL Mode 3 Function 12
// (selector 0x06c). Each row sorts its columns in `sortWidth`-width groups
// from column 0; the last group may be short. ascending when descending is
// false, descending otherwise. sortWidth must be 1..64 (0/LB0 omitted -> 32).
//
// Encoding-carrier note: the LinxV5 backend has no canonical BSTART.SFU
// TSORT mnemonic yet, so the TEPL 108 (TSORT32) carrier is used here. The
// bundle shape below matches the normative contract: only LB0, a single
// B.IOR with the descending flag, a source+value-dest B.IOT, then a
// destination-only index B.IOT. The two destinations use their own
// TileSizeCode (value FP16/FP32 vs index U32 differ in bytes).
template <is_tile_data_v ValueDstTile, is_tile_data_v IndexDstTile,
          is_tile_data_v SourceTile>
void TSORT(ValueDstTile &valueDst, IndexDstTile &indexDst,
           SourceTile &source, uint32_t sortWidth = 32,
           bool descending = false) {
  static_assert(std::is_same_v<typename ValueDstTile::DType,
                               typename SourceTile::DType>,
                "TSORT value destination dtype must match source dtype");
  static_assert(type_traits<typename SourceTile::DType>::TypeCode ==
                        __type_fp16 ||
                    type_traits<typename SourceTile::DType>::TypeCode ==
                        __type_fp32,
                "TSORT value source/destination must be FP16 or FP32");
  static_assert(type_traits<typename IndexDstTile::DType>::TypeCode ==
                    __type_uint32,
                "TSORT index destination must be U32");
  static_assert(ValueDstTile::Rows == SourceTile::Rows &&
                    ValueDstTile::Cols == SourceTile::Cols &&
                    IndexDstTile::Rows == SourceTile::Rows &&
                    IndexDstTile::Cols == SourceTile::Cols &&
                    (ValueDstTile::ValidRow == DYNAMIC ||
                     SourceTile::ValidRow == DYNAMIC ||
                     ValueDstTile::ValidRow == SourceTile::ValidRow) &&
                    (ValueDstTile::ValidCol == DYNAMIC ||
                     SourceTile::ValidCol == DYNAMIC ||
                     ValueDstTile::ValidCol == SourceTile::ValidCol) &&
                    (IndexDstTile::ValidRow == DYNAMIC ||
                     SourceTile::ValidRow == DYNAMIC ||
                     IndexDstTile::ValidRow == SourceTile::ValidRow) &&
                    (IndexDstTile::ValidCol == DYNAMIC ||
                     SourceTile::ValidCol == DYNAMIC ||
                     IndexDstTile::ValidCol == SourceTile::ValidCol),
                "TSORT value/index destinations must match source logical "
                "and valid shapes");
  static_assert(ValueDstTile::Loc == Location::Vec &&
                    IndexDstTile::Loc == Location::Vec &&
                    SourceTile::Loc == Location::Vec &&
                    ValueDstTile::isRowMajor && IndexDstTile::isRowMajor &&
                    SourceTile::isRowMajor &&
                    !ValueDstTile::isBoxedLayout &&
                    !IndexDstTile::isBoxedLayout &&
                    !SourceTile::isBoxedLayout,
                "TSORT operands must be Local RowMajor VEC Tiles");

  // Anti-fold: keep the 0/1 descending flag off the zero register so the
  // B.IOR binder still carries a real GPR (B.IOR [zero],[] does not match).
  volatile uint32_t descendingValue = descending ? 1u : 0u;
  asm volatile(
    "BSTART.TEPL 108, %D[DataType]\n"
    "B.DIM %[SortWidth], 0, ->lb0\n"
    "B.IOR [%[Descending]], []\n"
    "B.IOT %[Source], mask=1111, ->%[ValueDst]<%Z[ValueTileSize]>\n"
    "B.IOT mask=1111, last, ->%[IndexDst]<%Z[IndexTileSize]>\n"
    : [ValueDst] "=&Tr"(valueDst.data()),
      [IndexDst] "=&Tr"(indexDst.data())
    : [Source] "Tr"(source.data()),
      [DataType] "i"(type_traits<typename SourceTile::DType>::TypeCode),
      [SortWidth] "r"(sortWidth),
      [Descending] "r"(descendingValue),
      // Use each destination's logical TilesizeCode: FP16 value (e.g. 2 KB
      // for 32x32) and U32 index (4 KB) differ, and B.IOT must carry the
      // per-PE logical size, not the uniform 4 KB storage carrier.
      [ValueTileSize] "i"(ValueDstTile::TilesizeCode),
      [IndexTileSize] "i"(IndexDstTile::TilesizeCode)
  );
}

// Deprecated single-output sort: does not match the PTO ISA 0.58.3 TSORT
// contract (no U32 index destination, legacy LB1/LB2 shape bundle). Kept as
// a migration diagnostic that fails at instantiation.
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TSORT32(tile_shape_out &dst, tile_shape_in &src) {
  static_assert(pto_dependent_false_v<tile_shape_out, tile_shape_in>,
                "TSORT32 is removed; use TSORT(valueDst, indexDst, source, "
                "sortWidth, descending), which emits the PTO ISA 0.58.3 "
                "value+index dual-output bundle");
}

// TMRGSORT: merge two sorted single-row sources into one destination
// (PTO ISA 0.58.3 TEPL Mode 3 Function 13 / selector 0x06D; canonical
// BSTART.SFU TMRGSORT). No B.DIM: the block carries only B.IOR RegSrc0
// (0/1 ascending/descending) and one TwoSrc_Dst B.IOT with <last>.
template <is_tile_data_v DstTile, is_tile_data_v LeftTile,
          is_tile_data_v RightTile>
void TMRGSORT(DstTile &dst, LeftTile &left, RightTile &right,
              bool descending = false) {
  static_assert(std::is_same_v<typename DstTile::DType,
                               typename LeftTile::DType> &&
                    std::is_same_v<typename DstTile::DType,
                                   typename RightTile::DType>,
                "TMRGSORT dst/left/right must share one dtype");
  static_assert(type_traits<typename DstTile::DType>::TypeCode ==
                            __type_fp16 ||
                    type_traits<typename DstTile::DType>::TypeCode ==
                        __type_fp32,
                "TMRGSORT dtype must be FP16 or FP32");
  static_assert(DstTile::Loc == Location::Vec &&
                    LeftTile::Loc == Location::Vec &&
                    RightTile::Loc == Location::Vec &&
                    DstTile::isRowMajor && LeftTile::isRowMajor &&
                    RightTile::isRowMajor && !DstTile::isBoxedLayout &&
                    !LeftTile::isBoxedLayout && !RightTile::isBoxedLayout,
                "TMRGSORT operands must be Local RowMajor numeric Tiles");
  static_assert(DstTile::Rows == 1 && LeftTile::Rows == 1 &&
                    RightTile::Rows == 1,
                "TMRGSORT operands must be single-row Tiles");
  static_assert((LeftTile::ValidCol == DYNAMIC || LeftTile::ValidCol > 0) &&
                    (RightTile::ValidCol == DYNAMIC ||
                     RightTile::ValidCol > 0),
                "TMRGSORT sources must be non-empty");
  static_assert(
      (DstTile::ValidCol == DYNAMIC || LeftTile::ValidCol == DYNAMIC ||
       RightTile::ValidCol == DYNAMIC ||
       (DstTile::ValidCol == LeftTile::ValidCol + RightTile::ValidCol &&
        DstTile::Cols >= LeftTile::ValidCol + RightTile::ValidCol &&
        (DstTile::Cols & (DstTile::Cols - 1)) == 0 &&
        DstTile::Cols / 2 < LeftTile::ValidCol + RightTile::ValidCol)),
      "TMRGSORT destination must contain the combined source columns");
  // Anti-fold: keep the 0/1 flag off the zero register (B.IOR [zero],[] does
  // not match).
  volatile uint32_t descendingValue = descending ? 1u : 0u;
  asm volatile(
    "BSTART.TEPL 109, %D[DataType]\n"
    "B.IOR [%[Descending]], []\n"
    "B.IOT %[Left], %[Right], mask=1111, last, ->%[Dst]<%Z[DstSize]>\n"
    : [Dst] "=&Tr"(dst.data())
    : [Left] "Tr"(left.data()), [Right] "Tr"(right.data()),
      [Descending] "r"(descendingValue),
      [DataType] "i"(type_traits<typename DstTile::DType>::TypeCode),
      [DstSize] "i"(DstTile::TilesizeCode)
  );
}

// TTRANS: tile transpose
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TTRANS(tile_shape_out &dst, tile_shape_in &src) {
  asm volatile(
    "BSTART.TEPL 110, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 111, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 112, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 113, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 114, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 115, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 116, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 64, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 65, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 66, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 67, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 68, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 76, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 77, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 80, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 81, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 82, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 83, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 84, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 92, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 93, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, mask=1111, last, ->%0<%Z6>\n"
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
    "BSTART.TEPL 69, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 70, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 71, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 72, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 73, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 74, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 75, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 85, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 86, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 87, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 88, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 89, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 90, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 91, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
    "BSTART.TEPL 96, %D1\n"
    "B.DIM %2, 0, ->lb0\n"
    "B.DIM %3, 0, ->lb1\n"
    "B.DIM zero, %c4, ->lb2\n"
    "B.IOT %5, %6, mask=1111, last, ->%0<%Z7>\n"
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
  static_assert(pto_dependent_false_v<tile_shape_out, gm_shape>,
                "TGATHERB is retired in PTO ISA 0.58.3; no active replacement. Remove the call or migrate to the active surface.");
}


#endif // TEMPLATE_ASM_HPP
