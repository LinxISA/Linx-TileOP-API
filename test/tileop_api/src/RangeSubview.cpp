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
//   - uimm11 offset must be 0..2047 (wrapper-compile-time).
//   - source roles only map to B.SUBVIEW (never B.ASSEMBLE).

#include <common/pto_tileop.hpp>

using namespace pto;

using Src = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GMSrc = global_tensor<float, RowMajor<4, 8>>;
using GMDst = global_tensor<float, RowMajor<4, 8>>;

// Local source subview: store the subviewed source to GM.
__attribute__((noinline)) void subview_source_tstore(
    GMDst &dst, Src &s) {
  range::Subview<Src, 1, /*Off*/ 0, /*RegSrc*/ 0> sv(s, 0);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, r0, 0, 1
}

// SubviewSizeCode 12 boundary, max uimm11 offset, RegSrc=23 (r23).
__attribute__((noinline)) void subview_size12_tstore(
    GMDst &dst, Src &s) {
  range::Subview<Src, 12, /*Off*/ 2047, /*RegSrc*/ 23> sv(s, 23);
  TSTORE(dst, sv); // -> B.SUBVIEW 0, r23, 2047, 12
}

// The factory derives the common size-code from Src and hides the carrier
// type. Explicit size/offset/register settings remain available when needed.
__attribute__((noinline)) void subview_factory_tstore(
    GMDst &dst, Src &s) {
  auto sv = range::subview(s, 0);
  TSTORE(dst, sv);
}

__attribute__((noinline)) void subview_factory_custom_tstore(
    GMDst &dst, Src &s) {
  auto sv = range::subview_at<2047, 23>(s, 23);
  TSTORE(dst, sv);
}

void use(void *) {}
int main() {
  float src_buf[4 * 8];
  float dst_buf[4 * 8];
  GMSrc gs(src_buf);
  GMDst gd(dst_buf);
  Src s;
  subview_source_tstore(gd, s);
  subview_size12_tstore(gd, s);
  subview_factory_tstore(gd, s);
  subview_factory_custom_tstore(gd, s);
  use(src_buf);
  return 0;
}
