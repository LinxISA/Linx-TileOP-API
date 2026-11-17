#ifndef PTO_SELECT_HPP
#define PTO_SELECT_HPP

#include <cassert>
#include "common/pto_tile.hpp"

namespace pto::predicate {
// Complete low/high predicate planes for an 8-bit CUBE operation.
struct GPRMask { uint64_t low; uint64_t high; };
}

namespace pto::select_detail {

template <typename T> struct backing_type { using type = typename T::DType; };
template <typename NewType, typename Source>
struct backing_type<ReinterpretedTileView<NewType, Source>> : backing_type<Source> {};

constexpr bool data_type_supported(int code) {
  switch (code) {
  case __type_fp64: case __type_fp32: case __type_tf32: case __type_hf32:
  case __type_fp16: case __type_bf16: case __type_fp8_e4m3: case __type_fp8_e5m2:
  case __type_int64: case __type_int32: case __type_int16: case __type_int8:
  case __type_uint64: case __type_uint32: case __type_uint16: case __type_uint8:
    return true;
  default: return false;
  }
}

constexpr bool source_type_supported(int code, bool cube) {
  if (data_type_supported(code)) return true;
  // RowMajor accepts the wider raw TEPL backing domain.
  return !cube && (code == __type_hif8 || code == __type_fp6_e3m2 ||
                   code == __type_fp5_e2m3 || code == __type_fp8_e8m0);
}

template <typename Source, typename D>
constexpr bool source_compatible() {
  using Stored = typename backing_type<Source>::type;
  constexpr int stored = type_traits<Stored>::TypeCode;
  return source_type_supported(stored, D::IsCubeLayout) &&
         tile_element_bits_v<Stored> == tile_element_bits_v<typename D::DType>;
}

constexpr bool dimension_matches(int a, int b) {
  return a == DYNAMIC || b == DYNAMIC || a == b;
}

template <typename D, typename T, typename F>
__attribute__((always_inline)) inline void
validate_numeric(const T &t, const F &f) {
  static_assert(data_type_supported(type_traits<typename D::DType>::TypeCode),
                "TSEL unsupported operation data type");
  static_assert(source_compatible<T, D>() && source_compatible<F, D>(),
                "TSEL numeric sources require supported equal-width non-four-bit carriers");
  static_assert(D::IsValidActiveSize && T::IsValidActiveSize && F::IsValidActiveSize,
                "TSEL requires assigned Local Tile capacities");
  static_assert(D::Loc != Location::Shared && T::Loc != Location::Shared &&
                    F::Loc != Location::Shared && !D::isBoxedLayout &&
                    !T::isBoxedLayout && !F::isBoxedLayout,
                "TSEL requires unboxed Local operands");
  static_assert((D::BFractal == BLayout::RowMajor ||
                 D::BFractal == BLayout::CubeM16 || D::BFractal == BLayout::CubeM32) &&
                    T::BFractal == D::BFractal && F::BFractal == D::BFractal &&
                    T::PhysicalRows == D::PhysicalRows && F::PhysicalRows == D::PhysicalRows &&
                    T::PhysicalCols == D::PhysicalCols && F::PhysicalCols == D::PhysicalCols,
                "TSEL physical shapes and layouts must match the destination");
  static_assert(dimension_matches(D::ValidRow, T::ValidRow) &&
                    dimension_matches(D::ValidCol, T::ValidCol) &&
                    dimension_matches(D::ValidRow, F::ValidRow) &&
                    dimension_matches(D::ValidCol, F::ValidCol) &&
                    dimension_matches(T::ValidRow, F::ValidRow) &&
                    dimension_matches(T::ValidCol, F::ValidCol),
                "TSEL numeric valid shapes must match the destination");
  assert(t.GetValidRow() > 0 && t.GetValidRow() <= T::Rows &&
         t.GetValidCol() > 0 && t.GetValidCol() <= T::Cols);
  assert(t.GetValidRow() == f.GetValidRow() && t.GetValidCol() == f.GetValidCol());
  assert(t.GetValidRow() <= F::Rows && t.GetValidCol() <= F::Cols &&
         t.GetValidRow() <= D::Rows && t.GetValidCol() <= D::Cols);
  assert((D::ValidRow == DYNAMIC || D::ValidRow == t.GetValidRow()) &&
         (D::ValidCol == DYNAMIC || D::ValidCol == t.GetValidCol()));
}

template <typename D, typename P, typename T>
__attribute__((always_inline)) inline void validate_mask(const P &p, const T &t) {
  static_assert(P::Loc != Location::Shared && !P::isBoxedLayout &&
                    P::BFractal == D::BFractal,
                "TSEL predicate must have the numeric Local layout");
  // PredicateCell has its own U8 physical geometry, independent of basis.
  static_assert(D::IsCubeLayout ||
                    (P::PhysicalRows == D::PhysicalRows && P::PhysicalCols == D::PhysicalCols),
                "TSEL RowMajor predicate physical shapes must match the destination");
  static_assert(dimension_matches(P::ValidRow, T::ValidRow) &&
                    dimension_matches(P::ValidCol, T::ValidCol),
                "TSEL predicate valid shapes must match the numeric sources");
  assert(p.GetValidRow() == t.GetValidRow() && p.GetValidCol() == t.GetValidCol());
  assert(p.GetValidRow() <= P::Rows && p.GetValidCol() <= P::Cols);
}

template <typename D, typename T>
__attribute__((always_inline)) inline void publish_shape(D &d, const T &t) {
  if constexpr (D::ValidRow == DYNAMIC) d.RowMaskInternal = t.GetValidRow();
  if constexpr (D::ValidCol == DYNAMIC) d.ColMaskInternal = t.GetValidCol();
}

#define PTO_SELECT_LAYOUT \
  ".if %c[Layout] == 31\nB.DATR CUBE_M16, Null\n" \
  ".elseif %c[Layout] == 29\nB.DATR CUBE_M32, Null\n.endif\n"

// Keep constant dimensions immediate and runtime dimensions in GPRs.
#define PTO_SELECT_EMIT(BINDINGS, ...) \
  do { \
    if constexpr (T::ValidCol > 0 && T::ValidRow > 0) { \
      asm volatile("BSTART.TEPL TSEL, %D[Type]\n" PTO_SELECT_LAYOUT \
          "B.DIM zero, %c[Columns], ->LB0\nB.DIM zero, %c[Rows], ->LB1\n" \
          "B.DIM zero, %c[PhysicalColumns], ->LB2\n" BINDINGS \
          : [Result] "=Tr"(dst.data()) \
          : [Columns] "i"(T::ValidCol), [Rows] "i"(T::ValidRow), __VA_ARGS__); \
    } else if constexpr (T::ValidCol > 0) { \
      asm volatile("BSTART.TEPL TSEL, %D[Type]\n" PTO_SELECT_LAYOUT \
          "B.DIM zero, %c[Columns], ->LB0\nB.DIM %[Rows], 0, ->LB1\n" \
          "B.DIM zero, %c[PhysicalColumns], ->LB2\n" BINDINGS \
          : [Result] "=Tr"(dst.data()) \
          : [Columns] "i"(T::ValidCol), [Rows] "r"(when_true.GetValidRow()), __VA_ARGS__); \
    } else if constexpr (T::ValidRow > 0) { \
      asm volatile("BSTART.TEPL TSEL, %D[Type]\n" PTO_SELECT_LAYOUT \
          "B.DIM %[Columns], 0, ->LB0\nB.DIM zero, %c[Rows], ->LB1\n" \
          "B.DIM zero, %c[PhysicalColumns], ->LB2\n" BINDINGS \
          : [Result] "=Tr"(dst.data()) \
          : [Columns] "r"(when_true.GetValidCol()), [Rows] "i"(T::ValidRow), __VA_ARGS__); \
    } else { \
      asm volatile("BSTART.TEPL TSEL, %D[Type]\n" PTO_SELECT_LAYOUT \
          "B.DIM %[Columns], 0, ->LB0\nB.DIM %[Rows], 0, ->LB1\n" \
          "B.DIM zero, %c[PhysicalColumns], ->LB2\n" BINDINGS \
          : [Result] "=Tr"(dst.data()) \
          : [Columns] "r"(when_true.GetValidCol()), [Rows] "r"(when_true.GetValidRow()), __VA_ARGS__); \
    } \
  } while (false)

#define PTO_SELECT_INPUTS \
    [True] "Tr"(when_true.data()), [False] "Tr"(when_false.data()), \
    [Type] "i"(type_traits<typename D::DType>::TypeCode), \
    [Layout] "i"(D::BFractal == BLayout::CubeM16 ? 31 : D::BFractal == BLayout::CubeM32 ? 29 : 0), \
    [PhysicalColumns] "i"(T::Cols), [Size] "i"(D::TilesizeCode)

template <typename D, typename Carrier, typename T, typename F>
__attribute__((always_inline)) inline void
emit_tile(D &dst, const Carrier &mask, const T &when_true, const F &when_false) {
  validate_numeric<D>(when_true, when_false);
  PTO_SELECT_EMIT(
      "B.IOT %[Predicate], %[True], mask=1111\n"
      "B.IOT %[False], mask=1111, last, ->%[Result]<%Z[Size]>\n",
      [Predicate] "Tr"(mask), PTO_SELECT_INPUTS);
  publish_shape(dst, when_true);
}

template <bool TwoWords, typename D, typename T, typename F>
__attribute__((always_inline)) inline void
emit_gpr(D &dst, uint64_t low, uint64_t high, const T &when_true, const F &when_false) {
  validate_numeric<D>(when_true, when_false);
  constexpr int bits = type_traits<typename D::DType>::bits;
  constexpr int rows = D::BFractal == BLayout::CubeM16 ? 16 : 32;
  constexpr int fields = rows == 32 ? 2 : bits == 32 ? 2 : 4;
  constexpr int columns = fields * (TwoWords ? 2 : 1);
  static_assert(D::BFractal == BLayout::CubeM16 || D::BFractal == BLayout::CubeM32,
                "TSEL GPR mask requires CUBE_M16/M32");
  static_assert(TwoWords == (bits == 8),
                "TSEL requires two GPR mask words for 8-bit operations and one otherwise");
  static_assert((T::ValidRow == DYNAMIC || T::ValidRow <= rows) &&
                    (T::ValidCol == DYNAMIC || T::ValidCol <= columns),
                "TSEL GPR mask valid shape exceeds mask coverage");
  assert(when_true.GetValidRow() <= rows && when_true.GetValidCol() <= columns);
  if constexpr (TwoWords) {
    PTO_SELECT_EMIT(
        "B.IOT %[True], %[False], mask=1111, last, ->%[Result]<%Z[Size]>\n"
        "B.IOR [%[Low],%[High]], []\n",
        [Low] "r"(low), [High] "r"(high), PTO_SELECT_INPUTS);
  } else {
    PTO_SELECT_EMIT(
        "B.IOT %[True], %[False], mask=1111, last, ->%[Result]<%Z[Size]>\n"
        "B.IOR [%[Low]], []\n",
        [Low] "r"(low), PTO_SELECT_INPUTS);
  }
  publish_shape(dst, when_true);
}

#undef PTO_SELECT_INPUTS
#undef PTO_SELECT_EMIT
#undef PTO_SELECT_LAYOUT
}
#endif
