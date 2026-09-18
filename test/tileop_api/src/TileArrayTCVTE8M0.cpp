#include <common/pto_tileop.hpp>

#include <utility>

using namespace pto;

// Issue #170: the recommended TileArray + TASSEMBLY example must let the real
// producer write the destination slot directly, not stage a fragment and copy
// it in with a same-dtype TCVT. This fixture is the canonical BF16 -> E8M0 MX
// scale case: each of four 128-byte CELLs is a genuine BF16 -> E8M0 conversion
// whose destination is the assembly slot, so the single TCVT bundle both
// produces the scale and carries B.ASSEMBLE. TASSEMBLY emits no instruction.
//
// Assembly destinations are RowMajor (see pto_region_tcvt_subview_assemble);
// one 128-byte E8M0 CELL is [32,4] (physical) with a [32,1] valid region, and
// the matching BF16 source declares the same physical [32,4] shape.
using ScaleBf16 = Tile<Location::Vec, __bf16, 32, 4, BLayout::RowMajor, 32, 1>;
using ScaleE8M0Fragment =
    Tile<Location::Vec, __fp8_e8m0, 32, 4, BLayout::RowMajor, 32, 1>;
using ScaleE8M0Parent =
    Tile<Location::Vec, __fp8_e8m0, 32, 16, BLayout::RowMajor, 32, 4>;

__attribute__((noinline)) ScaleE8M0Parent
assemble_four_scales(ScaleBf16 (&scales)[4]) {
  TileArray<ScaleE8M0Fragment, 1, 4> destinations;

  for (int col = 0; col < 4; ++col) {
    auto slot = destinations[0][col]; // TileArrayOutputRef, destination only.
    // Algorithmic BF16 -> E8M0 scale quantization written straight to the slot.
    // LINX_RDN lowers to B.DATR ..., RTM (the MX floor rounding, OCP MX 6.3).
    TCVT<LINX_RDN>(slot, scales[col]);
  }

  return TASSEMBLY<ScaleE8M0Parent>(std::move(destinations));
}

int main() {
  ScaleBf16 scales[4];
  ScaleE8M0Parent result = assemble_four_scales(scales);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
