// PTO-ISA 0.58.4 ADR-0098 range modifiers: B.ASSEMBLE destination-side
// range carrier on a Local destination binder.
//
//   B.IOT mask=1111, last, ->%[dst]<TSize>
//   B.ASSEMBLE INIT, LAST, RegSrc, uimm11, ParentSizeCode
//
// The destination tile is wrapped in pto::range::Assemble which forwards
// the parent shape/dtype/storage and carries INIT/LAST + the range
// descriptor as compile-time wrapper parameters. The assembler output must
// contain the canonical B.ASSEMBLE line after the destination binder
// (verified by objdump in the target toolchain).
//
// Compile-time contract:
//   - ParentSizeCode must be 0..12 (13..15 reserved).
//   - destination roles only map to B.ASSEMBLE (never B.SUBVIEW).

#include <common/pto_tileop.hpp>

using namespace pto;

using Dst = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GMDst = global_tensor<float, RowMajor<4, 8>>;

// Local destination assemble: load GM into an Assemble-wrapped destination.
// INIT/LAST/Offset/RegSrc are compile-time in the wrapper so the B.ASSEMBLE
// slots stay static "i" immediates (ADR-0098 range descriptor).
__attribute__((noinline)) void assemble_dest_tload(
    GMDst &src, Dst &d) {
  range::Assemble<Dst, 12, /*INIT*/ true, /*LAST*/ false, /*Off*/ 0,
                  /*RegSrc*/ 0> as(d, 0);
  TLOAD(as, src); // -> B.ASSEMBLE 1, 0, r0, 0, 12
}

// Assemble with explicit RegSrc=23 (r23) base-address register.
__attribute__((noinline)) void assemble_regsrc23_tload(
    GMDst &src, Dst &d) {
  range::Assemble<Dst, 12, /*INIT*/ true, /*LAST*/ false, /*Off*/ 1,
                  /*RegSrc*/ 23> as(d, 23);
  TLOAD(as, src); // -> B.ASSEMBLE 1, 0, 23, 1, 12
}

// ParentSizeCode 0 boundary (non-INIT).
__attribute__((noinline)) void assemble_size0_tload(
    GMDst &src, Dst &d) {
  range::Assemble<Dst, 0, /*INIT*/ false, /*LAST*/ true, /*Off*/ 2047,
                  /*RegSrc*/ 2> as(d, 2);
  TLOAD(as, src); // -> B.ASSEMBLE 0, 1, 2, 2047, 0
}

// The factory derives the INIT parent size from Dst for the common case.
__attribute__((noinline)) void assemble_factory_tload(
    GMDst &src, Dst &d) {
  auto as = range::assemble(d, 0);
  TLOAD(as, src);
}

__attribute__((noinline)) void assemble_factory_custom_tload(
    GMDst &src, Dst &d) {
  auto as = range::assemble_last_at<2047>(d, 2);
  TLOAD(as, src);
}

__attribute__((noinline)) void assemble_factory_init_last_tload(
    GMDst &src, Dst &d) {
  auto as = range::assemble_init_last(d, 0);
  TLOAD(as, src);
}

__attribute__((noinline)) void assemble_factory_middle_tload(
    GMDst &src, Dst &d) {
  auto as = range::assemble_middle(d, 0);
  TLOAD(as, src);
}

void use(void *) {}
int main() {
  float src_buf[4 * 8];
  GMDst gs(src_buf);
  Dst d;
  assemble_dest_tload(gs, d);
  assemble_regsrc23_tload(gs, d);
  assemble_size0_tload(gs, d);
  assemble_factory_tload(gs, d);
  assemble_factory_custom_tload(gs, d);
  assemble_factory_init_last_tload(gs, d);
  assemble_factory_middle_tload(gs, d);
  use(src_buf);
  return 0;
}
