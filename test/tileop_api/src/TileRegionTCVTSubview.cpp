#include <common/pto_tileop.hpp>
#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

// The destination has a different physical column capacity because BF16 and
// FP32 use different bytes per element.  The logical valid shape and CUBE
// layout are preserved by TCVT.
using SourceParent = CubeTileM32<float, 32, 64>;
using SourceFragment = CubeTileM32<float, 32, 16>;
using Destination = CubeTileM32<__bf16, 32, 16>;

__attribute__((noinline)) void convert_subview(SourceParent &parent,
                                               Destination &destination) {
  auto views = TPARTVIEW<SourceFragment, 1, 4>(parent);
  auto source = views[0][0];
  TCVT(destination, source);
}

int main() {
  SourceParent parent;
  Destination destination;
  convert_subview(parent, destination);
  asm volatile("" : : "Tr"(destination.data()));
  return 0;
}