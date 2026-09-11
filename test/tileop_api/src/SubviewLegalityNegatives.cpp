#include <common/pto_tileop.hpp>

using namespace pto;

using RowMajorMatrix =
    Tile<Location::Left, float, 16, 16, BLayout::RowMajor>;
using VecCube = VecTileM16<float, 16, 16>;
using CubeMatrix = CubeTileM16<float, 16, 16>;
using SharedCube = SharedTile<CubeMatrix>;
using CubeParent = CubeTileM16<float, 16, 32>;
using CubeFragment = CubeTileM16<float, 16, 16>;

#if defined(SHOULD_FAIL_SUBVIEW_ROWMAJOR)
void reject(RowMajorMatrix &parent) { (void)range::subview(parent); }
#elif defined(SHOULD_FAIL_SUBVIEW_VEC_CUBE)
void reject(VecCube &parent) { (void)range::subview(parent); }
#elif defined(SHOULD_FAIL_TPARTVIEW_ROWMAJOR)
void reject(RowMajorMatrix &parent) {
  (void)TPARTVIEW<RowMajorMatrix, 1, 1>(parent);
}
#elif defined(SHOULD_FAIL_TPARTVIEW_SHARED)
void reject(SharedCube &parent) {
  (void)TPARTVIEW<CubeMatrix, 1, 1>(parent);
}
#elif defined(SHOULD_FAIL_TPARTVIEW_VEC_CUBE)
void reject(VecCube &parent) { (void)TPARTVIEW<VecCube, 1, 1>(parent); }
#endif

int main() { return 0; }