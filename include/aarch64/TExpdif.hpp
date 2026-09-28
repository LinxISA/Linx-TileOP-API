#ifndef AARCH64_TEXPDIF_HPP
#define AARCH64_TEXPDIF_HPP

#include "common/pto_tile.hpp"

// PTO 0.58.7 TEXPDIF has no AArch64/SME lowering in this repository.  Keep a
// named entry point so the API fails explicitly instead of silently replacing
// the direct operation with a TEXP(TSUB(...)) sequence.
template <typename D, typename A, typename B>
void TEXPDIF_Impl(D &, A &, B &) {
  static_assert(!std::is_same_v<D, D>,
                "TEXPDIF is not supported by the AArch64/SME backend");
}

#endif