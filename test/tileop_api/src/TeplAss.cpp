// Associated TEPL destinations are passed first at the C++ API boundary. The
// implementation still emits source B.IOTs before the destination-only B.IOT.
#include <common/pto_tileop.hpp>

using namespace pto;

using TileT = Tile<Location::Vec, float, 16, 16>;

__attribute__((noinline)) void tepl_ass_binary(TileT &a, TileT &b, TileT &d) {
  auto ad = range::assemble(d);
  TADD_ASS(ad, a, b);
  TSUB_ASS(ad, a, b);
  TMUL_ASS(ad, a, b);
  TDIV_ASS(ad, a, b);
  TREM_ASS(ad, a, b);
  TAND_ASS(ad, a, b);
  TOR_ASS(ad, a, b);
  TXOR_ASS(ad, a, b);
  TSHL_ASS(ad, a, b);
  TSHR_ASS(ad, a, b);
  TMAX_ASS(ad, a, b);
  TMIN_ASS(ad, a, b);
}

__attribute__((noinline)) void tepl_ass_unary(TileT &src, TileT &d) {
  auto ad = range::assemble(d);
  TABS_ASS(ad, src);
  TNOT_ASS(ad, src);
  TNEG_ASS(ad, src);
  TEXP_ASS(ad, src);
  TLOG_ASS(ad, src);
  TRECIP_ASS(ad, src);
}

__attribute__((noinline)) void tepl_ass_scalar(TileT &src, TileT &d) {
  auto ad = range::assemble(d);
  TADDS_ASS(ad, src, 1.0f);
  TSUBS_ASS(ad, src, 1.0f);
  TMULS_ASS(ad, src, 1.0f);
  TDIVS_ASS(ad, src, 1.0f);
  TREMS_ASS(ad, src, 1.0f);
  TANDS_ASS(ad, src, 1.0f);
  TORS_ASS(ad, src, 1.0f);
  TXORS_ASS(ad, src, 1.0f);
  TSHLS_ASS(ad, src, 1.0f);
  TSHRS_ASS(ad, src, 1.0f);
  TMAXS_ASS(ad, src, 1.0f);
  TMINS_ASS(ad, src, 1.0f);
}

int main() {
  TileT a, b, d;
  tepl_ass_binary(a, b, d);
  tepl_ass_unary(a, d);
  tepl_ass_scalar(a, d);
  return 0;
}