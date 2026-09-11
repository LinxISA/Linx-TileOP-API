// PTO-ISA 0.58.4 ADR-0098 range modifiers: B.SUBVIEW source-side range
// carrier on an assigned Local Matrix+CUBE source binder.
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

using Src = CubeTileM16<float, 16, 16>;
using Dst = global_tensor<float, RowMajor<16, 16>>;
using MaxSrc = CubeTileM32<float, 32, 2048>;
using MaxDst = global_tensor<float, RowMajor<32, 2048>>;

// Local Matrix+CUBE source subview consumed by the source-side TSTORE path.
__attribute__((noinline)) void subview_source_tstore(
    Dst &dst, Src &s) {
  range::Subview<Src, 1, /*Off*/ 0, /*RegSrc*/ 0> sv(s, 0);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, r0, 0, 1
}

// SubviewSizeCode 12 boundary, max uimm11 offset-unit count, RegSrc=23 (r23).
__attribute__((noinline)) void subview_size12_tstore(
    MaxDst &dst, MaxSrc &s) {
  range::Subview<MaxSrc, 12, /*Off*/ 2047, /*RegSrc*/ 23> sv(s, 23);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, r23, 2047, 12
}

// The no-base factory uses the zero register.
__attribute__((noinline)) void subview_factory_zero_tstore(
    Dst &dst, Src &s) {
  auto sv = range::subview(s);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, zero, 0, 4
}

// A runtime base value uses a compiler-allocated GPR.
__attribute__((noinline)) void subview_factory_runtime_tstore(
    Dst &dst, Src &s, uintptr_t base_units) {
  auto sv = range::subview(s, base_units);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, <allocated-gpr>, 0, 4
}

__attribute__((noinline)) void subview_factory_zero_offset_tstore(
    Dst &dst, Src &s) {
  auto sv = range::subview<128, 3>(s);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, zero, 3, 1
}

__attribute__((noinline)) void subview_factory_runtime_offset_tstore(
    Dst &dst, Src &s, uintptr_t base_units) {
  auto sv = range::subview<128, 3>(s, base_units);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, <allocated-gpr>, 3, 1
}

// Explicit register selection remains a low-level ABI/testing interface.
__attribute__((noinline)) void subview_factory_explicit_reg_tstore(
    Dst &dst, Src &s) {
  auto sv = range::subview_at_reg<3, 23>(s, 23);
  TSTORE(dst, sv); // -> B.IOT <src>; B.SUBVIEW 0, r23, 3, 4
}

void use(void *) {}
int main() {
  float dst_buf[16 * 16];
  float max_dst_buf[32 * 2048];
  Src s;
  Dst d(dst_buf);
  MaxSrc max_s;
  MaxDst max_d(max_dst_buf);
  subview_source_tstore(d, s);
  subview_size12_tstore(max_d, max_s);
  subview_factory_zero_tstore(d, s);
  subview_factory_runtime_tstore(d, s, 64);
  subview_factory_zero_offset_tstore(d, s);
  subview_factory_runtime_offset_tstore(d, s, 64);
  subview_factory_explicit_reg_tstore(d, s);
  use(dst_buf);
  return 0;
}
