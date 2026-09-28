#include <common/pto_tileop.hpp>

using namespace pto;

using F16 = Tile<Location::Vec, __half, 16, 16>;
using BF16 = Tile<Location::Vec, __bf16, 16, 16>;
using F32 = Tile<Location::Vec, float, 16, 16>;
using M16F16 = VecTileM16<__half, 16, 16>;
using M32F32 = VecTileM32<float, 32, 32>;

void texpdif_fp16(F16 &d, F16 &a, F16 &b) { TEXPDIF(d, a, b); }
void texpdif_bf16(BF16 &d, BF16 &a, BF16 &b) { TEXPDIF(d, a, b); }
void texpdif_fp32(F32 &d, F32 &a, F32 &b) { TEXPDIF(d, a, b); }
void texpdif_f16_to_f32(F32 &d, F16 &a, F16 &b) { TEXPDIF(d, a, b); }
void texpdif_bf16_to_f32(F32 &d, BF16 &a, BF16 &b) { TEXPDIF(d, a, b); }
void texpdif_cube_m16(M16F16 &d, M16F16 &a, M16F16 &b) { TEXPDIF(d, a, b); }
void texpdif_cube(M32F32 &d, M32F32 &a, M32F32 &b) { TEXPDIF(d, a, b); }
void texpdif_carriers(F16 &d, Tile<Location::Vec, uint16_t, 16, 16> &a,
                      Tile<Location::Vec, int16_t, 16, 16> &b) {
  auto av = reinterpret_tile<__half>(a);
  auto bv = reinterpret_tile<__half>(b);
  TEXPDIF(d, av, bv);
}

int main() { return 0; }