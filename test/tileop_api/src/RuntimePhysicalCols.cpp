#include <common/pto_tileop.hpp>

using Wide = Tile<Location::Vec, float, 32, 256,
                  BLayout::CubeM32, DYNAMIC, DYNAMIC>;
using Row = Tile<Location::Vec, float, 32, 1,
                 BLayout::CubeM32, DYNAMIC, 1>;

static_assert(pto_elementwise_user_dims::user_dims_legal<Wide>(6, 2, 6));
static_assert(pto_elementwise_user_dims::user_dims_legal<Wide>(5, 2, 5));
static_assert(!pto_elementwise_user_dims::user_dims_legal<Wide>(0, 2, 6));
static_assert(!pto_elementwise_user_dims::user_dims_legal<Wide>(7, 2, 6));
static_assert(!pto_elementwise_user_dims::user_dims_legal<Wide>(6, 2, 257));
static_assert(!pto_elementwise_user_dims::user_dims_legal<Wide>(6, 33, 6));

__attribute__((noinline)) void runtime_physical_cols(
    Wide &dst, Wide &left, Wide &right, Row &row,
    unsigned validCol, unsigned validRow, unsigned physicalCol) {
  TEXPANDS(dst, 0.0f, validCol, validRow, physicalCol);
  TADD(dst, left, right, validCol, validRow, physicalCol);
  TSUB(dst, left, right, validCol, validRow, physicalCol);
  TROWEXPANDMUL(dst, left, row, validCol, validRow, physicalCol);
}

__attribute__((noinline)) void odd_runtime_physical_cols(
    Wide &dst, Wide &left, Wide &right) {
  TADD(dst, left, right, 5, 2, 5);
}

// Zero overrides use the instance's valid dimensions for dynamic Tile types.
__attribute__((noinline)) void runtime_valid_shape_defaults(
    Wide &dst, Wide &left, Wide &right) {
  TEXPANDS(dst, 0.0f, 0, 0, 6);
  TADD(dst, left, right, 0, 0, 6);
}

int main() { return 0; }
