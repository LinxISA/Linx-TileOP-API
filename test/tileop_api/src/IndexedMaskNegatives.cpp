#include <common/pto_tileop.hpp>
using namespace pto;
#ifndef MASK_OPERATION
#define MASK_OPERATION 0
#endif
using D = Tile<Location::Vec, uint32_t, 2, 32>;
using I = Tile<Location::Vec, int32_t, 2, 32>;
#if defined(MASK_DTYPE)
using M = Tile<Location::Vec, uint32_t, 2, 128, BLayout::RowMajor, 2, 32>;
#elif defined(MASK_SIGNED)
using M = Tile<Location::Vec, int8_t, 2, 128, BLayout::RowMajor, 2, 32>;
#elif defined(MASK_LAYOUT)
using M = Tile<Location::Vec, uint8_t, 2, 128, BLayout::CubeM32, 2, 32>;
#elif defined(MASK_UNSUPPORTED_LAYOUT)
using M = Tile<Location::Vec, uint8_t, 2, 128, BLayout::ColMajor, 2, 32>;
#elif defined(MASK_ROWS)
using M = Tile<Location::Vec, uint8_t, 2, 128, BLayout::RowMajor, 1, 32>;
#elif defined(MASK_COLUMNS)
using M = Tile<Location::Vec, uint8_t, 2, 128, BLayout::RowMajor, 2, 16>;
#elif defined(MASK_CAPACITY)
using M = Tile<Location::Vec, uint8_t, 2, 262144, BLayout::RowMajor, 2, 32>;
#elif defined(MASK_ROW_CAPACITY)
using M = Tile<Location::Vec, uint8_t, 1, 128, BLayout::RowMajor, DYNAMIC, 32>;
#elif defined(MASK_COLUMN_CAPACITY)
using M = Tile<Location::Vec, uint8_t, 8, 16, BLayout::RowMajor, 2, DYNAMIC>;
#elif defined(MASK_SHARED)
using M = SharedTile<Tile<Location::Vec, uint8_t, 2, 128, BLayout::RowMajor, 2, 32>>;
#else
using M = Tile<Location::Vec, uint8_t, 2, 128, BLayout::RowMajor, 2, 32>;
#endif

void indexed_mask_contract(D &data, I &index, M &mask,
                           global_tensor<uint32_t, RowMajor<2, 32>> &gm) {
#if MASK_OPERATION == 0
  MGATHER_MASK(data, gm, index, mask);
#elif MASK_OPERATION == 1
  MGATHER_MASK(data, gm, index, mask, IndexedByteRange{-64, 60});
#elif MASK_OPERATION == 2
  MSCATTER_MASK(gm, data, index, mask);
#else
  MSCATTER_MASK(gm, data, index, mask, IndexedByteRange{-64, 60});
#endif
}
