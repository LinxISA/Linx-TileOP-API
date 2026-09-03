// Regression coverage for static/dynamic valid-shape immediate lowering.
//
// The 2026-09-01 valid-shape batch converted ordinary TileOP inline asm so
// that a statically-typed Tile (ValidRow/ValidCol > 0) binds LB0/LB1 as "i"
// immediates (C.B.DIMI eligible), while dynamic Tiles (ValidRow/ValidCol ==
// -1) keep the "r" register form. This fixture instantiates every converted
// family on both path kinds so template-instantiation errors, wrong-template
// name typos (e.g. the original TCONCAT "tile_shape" bug), and operand
// misbindings are caught by the compile gate.
//
// It is a compile-only regression: main() is minimal. The objective gate is
// `make TESTCASE=ValidShapeImmediate object` under the matching Linx
// toolchain; locally it must at least pass -fsyntax-only on both paths.

#include <common/pto_tileop.hpp>

using namespace pto;

// Static half: a fully-static Tile drives the "i" + C.B.DIMI path.
using S = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using SR = Tile<Location::Vec, float, 16, 1, BLayout::RowMajor>;   // row-reduce dst / row-expand source
using SC = Tile<Location::Vec, float, 1, 16, BLayout::RowMajor>;   // col-reduce dst / col-expand source
using SE = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;  // expand destination
using SO = Tile<Location::Vec, uint16_t, 16, 16, BLayout::RowMajor>;

__attribute__((noinline)) void static_path(S &d, S &a, S &b, S &c,
                                           SR &r, SC &rc, SE &ex,
                                           SO &off, float v) {
  // elementwise batch
  TADD(d, a, b);
  TSUB(d, a, b);
  TMUL(d, a, b);
  TDIV(d, a, b);
  TREM(d, a, b);
  TAND(d, a, b);
  TOR(d, a, b);
  TXOR(d, a, b);
  TSHL(d, a, b);
  TSHR(d, a, b);
  TMAX(d, a, b);
  TMIN(d, a, b);
  // reduce / broadcast batch
  TROWSUM(r, a);
  TROWMAX(r, a);
  TROWMIN(r, a);
  TROWARGMAX(r, a);
  TROWEXPAND(ex, r);
  TCOLSUM(rc, a);
  TCOLEXPAND(ex, rc);
  TROWEXPANDADD(ex, d, r);
  TROWEXPANDSUB(ex, d, r);
  TROWEXPANDMUL(ex, d, r);
  // unary / scalar batch
  TABS(d, a);
  TNOT(d, a);
  TNEG(d, a);
  TEXP(d, a);
  TLOG(d, a);
  TSQRT(d, a);
  TRSQRT(d, a);
  TRELU(d, a);
  TSUBS(d, a, v);
  TDIVS(d, a, v);
  TREMS(d, a, v);
  TANDS(d, a, v);
  TORS(d, a, v);
  TXORS(d, a, v);
  TSHLS(d, a, v);
  TSHRS(d, a, v);
  TMAXS(d, a, v);
  TMINS(d, a, v);
  TEXPANDS(d, v);
  TFMA(d, a, b, c);
  // part / gather batch
  TSEL(d, a, b);
  TPARTADD(d, a, b);
  TPARTMUL(d, a, b);
  TPARTMAX(d, a, b);
  TPARTMIN(d, a, b);
  TGATHER(d, a, off);
  TSCATTER(a, d, off);
  // movement / concat batch (TCONCAT regression: dst-derived valid shape)
  TEXTRACT(c, a, 1, 2);
  TINSERT(c, a, 1, 2);
  TTRANS(c, a);
  // TCONCAT requires src0/src1/dst to share dtype and row count; the valid
  // shape used by its B.DIM is dst-derived (tile_shape_out), which regresses
  // the original "tile_shape" undeclared-identifier bug.
  TCONCAT(d, a, b);
}

// Dynamic half: same families on dynamic Tiles drive the "r" + B.DIM path.
using D = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, -1, -1>;
using DR = Tile<Location::Vec, float, 16, 1, BLayout::RowMajor, -1, -1>;   // row-reduce dst / row-expand source
using DC = Tile<Location::Vec, float, 1, 16, BLayout::RowMajor, -1, -1>;   // col-reduce dst / col-expand source
using DE = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, -1, -1>;  // expand destination
using DO = Tile<Location::Vec, uint16_t, 16, 16, BLayout::RowMajor, -1, -1>;

__attribute__((noinline)) void dynamic_path(D &d, D &a, D &b, D &c,
                                            DR &r, DC &rc, DE &ex,
                                            DO &off, float v) {
  TADD(d, a, b);
  TSUB(d, a, b);
  TMUL(d, a, b);
  TROWSUM(r, a);
  TROWMAX(r, a);
  TROWEXPAND(ex, r);
  TROWEXPANDADD(ex, d, r);
  TABS(d, a);
  TEXP(d, a);
  TSUBS(d, a, v);
  TMAXS(d, a, v);
  TSEL(d, a, b);
  TPARTMUL(d, a, b);
  TGATHER(d, a, off);
  TSCATTER(a, d, off);
  TEXTRACT(c, a, 1, 2);
  TTRANS(c, a);
  TCONCAT(d, a, b);
}

int main() {
  S s, sa, sb, sc;
  SR sr;
  SC src;
  SE sex;
  SO soff;
  D d, da, db, dc;
  DR dr;
  DC drc;
  DE dex;
  DO doff;
  static_path(s, sa, sb, sc, sr, src, sex, soff, 1.0f);
  dynamic_path(d, da, db, dc, dr, drc, dex, doff, 1.0f);
  return 0;
}