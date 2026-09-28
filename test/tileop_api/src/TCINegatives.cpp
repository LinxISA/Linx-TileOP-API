#include <common/pto_tileop.hpp>

using namespace pto;

using M16 = CubeTileM16<int32_t, 16, 16>;
using M16TooManyRows = CubeTileM16<int32_t, 16, 16, 17, 16>;
using M16ZeroRows = CubeTileM16<int32_t, 16, 16, 0, 16>;
using M32 = CubeTileM32<int32_t, 32, 32>;
using Vec = Tile<Location::Vec, int32_t, 16, 16, BLayout::RowMajor>;
using NonCubeMatrix = Tile<Location::Left, int32_t, 16, 16,
                            BLayout::RowMajor>;
using FloatM16 = CubeTileM16<float, 16, 16>;

#if defined(SHOULD_FAIL_TCI2D_LAYOUT)
void reject_layout(NonCubeMatrix &dst) {
  TCI_2D<NonCubeMatrix, int32_t, 1, 0>(dst, 0);
}
#elif defined(SHOULD_FAIL_TCI2D_LOCATION)
void reject_location(Vec &dst) {
  TCI_2D<Vec, int32_t, 1, 0>(dst, 0);
}
#elif defined(SHOULD_FAIL_TCI2D_DTYPE)
void reject_dtype(FloatM16 &dst) {
  TCI_2D<FloatM16, float, 1, 0>(dst, 0.0f);
}
#elif defined(SHOULD_FAIL_TCI2D_SHAPE)
void reject_shape(M16ZeroRows &dst) {
  TCI_2D<M16ZeroRows, int32_t, 1, 0>(dst, 0);
}
#elif defined(SHOULD_FAIL_TCI2D_STEPS)
void reject_steps(M32 &dst) {
  TCI_2D<M32, int32_t, 2, 0>(dst, 0);
}
#elif defined(SHOULD_FAIL_TCI2D_M16_ROWS)
void reject_m16_rows(M16TooManyRows &dst) {
  TCI_2D<M16TooManyRows, int32_t, 1, 0>(dst, 0);
}
#endif

int main() { return 0; }