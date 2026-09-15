#include <common/pto_tileop.hpp>

using namespace pto;

using F = Tile<Location::Vec, float, 16, 16>;
using B = Tile<Location::Vec, __bf16, 16, 16>;
using R = Tile<Location::Vec, float, 16, 1, BLayout::RowMajor, 16, 1>;
using C = Tile<Location::Vec, float, 1, 16, BLayout::RowMajor, 1, 16>;
using H = Tile<Location::Vec, float, 16, 8>;

__attribute__((noinline)) void tepl_ass_special(F &a, F &b, F &c, F &fd,
                                                B &bd, R &row, C &col,
                                                H &left, H &right) {
  auto f = range::assemble(fd);
  auto d = range::assemble(bd);
  auto r = range::assemble(row);
  auto c0 = range::assemble(col);

  TCMP_ASS<CmpMode::LT>(f, a, b);
  TCMPS_ASS<CmpMode::GE>(f, a, 0.0f);
  TFMA_ASS(f, a, b, c);
  TSQRT_ASS(f, a);
  TRSQRT_ASS(f, a);
  TRELU_ASS(f, a);
  TCVT_ASS(d, a);
  TTRANS_ASS(f, a);
  // _ASS consumes only an already-open MIDDLE/LAST slot. Open the reduction
  // destinations with the plain producer form, then use LAST carriers for the
  // associated calls so their B.ASSEMBLE metadata is encoded.
  auto r_init = range::assemble_init_last(row);
  TROWSUM(r_init, a);
  auto r_last = range::assemble_last(row);
  TROWMAX_ASS(r_last, a);
  auto c_init = range::assemble_init_last(col);
  TCOLSUM(c_init, a);
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