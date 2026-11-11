#include <common/pto_tileop.hpp>

using namespace pto;

using M32 = VecTileM32<uint8_t, 32, 128>;
using ND32 = SharedTile<Tile<Location::Vec, uint8_t, 32, 128>>;
using Parent32 = SharedTile<Tile<Location::Vec, uint8_t, 128, 128>>;
using M16 = CubeTileM16<uint16_t, 16, 64>;
using ND16 = SharedTile<Tile<Location::Left, uint16_t, 16, 64>>;
using Parent16 = SharedTile<Tile<Location::Left, uint16_t, 64, 64>>;

// Every Shared handle is produced and consumed in one inline SSA lifetime.
// Distinct Local inputs may contain any payload; conversion is bitwise.
extern "C" void shared_nd_singleton_m32(M32 &dst, M32 &input) {
  ND32 shared;
  TMOV_CUBE_TO_SHARED_ND<8>(shared, input);
  TMOV_SHARED_ND_TO_CUBE<8>(dst, shared);
}

extern "C" void shared_nd_singleton_m16(M16 &dst, M16 &input) {
  ND16 shared;
  TMOV_CUBE_TO_SHARED_ND<8>(shared, input);
  TMOV_SHARED_ND_TO_CUBE<8>(dst, shared);
}

extern "C" void shared_nd_four_pe_m32(M32 &dst, M32 &input,
                                       uintptr_t source_offset_units) {
  Parent32 shared;
  auto first = range::assemble<32, 0>(shared);
  TMOV_CUBE_TO_SHARED_ND(first, input);
  auto second = range::assemble_middle<32, 32>(shared);
  TMOV_CUBE_TO_SHARED_ND(second, input);
  auto third = range::assemble_middle<32, 64>(shared);
  TMOV_CUBE_TO_SHARED_ND(third, input);
  auto last = range::assemble_last<32, 96>(shared);
  TMOV_CUBE_TO_SHARED_ND(last, input);
  auto view = range::shared_nd_subview<32>(shared, source_offset_units);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
}

extern "C" void shared_nd_four_pe_m16(M16 &dst, M16 &input,
                                       uintptr_t source_offset_units) {
  Parent16 shared;
  auto first = range::assemble<16, 0>(shared);
  TMOV_CUBE_TO_SHARED_ND(first, input);
  auto second = range::assemble_middle<16, 16>(shared);
  TMOV_CUBE_TO_SHARED_ND(second, input);
  auto third = range::assemble_middle<16, 32>(shared);
  TMOV_CUBE_TO_SHARED_ND(third, input);
  auto last = range::assemble_last<16, 48>(shared);
  TMOV_CUBE_TO_SHARED_ND(last, input);
  auto view = range::shared_nd_subview<16>(shared, source_offset_units);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
}

using TailInput = VecTileM32<uint32_t, 32, 4, 19, 3>;
using TailND = SharedTile<Tile<Location::Vec, uint32_t, 32, 4,
                                 BLayout::RowMajor, DYNAMIC, DYNAMIC>>;
using TailOutput = VecTileM32<uint32_t, 32, 4, DYNAMIC, DYNAMIC>;
extern "C" void shared_nd_dynamic_tail(TailOutput &dst, TailInput &input) {
  TailND shared;
  TMOV_CUBE_TO_SHARED_ND<8>(shared, input);
  auto view = range::shared_nd_subview<4>(shared);
  TMOV_SHARED_ND_TO_CUBE<8>(dst, view);
}

using Packed = VecTileM16<__uint4x2, 16, 32, 11, 17>;
using PackedND = SharedTile<Tile<Location::Vec, __uint4x2, 16, 32,
                                   BLayout::RowMajor, 11, 17>>;
extern "C" void shared_nd_packed_tail(Packed &dst, Packed &input) {
  PackedND shared;
  TMOV_CUBE_TO_SHARED_ND<8>(shared, input);
  TMOV_SHARED_ND_TO_CUBE<8>(dst, shared);
}

using B64 = VecTileM32<uint64_t, 32, 1>;
using B64ND = SharedTile<Tile<Location::Vec, uint64_t, 32, 1>>;
extern "C" void shared_nd_b64(B64 &dst, B64 &input) {
  B64ND shared;
  TMOV_CUBE_TO_SHARED_ND<8>(shared, input);
  TMOV_SHARED_ND_TO_CUBE<8>(dst, shared);
}

extern "C" void shared_nd_static_offset(M32 &dst, M32 &input) {
  Parent32 shared;
  auto first = range::assemble<32, 0>(shared);
  TMOV_CUBE_TO_SHARED_ND(first, input);
  auto second = range::assemble_middle<32, 32>(shared);
  TMOV_CUBE_TO_SHARED_ND(second, input);
  auto third = range::assemble_middle<32, 64>(shared);
  TMOV_CUBE_TO_SHARED_ND(third, input);
  auto last = range::assemble_last<32, 96>(shared);
  TMOV_CUBE_TO_SHARED_ND(last, input);
  auto view = range::shared_nd_subview<32, 64>(shared);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
}

extern "C" void shared_nd_zero(M32 &dst) {
  // No source allocation, handle read, GPR offset check or metadata update.
  Parent32 unavailable;
  auto view = range::shared_nd_subview<32>(unavailable, ~uintptr_t{0});
  TMOV_SHARED_ND_TO_CUBE<0>(dst, view);
}
