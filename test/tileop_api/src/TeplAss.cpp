// Associated TEPL destinations are passed last, matching the B.IOT source
// sequence followed by the destination-only B.IOT.
#include <common/pto_tileop.hpp>

using namespace pto;

using TileT = Tile<Location::Vec, float, 16, 16>;

__attribute__((noinline)) void tepl_ass_binary(TileT &a, TileT &b, TileT &d) {
  auto ad = range::assemble(d);
  TADD_ASS(a, b, ad);
  TSUB_ASS(a, b, ad);
  TMUL_ASS(a, b, ad);
  TDIV_ASS(a, b, ad);
  TREM_ASS(a, b, ad);
  TAND_ASS(a, b, ad);
  TOR_ASS(a, b, ad);
  TXOR_ASS(a, b, ad);
  TSHL_ASS(a, b, ad);
  TSHR_ASS(a, b, ad);
  TMAX_ASS(a, b, ad);
  TMIN_ASS(a, b, ad);
}

__attribute__((noinline)) void tepl_ass_unary(TileT &src, TileT &d) {
  auto ad = range::assemble(d);
  TABS_ASS(src, ad);
  TNOT_ASS(src, ad);
  TNEG_ASS(src, ad);
  TEXP_ASS(src, ad);
  TLOG_ASS(src, ad);
  TRECIP_ASS(src, ad);
}

__attribute__((noinline)) void tepl_ass_scalar(TileT &src, TileT &d) {
  auto ad = range::assemble(d);
  TADDS_ASS(src, 1.0f, ad);
  TSUBS_ASS(src, 1.0f, ad);
  TMULS_ASS(src, 1.0f, ad);
  TDIVS_ASS(src, 1.0f, ad);
  TREMS_ASS(src, 1.0f, ad);
  TANDS_ASS(src, 1.0f, ad);
  TORS_ASS(src, 1.0f, ad);
  TXORS_ASS(src, 1.0f, ad);
  TSHLS_ASS(src, 1.0f, ad);
  TSHRS_ASS(src, 1.0f, ad);
  TMAXS_ASS(src, 1.0f, ad);
  TMINS_ASS(src, 1.0f, ad);
}

int main() {
  TileT a, b, d;
  tepl_ass_binary(a, b, d);
  tepl_ass_unary(a, d);
  tepl_ass_scalar(a, d);
  return 0;
}