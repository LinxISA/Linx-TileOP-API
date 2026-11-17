// #320: mask physical capacity is independent of the indexed valid extent.
#include <common/pto_tileop.hpp>
using namespace pto;
using D = Tile<Location::Vec, uint32_t, 1, 32>;
using I = Tile<Location::Vec, int32_t, 1, 32>;
using M = Tile<Location::Vec, uint8_t, 1, 128, BLayout::RowMajor, 1, 32>;
using G = global_tensor<uint32_t, RowMajor<1, 32>>;

extern "C" void indexed_mask_all(D &data, I &index, M &mask, G &gm) {
  MGATHER_MASK(data, gm, index, mask);
  MGATHER_MASK(data, gm, index, mask, IndexedByteRange{-64, 60});
  MSCATTER_MASK(gm, data, index, mask);
  MSCATTER_MASK(gm, data, index, mask, IndexedByteRange{-64, 60});
}

using DD = Tile<Location::Vec, uint32_t, 1, 64, BLayout::RowMajor, 1, DYNAMIC>;
using DI = Tile<Location::Vec, int32_t, 1, 64, BLayout::RowMajor, 1, DYNAMIC>;
using DM = Tile<Location::Vec, uint8_t, 1, 128, BLayout::RowMajor, 1, DYNAMIC>;
extern "C" void indexed_mask_dynamic(DD &data, DI &index, DM &mask, G &gm) {
  // Caller guarantees all runtime valid columns agree and fit each carrier.
  MGATHER_MASK(data, gm, index, mask);
  MGATHER_MASK(data, gm, index, mask, IndexedByteRange{-64, 60});
  MSCATTER_MASK(gm, data, index, mask);
  MSCATTER_MASK(gm, data, index, mask, IndexedByteRange{-64, 60});
}
