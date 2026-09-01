#ifndef PTO_TILE_REGION_HPP
#define PTO_TILE_REGION_HPP

#include "common/pto_tile.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace pto {

namespace region {

template <typename...>
inline constexpr bool dependent_false_v = false;

constexpr int tile_size_code_for_bytes(std::size_t bytes) {
  std::size_t size = 128;
  int code = 1;
  while (size < bytes && code < 12) {
    size <<= 1;
    ++code;
  }
  return size == bytes ? code : -1;
}

template <typename Parent, typename SubTile, int Rows, int Cols>
struct partition_contract {
  static_assert(Rows > 0 && Cols > 0,
                "Tile partition extents must be positive");
  static_assert(std::is_same_v<typename Parent::DType, typename SubTile::DType>,
                "Tile partition requires matching element types");
  static_assert(Parent::Loc == SubTile::Loc,
                "Tile partition requires matching locations");
  static_assert(Parent::BFractal == SubTile::BFractal,
                "Tile partition requires matching layouts");
  static_assert(Parent::SFractal == SubTile::SFractal,
                "Tile partition requires matching storage layouts");
  static_assert(SubTile::LogicalTileBytes >= 128,
                "Tile partition fragments must contain at least one CELL");
  static_assert(Parent::Rows == Rows * SubTile::Rows &&
                    Parent::Cols == Cols * SubTile::Cols,
                "Tile partition must exactly cover the parent physical shape");
  static_assert(Parent::ValidRow == Rows * SubTile::ValidRow &&
                    Parent::ValidCol == Cols * SubTile::ValidCol,
                "Tile partition must exactly cover the parent valid shape");
  static_assert(Parent::LogicalTileBytes ==
                    Rows * Cols * SubTile::LogicalTileBytes,
                "Tile partition must cover the complete parent Tile");
};

template <typename Parent, typename SubTile>
class SubTileView {
public:
  using ParentTile = Parent;
  using SubTileType = SubTile;
  using DType = typename SubTile::DType;
  using TileDType = typename Parent::TileDType;
  static constexpr Location Loc = Parent::Loc;
  static constexpr int Rows = SubTile::Rows;
  static constexpr int Cols = SubTile::Cols;
  static constexpr int RowStride = SubTile::RowStride;
  static constexpr int ColStride = SubTile::ColStride;
  static constexpr int ValidRow = SubTile::ValidRow;
  static constexpr int ValidCol = SubTile::ValidCol;
  static constexpr BLayout BFractal = SubTile::BFractal;
  static constexpr SLayout SFractal = SubTile::SFractal;
  static constexpr int SFractalSize = SubTile::SFractalSize;
  static constexpr bool isRowMajor = SubTile::isRowMajor;
  static constexpr bool isBoxedLayout = SubTile::isBoxedLayout;
  static constexpr bool isInnerRowMajor = SubTile::isInnerRowMajor;
  static constexpr bool isInnerColMajor = SubTile::isInnerColMajor;
  static constexpr int InnerRows = SubTile::InnerRows;
  static constexpr int InnerCols = SubTile::InnerCols;
  static constexpr int Numel = SubTile::Numel;
  static constexpr int LogicalTileBytes = SubTile::LogicalTileBytes;
  static constexpr int TilesizeCode = SubTile::TilesizeCode;
  static constexpr bool IsValidActiveSize = SubTile::IsValidActiveSize;

  SubTileView(Parent &parent, int row, int col, int partition_cols)
      : parent_(&parent), row_(row), col_(col),
        partition_cols_(partition_cols) {}

  Parent &parent() const { return *parent_; }
  int row() const { return row_; }
  int col() const { return col_; }
  std::uintptr_t GetRangeBase() const {
    static_assert(SubTile::LogicalTileBytes % range::RangeAddressUnitBytes == 0,
                  "Tile partition offsets must be representable in 128B units");
    return static_cast<std::uintptr_t>(
        (static_cast<std::size_t>(row_) * partition_cols_ + col_) *
        (SubTile::LogicalTileBytes / range::RangeAddressUnitBytes));
  }
  int GetValidRow() const { return SubTile::ValidRow; }
  int GetValidCol() const { return SubTile::ValidCol; }
  decltype(auto) data() { return parent_->data(); }

private:
  Parent *parent_;
  int row_;
  int col_;
  int partition_cols_;
};

template <typename Parent, typename SubTile, int Rows, int Cols>
class BorrowedTileArray {
  using Contract = partition_contract<Parent, SubTile, Rows, Cols>;
  static_assert(sizeof(Contract) > 0);

public:
  using ParentTile = Parent;
  using SubTileType = SubTile;
  static constexpr int rank = 2;
  static constexpr int rows = Rows;
  static constexpr int cols = Cols;

  explicit BorrowedTileArray(Parent &parent) : parent_(&parent) {}

  Parent &parent() const { return *parent_; }

  class Row {
  public:
    Row(Parent &parent, int row) : parent_(&parent), row_(row) {}

    SubTileView<Parent, SubTile> operator[](int col) const {
      return SubTileView<Parent, SubTile>(*parent_, row_, col, Cols);
    }

  private:
    Parent *parent_;
    int row_;
  };

  Row operator[](int row) const { return Row(*parent_, row); }

  Row row(int row) const {
    return Row(*parent_, row);
  }

private:
  Parent *parent_;
};

template <typename SubTile>
class TileArrayOutputRef;

template <typename SubTile, int Rows, int Cols>
class TileArray {
public:
  static_assert(Rows > 0 && Cols > 0,
                "TileArray extents must be positive");
  using SubTileType = SubTile;
  static constexpr int rows = Rows;
  static constexpr int cols = Cols;
  static constexpr std::size_t slot_count =
      static_cast<std::size_t>(Rows) * static_cast<std::size_t>(Cols);
  static constexpr std::size_t ParentBytes =
      SubTile::LogicalTileBytes * slot_count;
  static constexpr int ParentSizeCode =
      tile_size_code_for_bytes(ParentBytes);
  static_assert((slot_count - 1) *
                    (SubTile::LogicalTileBytes / range::RangeAddressUnitBytes) <=
                2047,
                "Tile assembly slot range exceeds the ISA uimm11 limit");
  static_assert(ParentSizeCode > 0,
                "Tile assembly parent capacity needs a PTO SizeCode");

#ifdef __linx
  using ParentCarrier = linx_tile_carrier<ParentBytes>;
  using ParentRegisterType = typename ParentCarrier::RegisterType;
#else
  using ParentCarrier =
      typename SubTile::DType[ParentBytes * 8 /
                              type_traits<typename SubTile::DType>::bits];
#endif

  TileArray() = default;
  TileArray(const TileArray &) = delete;
  TileArray &operator=(const TileArray &) = delete;
  TileArray(TileArray &&) = default;
  TileArray &operator=(TileArray &&) = default;

#ifdef __linx
  ParentRegisterType &data() { return data_.Register; }
  ParentCarrier &carrier() { return data_; }
#else
  ParentCarrier &data() { return data_; }
  ParentCarrier &carrier() { return data_; }
#endif

  class Row {
  public:
    Row(TileArray &array, int row) : array_(&array), row_(row) {}

    TileArrayOutputRef<SubTile> operator[](int col) {
      return array_->makeOutputRef(row_, col);
    }

  private:
    TileArray *array_;
    int row_;
  };

  Row operator[](int row) { return Row(*this, row); }
  Row row(int row) { return Row(*this, row); }

private:
  TileArrayOutputRef<SubTile> makeOutputRef(int row, int col) {
    return TileArrayOutputRef<SubTile>(data_, row, col, Cols,
                                       Rows * Cols, ParentSizeCode);
  }
  ParentCarrier data_;
};

template <typename SubTile>
class TileArrayOutputRef {
public:
  using SubTileType = SubTile;

  template <typename, int, int>
  friend class TileArray;

  int row() const { return row_; }
  int col() const { return col_; }
  int ordinal() const { return row_ * array_cols_ + col_; }
  int slot_count() const { return slot_count_; }
  std::uintptr_t range_base_units() const {
    static_assert(SubTile::LogicalTileBytes % range::RangeAddressUnitBytes == 0,
                  "Tile assembly offsets must be representable in 128B units");
    return static_cast<std::uintptr_t>(
        ordinal() * (SubTile::LogicalTileBytes /
                     range::RangeAddressUnitBytes));
  }
  static constexpr int Rows = SubTile::Rows;
  static constexpr int Cols = SubTile::Cols;
  static constexpr int ValidRow = SubTile::ValidRow;
  static constexpr int ValidCol = SubTile::ValidCol;
  static constexpr int RowStride = SubTile::RowStride;
  static constexpr int ColStride = SubTile::ColStride;
  static constexpr Location Loc = SubTile::Loc;
  static constexpr BLayout BFractal = SubTile::BFractal;
  static constexpr SLayout SFractal = SubTile::SFractal;
  static constexpr bool isRowMajor = SubTile::isRowMajor;
  static constexpr bool isBoxedLayout = SubTile::isBoxedLayout;
  static constexpr bool isInnerRowMajor = SubTile::isInnerRowMajor;
  static constexpr bool isInnerColMajor = SubTile::isInnerColMajor;
  static constexpr int InnerRows = SubTile::InnerRows;
  static constexpr int InnerCols = SubTile::InnerCols;
  static constexpr int Numel = SubTile::Numel;
  static constexpr int LogicalTileBytes = SubTile::LogicalTileBytes;
  static constexpr int TilesizeCode = SubTile::TilesizeCode;
  static constexpr bool IsValidActiveSize = SubTile::IsValidActiveSize;
  using DType = typename SubTile::DType;
  using TileDType = typename SubTile::TileDType;

  template <int ParentSizeCode>
  auto &parent_data() {
    static_assert(ParentSizeCode >= 1 && ParentSizeCode <= 12);
    constexpr std::size_t ParentBytes =
        static_cast<std::size_t>(128) << (ParentSizeCode - 1);
#ifdef __linx
    using Carrier = linx_tile_carrier<ParentBytes>;
    return reinterpret_cast<Carrier *>(array_)->Register;
#else
    using Carrier = typename SubTile::DType[
        ParentBytes * 8 / type_traits<typename SubTile::DType>::bits];
    return *reinterpret_cast<Carrier *>(array_);
#endif
  }
  int GetValidRow() const { return ValidRow; }
  int GetValidCol() const { return ValidCol; }
  int parent_size_code() const { return parent_size_code_; }

private:
  template <typename, int, int>
  friend class TileArray;
  template <typename Carrier>
  TileArrayOutputRef(Carrier &array, int row, int col, int array_cols,
                     int slot_count, int parent_size_code)
      : array_(&array), row_(row), col_(col), array_cols_(array_cols),
        slot_count_(slot_count), parent_size_code_(parent_size_code) {}
  void *array_ = nullptr;
  int row_ = 0;
  int col_ = 0;
  int array_cols_ = 0;
  int slot_count_ = 0;
  int parent_size_code_ = 0;
};

} // namespace region

template <typename Parent, typename SubTile>
using SubTileView = region::SubTileView<Parent, SubTile>;

template <typename Parent, typename SubTile, int Rows, int Cols>
using BorrowedTileArray =
    region::BorrowedTileArray<Parent, SubTile, Rows, Cols>;

template <typename SubTile, int Rows, int Cols>
using TileArray = region::TileArray<SubTile, Rows, Cols>;

template <typename SubTile>
using TileArrayOutputRef = region::TileArrayOutputRef<SubTile>;

template <typename T>
struct is_subtile_view : std::false_type {};
template <typename Parent, typename SubTile>
struct is_subtile_view<region::SubTileView<Parent, SubTile>> : std::true_type {};
template <typename T>
inline constexpr bool is_subtile_view_v = is_subtile_view<T>::value;

template <typename T>
struct is_tile_array_output_ref : std::false_type {};
template <typename SubTile>
struct is_tile_array_output_ref<region::TileArrayOutputRef<SubTile>>
    : std::true_type {};
template <typename T>
inline constexpr bool is_tile_array_output_ref_v =
    is_tile_array_output_ref<T>::value;

template <typename SubTile, int Rows, int Cols, typename Parent>
auto TPARTVIEW(Parent &parent)
    -> region::BorrowedTileArray<Parent, SubTile, Rows, Cols> {
  return region::BorrowedTileArray<Parent, SubTile, Rows, Cols>(parent);
}

template <typename Parent, typename SubTile, int Rows, int Cols>
auto TASSEMBLY(region::TileArray<SubTile, Rows, Cols> &&array) -> Parent {
  static_assert(std::is_same_v<typename Parent::DType,
                               typename SubTile::DType>,
                "TASSEMBLY parent and fragment dtypes must match");
  static_assert(Parent::Loc == SubTile::Loc,
                "TASSEMBLY parent and fragment locations must match");
  static_assert(Parent::BFractal == SubTile::BFractal,
                "TASSEMBLY parent and fragment layouts must match");
  static_assert(Parent::SFractal == SubTile::SFractal,
                "TASSEMBLY parent and fragment storage layouts must match");
  static_assert(Parent::Rows == Rows * SubTile::Rows &&
                    Parent::Cols == Cols * SubTile::Cols,
                "TASSEMBLY parent and fragment shapes do not match");
  static_assert(Parent::ValidRow == Rows * SubTile::ValidRow &&
                    Parent::ValidCol == Cols * SubTile::ValidCol,
                "TASSEMBLY parent and fragment valid shapes do not match");
  static_assert(Parent::LogicalTileBytes ==
                    SubTile::LogicalTileBytes * Rows * Cols,
                "TASSEMBLY parent and fragment coverage do not match");
  Parent result;
#ifdef __linx
  static_assert(std::is_same_v<typename Parent::TileDType,
                               typename region::TileArray<SubTile, Rows,
                                                          Cols>::ParentCarrier>,
                "TASSEMBLY parent and TileArray carriers must match");
  result.assignData(array.carrier());
#endif
  return result;
}

} // namespace pto

#endif
