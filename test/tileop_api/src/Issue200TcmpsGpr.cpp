#include <common/pto_tileop.hpp>

using InputTile = Tile<Location::Vec, int32_t, 32, 2, BLayout::CubeM32>;
using U8Tile = Tile<Location::Vec, unsigned char, 32, 4, BLayout::CubeM32>;

uint64_t issue_200_tcmps_gpr(InputTile &src, int32_t scalar) {
  return TCMPS<CmpMode::GE>(src, scalar);
}

uint64_t issue_200_tcmps_gpr_u8_high(U8Tile &src, unsigned char scalar) {
  return TCMPS<CmpMode::LT, true>(src, scalar);
}
