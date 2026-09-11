#include <common/pto_tileop.hpp>

using namespace pto;

template <typename T>
using GMovTile = Tile<Location::Vec, T, 8, 16, BLayout::RowMajor>;

template <int PEMask, typename T>
__attribute__((noinline)) void gmov_type(uint64_t peer_tid) {
  GMovTile<T> dst;
  GMovTile<T> src;
  GMOV<PEMask>(dst, peer_tid, src);
}

int main() {
  gmov_type<3, float>(0);       // 0011: partial, non-legacy mask
  gmov_type<5, __bf16>(0);      // 0101: partial, non-legacy mask
  gmov_type<10, int32_t>(0);    // 1010: partial, non-legacy mask
  gmov_type<13, uint8_t>(0);    // 1101: partial, non-legacy mask
  gmov_type<15, __fp4_e2m1x2>(0);
  gmov_type<1, __uint4x2>(0);   // single-PE mask
  return 0;
}