#include <common/pto_tileop.hpp>
#include <utility>

using namespace pto;

using M16Parent = CubeTileM16<float, 16, 64>;
using M16Fragment = CubeTileM16<float, 16, 16>;
using M32Parent = CubeTileM32<float, 32, 64>;
using M32Fragment = CubeTileM32<float, 32, 16>;

static_assert(M16Parent::BFractal == BLayout::CubeM16);
static_assert(M32Parent::BFractal == BLayout::CubeM32);
static_assert(M16Parent::LogicalTileBytes ==
              4 * M16Fragment::LogicalTileBytes);
static_assert(M32Parent::LogicalTileBytes ==
              4 * M32Fragment::LogicalTileBytes);

void cube_m16_partition(M16Parent &parent) {
  auto parts = TPARTVIEW<M16Fragment, 1, 4>(parent);
  auto source = parts[0][2];
  (void)source;
}

void cube_m32_partition(M32Parent &parent) {
  auto parts = TPARTVIEW<M32Fragment, 1, 4>(parent);
  auto source = parts[0][2];
  (void)source;
}

int main() { return 0; }
