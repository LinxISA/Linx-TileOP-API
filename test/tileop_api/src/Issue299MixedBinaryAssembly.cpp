#include <common/pto_tileop.hpp>

#include <cstdint>
#include <utility>

using namespace pto;

using Fragment = VecTileM32<int32_t, 32, 1>;
using Parent = VecTileM32<int32_t, 32, 8>;

static_assert(Fragment::LogicalTileBytes == 128);
static_assert(Parent::LogicalTileBytes == 1024);

__attribute__((noinline)) Parent assemble_view_tile(Parent &parent,
                                                    Fragment &rhs) {
  auto lhs = TPARTVIEW<Fragment, 1, 8>(parent)[0][7];
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

__attribute__((noinline)) Parent assemble_tile_view(Fragment &lhs,
                                                    Parent &parent) {
  auto rhs = TPARTVIEW<Fragment, 1, 8>(parent)[0][6];
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

#define INSTANTIATE_MIXED_BINARY(Name)                                         \
  do {                                                                         \
    TileArray<Fragment, 1, 1> left_slots;                                      \
    Name(left_slots[0][0], view, tile);                                        \
    Fragment left = TASSEMBLY<Fragment>(std::move(left_slots));                \
    TileArray<Fragment, 1, 1> right_slots;                                     \
    Name(right_slots[0][0], tile, view);                                       \
    Fragment right = TASSEMBLY<Fragment>(std::move(right_slots));              \
    asm volatile("" : : "Tr"(left.data()), "Tr"(right.data()));                \
  } while (false)

__attribute__((noinline)) void instantiate_all_mixed_binary(Parent &parent,
                                                            Fragment &tile) {
  auto view = TPARTVIEW<Fragment, 1, 8>(parent)[0][0];
  INSTANTIATE_MIXED_BINARY(TADD);
  INSTANTIATE_MIXED_BINARY(TSUB);
  INSTANTIATE_MIXED_BINARY(TMUL);
  INSTANTIATE_MIXED_BINARY(TDIV);
  INSTANTIATE_MIXED_BINARY(TREM);
  INSTANTIATE_MIXED_BINARY(TAND);
  INSTANTIATE_MIXED_BINARY(TOR);
  INSTANTIATE_MIXED_BINARY(TXOR);
  INSTANTIATE_MIXED_BINARY(TSHL);
  INSTANTIATE_MIXED_BINARY(TSHR);
  INSTANTIATE_MIXED_BINARY(TMAX);
  INSTANTIATE_MIXED_BINARY(TMIN);
}

#undef INSTANTIATE_MIXED_BINARY

int main() {
  Parent source_parent;
  Fragment tile;
  Parent left = assemble_view_tile(source_parent, tile);
  Parent right = assemble_tile_view(tile, source_parent);
  instantiate_all_mixed_binary(source_parent, tile);
  asm volatile("" : : "Tr"(left.data()), "Tr"(right.data()));
  return 0;
}
