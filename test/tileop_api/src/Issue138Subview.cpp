#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

using ReductionResult = VecTileM32<float, 32, 128, 32, 1>;
using RowTile = VecTileM32<float, 32, 1, 32, 1>;

__attribute__((noinline)) void consume_reduction_prefix(
    ReductionResult &reduction, RowTile &state, RowTile &result) {
  auto prefix = TREDUCEPREFIXVIEW<RowTile>(reduction);
  TMAX(result, state, prefix);
  TMAX(result, prefix, state);
}

int main() {
  ReductionResult reduction;
  RowTile state;
  RowTile result;
  consume_reduction_prefix(reduction, state, result);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
