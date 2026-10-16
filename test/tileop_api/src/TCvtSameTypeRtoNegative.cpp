// PTO-SPEC #388: same-type floating TCVT with explicit RTO is illegal.
#include <common/pto_tileop.hpp>

using namespace pto;

#if defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_FP16)
using T = Tile<Location::Vec, __half, 16, 16, BLayout::RowMajor>;
void reject(T &dst, T &src) { TCVT<LINX_RTO>(dst, src); }
#elif defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_BF16)
using T = Tile<Location::Vec, __bf16, 16, 16, BLayout::RowMajor>;
void reject(T &dst, T &src) { TCVT<LINX_RTO>(dst, src); }
#elif defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_FP32)
using T = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
void reject(T &dst, T &src) { TCVT<LINX_RTO>(dst, src); }
#elif defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_FP64)
using T = Tile<Location::Vec, double, 16, 16, BLayout::RowMajor>;
void reject(T &dst, T &src) { TCVT<LINX_RTO>(dst, src); }
#elif defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_REINTERPRETED)
using Backing = Tile<Location::Vec, uint32_t, 16, 16, BLayout::RowMajor>;
using FloatTile = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
void reject(FloatTile &dst, Backing &backing) {
  auto operation_view = reinterpret_tile<float>(backing);
  TCVT<LINX_RTO>(dst, operation_view);
}
#elif defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_PREFIX)
using Parent = VecTileM32<float, 32, 128, 32, 1>;
using Cell = VecTileM32<float, 32, 1, 32, 1>;
void reject(Cell &dst, Parent &parent) {
  auto prefix = TREDUCEPREFIXVIEW<Cell>(parent);
  TCVT<LINX_RTO>(dst, prefix);
}
#elif defined(SHOULD_FAIL_TCVT_SAME_TYPE_RTO_TILEARRAY)
using Fragment = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
void reject(Fragment &src) {
  TileArray<Fragment, 1, 1> destinations;
  auto slot = destinations[0][0];
  TCVT<LINX_RTO>(slot, src);
}
#endif

int main() { return 0; }
