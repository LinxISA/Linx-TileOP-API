// PTO-ISA 0.58.4 ADR-0098 range modifiers: B.SUBVIEW source-side range
// carrier on a Local source binder.
//
//   B.IOT <src>, mask=1111, last
//   B.SUBVIEW SrcSelect, RegSrc, uimm11, SubviewSizeCode
//
// The source tile is wrapped in pto::range::Subview which forwards the
// parent shape/dtype/storage and carries the range descriptor as
// compile-time wrapper parameters. The assembler output must contain the
// canonical B.SUBVIEW line after the source binder (verified by objdump in
// the target toolchain).
//
// Compile-time contract:
//   - SubviewSizeCode must be 1..12 (128 B..256 KiB per PE).
//   - uimm11 offset units must be 0..2047; one unit is 128 B (wrapper-compile-time).
//   - source roles only map to B.SUBVIEW (never B.ASSEMBLE).

#include <common/pto_tileop.hpp>

using namespace pto;

using Src = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using MaxSrc = Tile<Location::Vec, float, 256, 256, BLayout::RowMajor>;
using GMSrc = global_tensor<float, RowMajor<4, 8>>;
using GMDst = global_tensor<float, RowMajor<4, 8>>;
using MaxGMDst = global_tensor<float, RowMajor<256, 256>>;

// Local source subview: store the subviewed source to GM.
__attribute__((noinline)) void subview_source_tstore(
    GMDst &dst, Src &s) {
  range::Subview<Src, 1, /*Off*/ 0, /*RegSrc*/ 0> sv(s, 0);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, r0, 0, 1
}

// SubviewSizeCode 12 boundary, max uimm11 offset-unit count, RegSrc=23 (r23).
__attribute__((noinline)) void subview_size12_tstore(
    MaxGMDst &dst, MaxSrc &s) {
  range::Subview<MaxSrc, 12, /*Off*/ 2047, /*RegSrc*/ 23> sv(s, 23);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, r23, 3, 12
}

// The no-base factory uses the zero register.
__attribute__((noinline)) void subview_factory_zero_tstore(
    GMDst &dst, Src &s) {
  auto sv = range::subview(s);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, zero, 0, 1
}

// A runtime base value uses a compiler-allocated GPR.
__attribute__((noinline)) void subview_factory_runtime_tstore(
    GMDst &dst, Src &s, uintptr_t base_units) {
  auto sv = range::subview(s, base_units);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, <allocated-gpr>, 0, 1
}

__attribute__((noinline)) void subview_factory_zero_offset_tstore(
    GMDst &dst, Src &s) {
  auto sv = range::subview<128, 3>(s);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, zero, 3, 1
}

__attribute__((noinline)) void subview_factory_runtime_offset_tstore(
    GMDst &dst, Src &s, uintptr_t base_units) {
  auto sv = range::subview<128, 3>(s, base_units);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, <allocated-gpr>, 3, 1
}

// Explicit register selection remains a low-level ABI/testing interface.
__attribute__((noinline)) void subview_factory_explicit_reg_tstore(
    GMDst &dst, Src &s) {
  auto sv = range::subview_at_reg<3, 23>(s, 23);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, r23, 3, 1
}

void use(void *) {}
int main() {
  float src_buf[4 * 8];
  float dst_buf[4 * 8];
  float max_src_buf[256 * 256];
  float max_dst_buf[256 * 256];
  GMSrc gs(src_buf);
  GMDst gd(dst_buf);
  MaxGMDst max_gd(max_dst_buf);
  Src s;
  MaxSrc max_s;
  subview_source_tstore(gd, s);
  subview_size12_tstore(max_gd, max_s);
  subview_factory_zero_tstore(gd, s);
  subview_factory_runtime_tstore(gd, s, 64);
  subview_factory_zero_offset_tstore(gd, s);
  subview_factory_runtime_offset_tstore(gd, s, 64);
  subview_factory_explicit_reg_tstore(gd, s);
  use(src_buf);
  return 0;
}
