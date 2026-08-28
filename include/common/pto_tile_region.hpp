#ifndef PTO_TILE_REGION_HPP
#define PTO_TILE_REGION_HPP

#include "common/pto_tile.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

namespace pto {

namespace region {

template <typename...>
inline constexpr bool dependent_false_v = false;

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

  SubTileView(Parent &parent, int row, int col)
      : parent_(&parent), row_(row), col_(col) {}

  Parent &parent() const { return *parent_; }
  int row() const { return row_; }
  int col() const { return col_; }

private:
  Parent *parent_;
  int row_;
  int col_;
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
      return SubTileView<Parent, SubTile>(*parent_, row_, col);
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
  using SubTileType = SubTile;
  static constexpr int rows = Rows;
  static constexpr int cols = Cols;
  static constexpr std::size_t slot_count =
      static_cast<std::size_t>(Rows) * static_cast<std::size_t>(Cols);

  TileArray() = default;
  TileArray(const TileArray &) = delete;
  TileArray &operator=(const TileArray &) = delete;
  TileArray(TileArray &&) = default;
  TileArray &operator=(TileArray &&) = default;

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
    return TileArrayOutputRef<SubTile>(row, col);
  }
};

template <typename SubTile>
class TileArrayOutputRef {
public:
  using SubTileType = SubTile;

  template <typename, int, int>
  friend class TileArray;

  int row() const { return row_; }
  int col() const { return col_; }

private:
  TileArrayOutputRef(int row, int col) : row_(row), col_(col) {}
  int row_ = 0;
  int col_ = 0;
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

template <typename SubTile, int Rows, int Cols, typename Parent>
auto TPARTVIEW(Parent &parent)
    -> region::BorrowedTileArray<Parent, SubTile, Rows, Cols> {
  return region::BorrowedTileArray<Parent, SubTile, Rows, Cols>(parent);
}

template <typename Parent, typename SubTile, int Rows, int Cols>
auto TASSEMBLY(region::TileArray<SubTile, Rows, Cols> &&) -> Parent {
  static_assert(region::dependent_false_v<Parent>,
                "TASSEMBLY requires compiler region/session lowering");
}

} // namespace pto

#endif
