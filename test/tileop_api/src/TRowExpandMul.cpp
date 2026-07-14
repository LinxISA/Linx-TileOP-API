#include "../data.hpp"
#include <jcore/template_asm.hpp>  // TROWEXPAND* / TCOLEXPAND* live here (no public wrapper yet)

#ifdef LINX_PMC
#include "../linxStartEnd.hpp"
#endif

// Regression for the TROWEXPAND* / TCOLEXPAND* shape split: src1 may carry a
// different shape (per-row / per-column scalar vector in PTO Mode 1) from src0;
// only dtype must match.
// See docs/tileop-usage/reduce-broadcast.md and pto/TROWEXPANDMUL.md.

template <uint16_t row, uint16_t col, typename T>
void test_row_vector_src1(T *dst, T *s0, T *s1) {
  using gm_mat = global_tensor<T, RowMajor<row, col>>;
  using gm_vec = global_tensor<T, RowMajor<row, 1>>;

  // src0 / dst : R x C  (square matrix)
  using tile_mat = Tile<Location::Vec, T, row, col, BLayout::RowMajor, row, col>;
  // src1        : R x 1  (one scalar per row -- DIFFERENT shape from src0)
  using tile_vec = Tile<Location::Vec, T, row, 1, BLayout::RowMajor, row, 1>;

  gm_mat g0(s0), gd(dst);
  gm_vec g1(s1);

  tile_mat d0, d_out;
  tile_vec d1;
  TCOPYIN(d0, g0);
  TCOPYIN(d1, g1);

  TROWEXPANDMUL(d_out, d0, d1);
  TROWEXPANDADD(d_out, d0, d1);
  TROWEXPANDSUB(d_out, d0, d1);
  TROWEXPANDDIV(d_out, d0, d1);
  TROWEXPANDMAX(d_out, d0, d1);
  TROWEXPANDMIN(d_out, d0, d1);
  TROWEXPANDEXPDIF(d_out, d0, d1);

  TCOPYOUT(gd, d_out);
}

template <uint16_t row, uint16_t col, typename T>
void test_col_vector_src1(T *dst, T *s0, T *s1) {
  using gm_mat = global_tensor<T, RowMajor<row, col>>;
  using gm_vec = global_tensor<T, RowMajor<1, col>>;

  // src0 / dst : R x C  (square matrix)
  using tile_mat = Tile<Location::Vec, T, row, col, BLayout::RowMajor, row, col>;
  // src1        : 1 x C  (one scalar per col -- DIFFERENT shape from src0)
  using tile_vec = Tile<Location::Vec, T, 1, col, BLayout::RowMajor, 1, col>;

  gm_mat g0(s0), gd(dst);
  gm_vec g1(s1);

  tile_mat d0, d_out;
  tile_vec d1;
  TCOPYIN(d0, g0);
  TCOPYIN(d1, g1);

  TCOLEXPANDMUL(d_out, d0, d1);
  TCOLEXPANDADD(d_out, d0, d1);
  TCOLEXPANDSUB(d_out, d0, d1);
  TCOLEXPANDDIV(d_out, d0, d1);
  TCOLEXPANDMAX(d_out, d0, d1);
  TCOLEXPANDMIN(d_out, d0, d1);
  TCOLEXPANDEXPDIF(d_out, d0, d1);

  TCOPYOUT(gd, d_out);
}

int main() {
  const uint16_t row = 16;
  const uint16_t col = 16;
  size_t size_mat = row * col;
  size_t size_row_vec = row;   // R scalars for row broadcast
  size_t size_col_vec = col;   // C scalars for col broadcast

  __half *dst = (__half *)malloc(size_mat * sizeof(__half));
  check_mem_alloc(dst);
  init_dst(dst, size_mat);

  __half *s0 = (__half *)malloc(size_mat * sizeof(__half));
  check_mem_alloc(s0);
  init_src_fp(s0, size_mat);

  __half *s1_row = (__half *)malloc(size_row_vec * sizeof(__half));
  check_mem_alloc(s1_row);
  init_src_fp(s1_row, size_row_vec);

  __half *s1_col = (__half *)malloc(size_col_vec * sizeof(__half));
  check_mem_alloc(s1_col);
  init_src_fp(s1_col, size_col_vec);

#ifdef LINX_PMC
  PMC_START();
#endif

  test_row_vector_src1<row, col, __half>(dst, s0, s1_row);
  test_col_vector_src1<row, col, __half>(dst, s0, s1_col);

#ifdef LINX_PMC
  PMC_END();
#endif

  printf("Result:\n");
  OutArray(dst, size_mat);

  free(dst);
  free(s0);
  free(s1_row);
  free(s1_col);
  return 0;
}
