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
  auto f = range::assemble(fd);
  auto d = range::assemble(bd);
  auto r = range::assemble(row);
  auto c0 = range::assemble(col);

  TCMP_ASS<CmpMode::LT>(a, b, f);
  TCMPS_ASS<CmpMode::GE>(a, 0.0f, f);
  TFMA_ASS(a, b, c, f);
  TSQRT_ASS(a, f);
  TRSQRT_ASS(a, f);
  TRELU_ASS(a, f);
  TCVT_ASS(a, d);
  TTRANS_ASS(a, f);
  TROWSUM_ASS(a, r);
  TROWMAX_ASS(a, r);
  TCOLSUM_ASS(a, c0);
  TROWEXPAND_ASS(row, f);
  TCOLEXPAND_ASS(col, f);
  TCONCAT_ASS(left, right, f);
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