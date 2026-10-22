#include <common/pto_tileop.hpp>
using namespace pto;
using SrcCell = VecTileM32<__bf16, 32, 2, 32, 1>;
using PackedWord = VecTileM32<uint32_t, 32, 1, 32, 1>;
void pack_bf16_prefix(SrcCell &a, SrcCell &b, PackedWord &out) {
  TPACK(out, a, b, 0x00000202u);
}
