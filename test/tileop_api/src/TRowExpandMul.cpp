#include "../data.hpp"
#include <jcore/template_asm.hpp>  // TROWEXPAND* / TCOLEXPAND* / TCONCAT live here (no public wrapper yet)

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
  TLOAD(d0, g0);
  TLOAD(d1, g1);

  TROWEXPAND(d_out, d1);
  TROWEXPANDMUL(d_out, d0, d1);
  TROWEXPANDADD(d_out, d0, d1);
  TROWEXPANDSUB(d_out, d0, d1);
  TROWEXPANDDIV(d_out, d0, d1);
  TROWEXPANDMAX(d_out, d0, d1);
  TROWEXPANDMIN(d_out, d0, d1);
  TROWEXPANDEXPDIF(d_out, d0, d1);

  TSTORE(gd, d_out);
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
  TLOAD(d0, g0);
  TLOAD(d1, g1);

  TCOLEXPAND(d_out, d1);
  TCOLEXPANDMUL(d_out, d0, d1);
  TCOLEXPANDADD(d_out, d0, d1);
  TCOLEXPANDSUB(d_out, d0, d1);
  TCOLEXPANDDIV(d_out, d0, d1);
  TCOLEXPANDMAX(d_out, d0, d1);
  TCOLEXPANDMIN(d_out, d0, d1);
  TCOLEXPANDEXPDIF(d_out, d0, d1);

  TSTORE(gd, d_out);
}

// Regression for TCONCAT column-concat: dst = [src0 | src1].
// All three shapes differ by construction (R x C0, R x C1, R x (C0+C1));
// B.DIM now takes dst's geometry, dtype uniform, row count equal.
template <uint16_t row, uint16_t col0, uint16_t col1, typename T>
void test_concat(T *dst, T *s0, T *s1) {
  using gm_mat0 = global_tensor<T, RowMajor<row, col0>>;
  using gm_mat1 = global_tensor<T, RowMajor<row, col1>>;
  using gm_out  = global_tensor<T, RowMajor<row, col0 + col1>>;

  using tile_in0 = Tile<Location::Vec, T, row, col0, BLayout::RowMajor, row, col0>;
  using tile_in1 = Tile<Location::Vec, T, row, col1, BLayout::RowMajor, row, col1>;
  using tile_out = Tile<Location::Vec, T, row, col0 + col1, BLayout::RowMajor,
                        row, col0 + col1>;

  gm_mat0 g0(s0);
  gm_mat1 g1(s1);
  gm_out  gd(dst);

  tile_in0 d0;
  tile_in1 d1;
  tile_out d_out;
  TLOAD(d0, g0);
  TLOAD(d1, g1);

  TCONCAT(d_out, d0, d1);   // d_out = [d0 | d1]  (R x (C0+C1))

  TSTORE(gd, d_out);
}

int main() {
  const uint16_t row = 16;
  const uint16_t col = 16;
  const uint16_t cat_half = col / 2;   // 8: concat two 16x8 -> 16x16
  size_t size_mat = row * col;
  size_t size_row_vec = row;   // R scalars for row broadcast
  size_t size_col_vec = col;   // C scalars for col broadcast
  size_t size_half = row * cat_half;   // 16x8 each concat source

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

  __half *c0 = (__half *)malloc(size_half * sizeof(__half));
  check_mem_alloc(c0);
  init_src_fp(c0, size_half);

  __half *c1 = (__half *)malloc(size_half * sizeof(__half));
  check_mem_alloc(c1);
  init_src_fp(c1, size_half);

#ifdef LINX_PMC
  PMC_START();
#endif

  test_row_vector_src1<row, col, __half>(dst, s0, s1_row);
  test_col_vector_src1<row, col, __half>(dst, s0, s1_col);
  test_concat<row, cat_half, cat_half, __half>(dst, c0, c1);

#ifdef LINX_PMC
  PMC_END();
#endif

  printf("Result:\n");
  OutArray(dst, size_mat);

  free(dst);
  free(s0);
  free(s1_row);
  free(s1_col);
  free(c0);
  free(c1);
  return 0;
}
