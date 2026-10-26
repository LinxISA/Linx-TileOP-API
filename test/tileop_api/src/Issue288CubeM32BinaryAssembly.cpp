#include <common/pto_tileop.hpp>

#include <cstdint>
#include <utility>

using namespace pto;

using Fragment = VecTileM32<int32_t, 32, 1>;
using Parent = VecTileM32<int32_t, 32, 8>;

static_assert(Fragment::LogicalTileBytes == 128);
static_assert(Parent::LogicalTileBytes == 1024);

__attribute__((noinline)) Parent assemble_binary_cube_m32(Fragment &lhs,
                                                          Fragment &rhs) {
  TileArray<Fragment, 1, 8> slots;
  TADD(slots[0][0], lhs, rhs);
  TADD(slots[0][1], lhs, rhs);
  TADD(slots[0][2], lhs, rhs);
  TADD(slots[0][3], lhs, rhs);
  TADD(slots[0][4], lhs, rhs);
  TADD(slots[0][5], lhs, rhs);
  TADD(slots[0][6], lhs, rhs);
  TADD(slots[0][7], lhs, rhs);
  return TASSEMBLY<Parent>(std::move(slots));
}

int main() {
  Fragment lhs;
  Fragment rhs;
  Parent result = assemble_binary_cube_m32(lhs, rhs);
  asm volatile("" : : "Tr"(result.data()));
  return 0;
}
