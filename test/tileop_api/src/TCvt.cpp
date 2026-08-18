#include "../data.hpp"
#include <common/pto_tileop.hpp>

#ifdef LINX_PMC
#include "../linxStartEnd.hpp"
#endif

using tcvt_fp16_2x1024 =
    Tile<Location::Vec, __half, 2, 1024, BLayout::RowMajor>;
using tcvt_fp32_2x1024 =
    Tile<Location::Vec, float, 2, 1024, BLayout::RowMajor>;
using tile_i32_32x64 =
    Tile<Location::Vec, int32_t, 32, 64, BLayout::RowMajor>;

static_assert(tcvt_fp16_2x1024::TilesizeCode == __tilesize_4KB);
static_assert(tcvt_fp32_2x1024::TilesizeCode == __tilesize_8KB);
static_assert(tile_i32_32x64::TilesizeCode == __tilesize_8KB);
#ifdef __linx
static_assert(
    tile_type_traits<typename tcvt_fp32_2x1024::TileDType>::TilesizeCode ==
    tcvt_fp32_2x1024::TilesizeCode);
static_assert(
    tile_type_traits<typename tile_i32_32x64::TileDType>::TilesizeCode ==
    tile_i32_32x64::TilesizeCode);
#endif

template <uint16_t row, uint16_t col> void testRow2Nz(float *dst, float *src) {
  using gm_shape = global_tensor<float, RowMajor<row, col>>;

  using tile_shape_in = Tile<Location::Vec, float, row, col, BLayout::RowMajor>;
  using tile_shape_out = TileLeft<float, row, col>;

  gm_shape s0(src);
  gm_shape res(dst);

  tile_shape_in d0;
  tile_shape_out d1;

  TLOAD(d0, s0);
  TCVT(d1, d0);
  TCVT(d0, d1);
  TSTORE(res, d0);
}

template <uint16_t row, uint16_t col> void testNz2Col(float *dst, float *src) {
  using gm_shape = global_tensor<float, RowMajor<row, col>>;

  using tile_shape_in = TileLeft<float, row, col>;
  using tile_shape_out = Tile<Location::Vec, float, row, col, BLayout::RowMajor>;

  gm_shape s0(src);
  gm_shape res(dst);

  tile_shape_in d0;
  tile_shape_out d1;

  TLOAD(d0, s0);
  TCVT(d1, d0);
  TCVT(d0, d1);
  TSTORE(res, d0);
}

template <uint16_t row, uint16_t col> void testNz2Zn(float *dst, float *src) {
  using gm_shape = global_tensor<float, RowMajor<row, col>>;

  using tile_shape_in = TileLeft<float, row, col>;
  using tile_shape_out = TileRight<float, row, col>;

  gm_shape s0(src);
  gm_shape res(dst);

  tile_shape_in d0;
  tile_shape_out d1;

  TLOAD(d0, s0);
  TCVT(d1, d0);
  TCVT(d0, d1);
  TSTORE(res, d0);
}

template <uint16_t row, uint16_t col> void testZn2Nz(float *dst, float *src) {
  using gm_shape = global_tensor<float, RowMajor<row, col>>;

  using tile_shape_in = TileRight<float, row, col>;
  using tile_shape_out = TileLeft<float, row, col>;

  gm_shape s0(src);
  gm_shape res(dst);

  tile_shape_in d0;
  tile_shape_out d1;

  TLOAD(d0, s0);
  TCVT(d1, d0);
  TCVT(d0, d1);
  TSTORE(res, d0);
}

template <uint16_t row, uint16_t col> void testNz2Nz(float *dst, float *src) {
  using gm_shape = global_tensor<float, RowMajor<row, col>>;

  using tile_shape_in = TileLeft<float, row, col>;
  using tile_shape_out = TileLeft<float, row, col>;

  gm_shape s0(src);
  gm_shape res(dst);

  tile_shape_in d0;
  tile_shape_out d1;

  TLOAD(d0, s0);
  TCVT(d1, d0);
  TCVT(d0, d1);
  TSTORE(res, d0);
}

int main() {
  const uint16_t row = 16;
  const uint16_t col = 32;

  size_t size = row * col;

  float *dst = (float *)malloc(size * sizeof(float));
  check_mem_alloc(dst);
  init_dst(dst, size);

  float *src = (float *)malloc(size * sizeof(float));
  check_mem_alloc(src);
  init_src_fp(src, size);

  float *dst1 = (float *)malloc(size * sizeof(float));
  check_mem_alloc(dst1);
  init_dst(dst1, size);

  float *src1 = (float *)malloc(size * sizeof(float));
  check_mem_alloc(src1);
  init_rows_fp(src1, row, col);

  float *dst2 = (float *)malloc(size * sizeof(float));
  check_mem_alloc(dst2);
  init_dst(dst2, size);

  float *src2 = (float *)malloc(size * sizeof(float));
  check_mem_alloc(src2);
  init_rows_fp(src2, row, col);

#ifdef LINX_PMC
  PMC_START();
#endif

  testRow2Nz<row, col>(dst, src);
  testNz2Col<row, col>(dst1, src1);
  testNz2Zn<row, col>(dst2, src2);

#ifdef LINX_PMC
  PMC_END();
#endif

  printf("Result:\n");
  OutArray(dst, size);
  OutArray(dst1, size);
  OutArray(dst2, size);

  free(dst);
  free(src);
  free(dst1);
  free(src1);
  free(dst2);
  free(src2);

  return 0;
}
