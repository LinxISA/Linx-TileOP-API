#include <common/pto_tileop.hpp>

using namespace pto;

using F = Tile<Location::Vec, float, 16, 16>;
using B = Tile<Location::Vec, __bf16, 16, 16>;
using R = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, 16, 1>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, 1, 16>;
using H = Tile<Location::Vec, float, 16, 8>;

__attribute__((noinline)) void tepl_ass_special(F &a, F &b, F &c, F &fd,
                                                B &bd, R &row, C &col,
                                                H &left, H &right) {
  // The parents are first written by plain allocating forms (or arrive as
  // already-live registers); the _ASS interface only consumes non-INIT
  // (middle/last) carriers.
  TADD(fd, a, b);                 // allocate the float parent
  TCVT(bd, a);                    // allocate the bf16 parent

  auto f = range::assemble_last(fd);
  auto d = range::assemble_last(bd);
  auto r = range::assemble_last(row);
  auto c0 = range::assemble_last(col);

  TCMP_ASS<CmpMode::LT>(f, a, b);
  TCMPS_ASS<CmpMode::GE>(f, a, 0.0f);
  TFMA_ASS(f, a, b, c);
  TSQRT_ASS(f, a);
  TRSQRT_ASS(f, a);
  TRELU_ASS(f, a);
  TCVT_ASS(d, a);
  TTRANS_ASS(f, a);
  TROWSUM_ASS(r, a);
  TROWMAX_ASS(r, a);
  TCOLSUM_ASS(c0, a);
  TROWEXPAND_ASS(f, row);
  TCOLEXPAND_ASS(f, col);
  TCONCAT_ASS(f, left, right);
}

int main() {
  F a, b, c, fd;
  B bd;
  R row;
  C col;
  H left, right;
  tepl_ass_special(a, b, c, fd, bd, row, col, left, right);
  return 0;
}