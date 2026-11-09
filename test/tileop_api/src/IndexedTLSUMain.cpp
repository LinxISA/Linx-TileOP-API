// Main contract: byte displacements and GM-base-only B.IOR across 23 operations.
#include <common/pto_tileop.hpp>
using namespace pto;
using D = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using I = Tile<Location::Vec, int32_t, 8, 256, BLayout::RowMajor>;
using P = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
extern "C" void indexed_all(D &old, I &ix, D &v, D &expected, P &mask, uint32_t *base) {
  global_tensor<uint32_t, RowMajor<8, 256>> gm(base);
  MGATHER(old, gm, ix);
  MSCATTER(gm, v, ix);
  MGATHER_MASK(old, gm, ix, mask);
  MSCATTER_MASK(gm, v, ix, mask);
  MGATHER_CAS(old, reinterpret_cast<uintptr_t>(base), ix, expected, v, 256, 2);
  MGATHER_EXCH(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_MAX(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_MIN(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_ADD(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_INC(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_DEC(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_AND(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_OR(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MGATHER_XOR(old, reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_MAX(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_MIN(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_ADD(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_INC(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_DEC(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_AND(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_OR(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_XOR(reinterpret_cast<uintptr_t>(base), ix, v, 256, 2);
  MSCATTER_POPC(reinterpret_cast<uintptr_t>(base), ix, 256, 2);
}
using DY = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor, DYNAMIC, DYNAMIC>;
extern "C" void indexed_dynamic(DY &out, DY &ix, uint32_t *base) {
  global_tensor<uint32_t, RowMajor<8, 256>> gm(base);
  MGATHER(out, gm, ix);
}
