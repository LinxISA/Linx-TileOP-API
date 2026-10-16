#include <common/pto_tileop.hpp>
using namespace pto;
using U = VecTileM32<uint16_t, 32, 8>;
using B = VecTileM32<__bf16, 32, 8>;
void mul_reinterpreted(U &packed, B &scale, B &dst) {
  auto bf = reinterpret_tile<__bf16>(packed);
  TMUL(dst, bf, scale);
}
