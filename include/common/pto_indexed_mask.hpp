#ifndef PTO_INDEXED_MASK_HPP
#define PTO_INDEXED_MASK_HPP

#include "common/pto_tile.hpp"

namespace pto::detail {

// Indexed TLSU consumes Numeric U8 lanes, not packed comparison predicates.
// Types cannot identify the producer or prove that payload bytes are 0/1.
// Dynamic valid extents must agree at execution; this adds no payload scan.
template <typename Data, typename Index, typename Mask>
constexpr void validate_indexed_mask() {
  static_assert(std::is_same_v<typename Mask::DType, uint8_t>,
                "indexed TLSU mask requires canonical U8 byte lanes; packed "
                "RowMajor TCMP/TCMPS results cannot be used directly");
  static_assert(!is_shared_tile_v<Mask>,
                "indexed TLSU mask must use a Local Tile");
  static_assert(Mask::BFractal == BLayout::RowMajor ||
                    Mask::BFractal == BLayout::CubeM16 ||
                    Mask::BFractal == BLayout::CubeM32,
                "indexed TLSU mask requires RowMajor, CUBE_M16 or CUBE_M32");
  static_assert(Mask::BFractal == Index::BFractal &&
                    Mask::BFractal == Data::BFractal,
                "indexed TLSU mask, index and data layouts must match");
  static_assert(Mask::IsValidActiveSize,
                "indexed TLSU mask capacity must be 128 B..256 KiB "
                "with an encodable SizeCode");
  static_assert(Mask::ValidRow == DYNAMIC || Index::ValidRow == DYNAMIC ||
                    Mask::ValidRow == Index::ValidRow,
                "indexed TLSU mask and index valid rows must match");
  static_assert(Mask::ValidCol == DYNAMIC || Index::ValidCol == DYNAMIC ||
                    Mask::ValidCol == Index::ValidCol,
                "indexed TLSU mask and index valid columns must match");
  // Each operand retains independent physical geometry and capacity. Only
  // reject an extent that cannot fit, including a known peer of a dynamic one.
  static_assert(Index::ValidRow == DYNAMIC || Mask::Rows >= Index::ValidRow,
                "indexed TLSU mask row capacity must cover the index extent");
  static_assert(Index::ValidCol == DYNAMIC || Mask::Cols >= Index::ValidCol,
                "indexed TLSU mask column capacity must cover the index extent");
  static_assert(Mask::ValidRow == DYNAMIC || Index::Rows >= Mask::ValidRow,
                "indexed TLSU index row capacity must cover the mask extent");
  static_assert(Mask::ValidCol == DYNAMIC || Index::Cols >= Mask::ValidCol,
                "indexed TLSU index column capacity must cover the mask extent");
}

} // namespace pto::detail

#endif
