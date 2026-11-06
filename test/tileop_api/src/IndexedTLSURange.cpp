// Explicit byte bounds across all 23 indexed TLSU operations.
#include <common/pto_tileop.hpp>
using namespace pto;
using D = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using I = Tile<Location::Vec, int32_t, 8, 256, BLayout::RowMajor>;
using P = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
extern "C" void indexed_range_all(D &old, I &ix, D &v, D &expected, P &mask, uint32_t *base, int64_t min, int64_t max) {
  IndexedByteRange range{min, max};
  global_tensor<uint32_t, RowMajor<8, 256>> gm(base);
  MGATHER(old, gm, ix, range);
  MSCATTER(gm, v, ix, range);
  MGATHER_MASK(old, gm, ix, mask, range);
  MSCATTER_MASK(gm, v, ix, mask, range);
  MGATHER_CAS(old, reinterpret_cast<uintptr_t>(base), ix, expected, v, 256, range, 2);
  MGATHER_EXCH(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_MAX(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_MIN(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_ADD(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_INC(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_DEC(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_AND(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_OR(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MGATHER_XOR(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_MAX(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_MIN(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_ADD(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_INC(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_DEC(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_AND(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_OR(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_XOR(reinterpret_cast<uintptr_t>(base), ix, v, 256, range, 2);
  MSCATTER_POPC(reinterpret_cast<uintptr_t>(base), ix, 256, range, 2);
}
extern "C" void indexed_zero(D &old, I &ix, uint32_t *base) {
  global_tensor<uint32_t, RowMajor<8, 256>> gm(base);
  MGATHER(old, gm, ix, IndexedByteRange{0, 0});
}
using DY = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor, DYNAMIC, DYNAMIC>;
extern "C" void indexed_dynamic(DY &out, DY &ix, uint32_t *base, int64_t min, int64_t max) {
  global_tensor<uint32_t, RowMajor<8, 256>> gm(base);
  MGATHER(out, gm, ix, IndexedByteRange{min, max});
}
