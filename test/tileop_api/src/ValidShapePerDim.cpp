// Per-dimension static/dynamic B.DIM regression: all four ValidCol/ValidRow
// combinations must lower to the correct B.DIM form.
//   static dim  -> B.DIM zero, imm   (assembler compresses to C.B.DIMI)
//   dynamic dim -> B.DIM reg, 0
#include <jcore/template_asm.hpp>

using namespace pto;

// SS: both dims static
using TSS = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 16, 32>;
// SD: static col, dynamic row
using TSD = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 16, -1>;
// DS: dynamic col, static row
using TDS = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, -1, 32>;
// DD: both dynamic
using TDD = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, -1, -1>;

__attribute__((noinline)) void case_ss_static_both(TSS &a, TSS &b, TSS &c) { TADD(a, b, c); }
__attribute__((noinline)) void case_sd_static_col(TSD &a, TSD &b) { TADD(a, b, b); }
__attribute__((noinline)) void case_ds_static_row(TDS &a, TDS &b) { TADD(a, b, b); }
__attribute__((noinline)) void case_dd_dynamic_both(TDD &a, TDD &b) { TADD(a, b, b); }

__attribute__((noinline)) void case_tmul_sd_ds(TSD &a, TSD &b, TDS &c, TDS &d) {
  TMUL(a, b, b);
  TMUL(c, d, d);
}

__attribute__((noinline)) void case_trecip_dynamic(TDD &a, TDD &b) { TRECIP(a, b); }

__attribute__((noinline)) void case_texp_mixed(TSD &a, TSD &b) { TEXP(a, b); }

int main() {
  static TSS ss_a, ss_b, ss_c;
  static TSD sd_a, sd_b;
  static TDS ds_a, ds_b;
  static TDD dd_a, dd_b;
  case_ss_static_both(ss_a, ss_b, ss_c);
  case_sd_static_col(sd_a, sd_b);
  case_ds_static_row(ds_a, ds_b);
  case_dd_dynamic_both(dd_a, dd_b);
  case_tmul_sd_ds(sd_a, sd_b, ds_a, ds_b);
  case_trecip_dynamic(dd_a, dd_b);
  case_texp_mixed(sd_a, sd_b);
  return 0;
}
