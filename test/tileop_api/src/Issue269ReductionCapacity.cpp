// Issue #269: reduction sources follow the general Tile allocation rules and
// do not have an operation-specific 2048-byte capacity ceiling.
#include <common/pto_tileop.hpp>

using namespace pto;

using Source = CubeTileM32<__bf16, 32, 128>;
using Destination = VecTileM32<__bf16, 32, 128, 32, 1>;

static_assert(Source::LogicalTileBytes == 8192);

__attribute__((noinline)) void reduce_source_larger_than_2kb(Source &source) {
  Destination destination;
  TROWSUM(destination, source);
  asm volatile("" : : "Tr"(destination.data()));
}
