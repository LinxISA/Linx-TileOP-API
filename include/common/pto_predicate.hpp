#ifndef PTO_PREDICATE_HPP
#define PTO_PREDICATE_HPP

#include "common/pto_tile.hpp"

namespace pto {
namespace predicate {

template <typename Basis> class Predicate;
template <CmpMode Mode, typename Basis>
__attribute__((always_inline)) inline Predicate<Basis>
compare_scalar(Basis &source, typename Basis::DType scalar);

template <typename Basis> class Predicate {
  static_assert(Basis::Loc == Location::Vec &&
                    Basis::BFractal == BLayout::CubeM32,
                "Predicate currently requires a Local VEC CUBE_M32 basis");
  static_assert(Basis::ValidRow > 0 && Basis::ValidCol > 0,
                "Predicate currently requires static valid dimensions");
  static constexpr int StorageRows = 32;
  static constexpr int MinimumColumns = 128 / StorageRows;
  static constexpr int StorageColumns = Basis::Cols < MinimumColumns
                                           ? MinimumColumns : Basis::Cols;
  using Storage = Tile<Location::Vec, uint8_t, StorageRows, StorageColumns,
                       Basis::BFractal, Basis::ValidRow, Basis::ValidCol>;
  typename Storage::TileRegisterType Carrier;

  Predicate() = default;
  template <CmpMode Mode, typename Source>
  friend __attribute__((always_inline)) inline Predicate<Source>
  compare_scalar(Source &, typename Source::DType);
  template <typename Source, typename Index, typename Value, typename Global>
  friend __attribute__((always_inline)) inline void
  scatter(Global &, const Index &, const Value &, const Predicate<Source> &);
  template <bool Invert, typename Source, typename Index, typename Value,
            typename Global>
  friend __attribute__((always_inline)) inline void
  atomic_add(Global &, const Index &, const Value &, const Predicate<Source> &);

public:
  using BasisTile = Basis;
};

template <CmpMode Mode, typename Basis>
__attribute__((always_inline)) inline Predicate<Basis>
compare_scalar(Basis &source, typename Basis::DType scalar) {
  static_assert(Mode == CmpMode::EQ || Mode == CmpMode::NE ||
                    Mode == CmpMode::LT || Mode == CmpMode::GT ||
                    Mode == CmpMode::LE || Mode == CmpMode::GE,
                "Predicate comparison mode is reserved");
  static_assert(std::is_same_v<typename Basis::DType, float> ||
                    std::is_same_v<typename Basis::DType, uint32_t> ||
                    std::is_same_v<typename Basis::DType, int32_t>,
                "Predicate comparison currently supports FP32/S32/U32");
  Predicate<Basis> result;
  asm volatile(
      "BSTART.TEPL 45, %D[Type]\n"
      ".if %c[Mode] == 0\nB.DATR CUBE_M32, DTYPE_NONE, Zero, EQ, RNONE\n"
      ".elseif %c[Mode] == 1\nB.DATR CUBE_M32, DTYPE_NONE, Zero, NE, RNONE\n"
      ".elseif %c[Mode] == 2\nB.DATR CUBE_M32, DTYPE_NONE, Zero, LT, RNONE\n"
      ".elseif %c[Mode] == 3\nB.DATR CUBE_M32, DTYPE_NONE, Zero, GT, RNONE\n"
      ".elseif %c[Mode] == 4\nB.DATR CUBE_M32, DTYPE_NONE, Zero, LE, RNONE\n"
      ".else\nB.DATR CUBE_M32, DTYPE_NONE, Zero, GE, RNONE\n.endif\n"
      "B.DIM zero, %c[Columns], ->LB0\n"
      "B.DIM zero, %c[Rows], ->LB1\n"
      "B.DIM zero, %c[PhysicalColumns], ->LB2\n"
      "B.IOT %[Source], mask=1111, last, ->%[Result]<%Z[Size]>\n"
      "B.IOR [%[Scalar]], []\n"
      : [Result] "=Tr"(result.Carrier)
      : [Source] "Tr"(source.data()), [Scalar] "r"(scalar),
        [Type] "i"(type_traits<typename Basis::DType>::TypeCode),
        [Mode] "i"(static_cast<int>(Mode)),
        [Rows] "i"(Basis::ValidRow), [Columns] "i"(Basis::ValidCol),
        [PhysicalColumns] "i"(Basis::Cols),
        [Size] "i"(tile_type_traits<typename Predicate<Basis>::Storage::TileDType>::TilesizeCode));
  return result;
}

template <typename Basis, typename Index, typename Value>
constexpr void validate_consumer() {
  static_assert(Index::Loc == Location::Vec && Value::Loc == Location::Vec,
                "Predicate consumers require Local VEC operands");
  static_assert(Index::BFractal == Basis::BFractal &&
                    Value::BFractal == Basis::BFractal,
                "Predicate consumer layout must match producer layout");
  static_assert(Index::ValidRow == Basis::ValidRow &&
                    Index::ValidCol == Basis::ValidCol &&
                    Value::ValidRow == Basis::ValidRow &&
                    Value::ValidCol == Basis::ValidCol,
                "Predicate consumer valid shape must match producer shape");
  static_assert(std::is_same_v<typename Index::DType, uint64_t> ||
                    std::is_same_v<typename Index::DType, int64_t>,
                "Predicate indices must be S64/U64 byte displacements");
  static_assert(std::is_same_v<typename Value::DType, uint32_t> ||
                    std::is_same_v<typename Value::DType, int32_t> ||
                    std::is_same_v<typename Value::DType, float>,
                "Predicate consumers currently support FP32/S32/U32 values");
  static_assert(std::is_same_v<typename Value::DType, typename Basis::DType>,
                "Predicate consumer data type must match its comparison basis");
  static_assert(Index::Cols >= Basis::ValidCol &&
                    Value::Cols >= Basis::ValidCol,
                "Predicate operands must cover the complete valid region");
}

template <typename Basis, typename Index, typename Value, typename Global>
__attribute__((always_inline)) inline void
scatter(Global &destination, const Index &offsets, const Value &values,
        const Predicate<Basis> &active) {
  validate_consumer<Basis, Index, Value>();
  static_assert(std::is_same_v<typename Global::DType, typename Value::DType>,
                "Predicate destination element type must match values");
  asm volatile(
      "BSTART.TLSU MSCATTER.MASK, %D[Type]\n"
      "B.DATR CUBE_M32, Zero\n"
      "B.DIM zero, %c[Columns], ->LB0\n"
      "B.DIM zero, %c[Rows], ->LB1\n"
      "B.DIM zero, %c[PhysicalColumns], ->LB2\n"
      "B.IOT %[Values], %[Offsets], mask=1111\n"
      "B.IOT %[Predicate], mask=1111, last\n"
      "B.IOR [%[Base]], []\n"
      :
      : [Values] "Tr"(values.data()), [Offsets] "Tr"(offsets.data()),
        [Predicate] "Tr"(active.Carrier), [Base] "r"(destination.data()),
        [Type] "i"(type_traits<typename Value::DType>::TypeCode),
        [Columns] "i"(Basis::ValidCol), [Rows] "i"(Basis::ValidRow),
        [PhysicalColumns] "i"(Value::Cols)
      : "memory");
}

template <bool Invert = false, typename Basis, typename Index, typename Value,
          typename Global>
__attribute__((always_inline)) inline void
atomic_add(Global &destination, const Index &offsets, const Value &values,
           const Predicate<Basis> &active) {
  validate_consumer<Basis, Index, Value>();
  static_assert(std::is_same_v<typename Global::DType, typename Value::DType>,
                "Predicate destination element type must match values");
  ew_mscatter_add_predicate(
      Basis::ValidRow, Basis::ValidCol, Value::Cols,
      type_traits<typename Value::DType>::TypeCode,
      29, destination.data(),
      values.data(), offsets.data(), active.Carrier, Invert);
}

}
}

#endif
