#include <common/pto_tileop.hpp>

using namespace pto;

template <typename T>
using Vec = Tile<Location::Vec, T, 8, 16, BLayout::RowMajor>;

#if defined(SHOULD_FAIL_GMOV_FP64)
void reject(double, uint64_t peer) { Vec<double> d, s; GMOV(d, peer, s); }
#elif defined(SHOULD_FAIL_GMOV_S64)
void reject(int64_t, uint64_t peer) { Vec<int64_t> d, s; GMOV(d, peer, s); }
#elif defined(SHOULD_FAIL_GMOV_U64)
void reject(uint64_t peer) {
  Tile<Location::Vec, unsigned long, 8, 16, BLayout::RowMajor> d, s;
  GMOV(d, peer, s);
}
#elif defined(SHOULD_FAIL_GMOV_DTYPE)
void reject(uint64_t peer) { Vec<float> d; Vec<int32_t> s; GMOV(d, peer, s); }
#elif defined(SHOULD_FAIL_GMOV_SHAPE)
void reject(uint64_t peer) {
  Vec<float> d;
  Tile<Location::Vec, float, 16, 8, BLayout::RowMajor> s;
  GMOV(d, peer, s);
}
#elif defined(SHOULD_FAIL_GMOV_VALID_SHAPE)
void reject(uint64_t peer) {
  Tile<Location::Vec, float, 8, 16, BLayout::RowMajor, 8, 15> d;
  Vec<float> s;
  GMOV(d, peer, s);
}
#elif defined(SHOULD_FAIL_GMOV_LAYOUT)
void reject(uint64_t peer) {
  Vec<float> d;
  Tile<Location::Vec, float, 8, 16, BLayout::ColMajor> s;
  GMOV(d, peer, s);
}
#elif defined(SHOULD_FAIL_GMOV_CAPACITY)
void reject(uint64_t peer) {
  Vec<float> d;
  Tile<Location::Vec, float, 16, 16, BLayout::RowMajor> s;
  GMOV(d, peer, s);
}
#elif defined(SHOULD_FAIL_GMOV_LOCATION)
void reject(uint64_t peer) {
  Vec<float> d;
  Tile<Location::Left, float, 8, 16, BLayout::RowMajor> s;
  GMOV(d, peer, s);
}
#elif defined(SHOULD_FAIL_GMOV_SHARED)
void reject(uint64_t peer) {
  Vec<float> d;
  SharedTile<Vec<float>> s;
  GMOV(d, peer, s);
}
#endif

int main() { return 0; }