#ifndef PTO_JCORE_SCALAR_CVT_HPP
#define PTO_JCORE_SCALAR_CVT_HPP

#include <cstdint>
#include "common/pto_rounding.hpp"

enum LinxSat { LINX_NOSAT = 0, LINX_SAT = 1 };

template <class...> struct linx_cvt_false { enum { value = 0 }; };
template <int RMode> struct linx_valid_rmode {
  enum { value = RMode >= pto::LINX_RNONE && RMode <= pto::LINX_RHB };
};
template <int Sat> struct linx_valid_sat {
  enum { value = Sat == LINX_NOSAT || Sat == LINX_SAT };
};

namespace pto::detail {

// FCVTZ supplies the result and numeric flags. Only fractional FP32 values
// need correction for RTO/RHB; every FP32 with |x| >= 2^23 is integral.
// All arithmetic on the integer encoding is unsigned (including wrap).
template <int RMode, int Sat>
constexpr uint32_t scalar_cvt_f32_s32_finish(uint32_t bits, uint32_t result) {
  const uint32_t magnitude = bits & 0x7fffffffu;
  const bool negative = (bits >> 31) != 0;
  if (magnitude > 0x7f800000u) return 0; // NaN: zero, NV from FCVT.
  if (magnitude == 0x7f800000u)
    return negative ? 0x80000000u : 0x7fffffffu;
  if (magnitude >= 0x4f000000u) {
    if constexpr (Sat == LINX_SAT)
      return negative ? 0x80000000u : 0x7fffffffu;
    // Common-conversion finite overflow wraps to the destination width.
    // FP32 values this large have no fractional bits. Do not rely on a
    // host float-to-int cast or an implementation's overflow sentinel.
    const unsigned shift = (magnitude >> 23) - 150;
    const uint32_t significand = (magnitude & 0x7fffffu) | 0x800000u;
    const uint32_t wrapped = shift < 32 ? significand << shift : 0;
    return negative ? uint32_t(0) - wrapped : wrapped;
  }
  if constexpr (RMode == LINX_RTO || RMode == LINX_RHB) {
    const unsigned exponent = magnitude >> 23;
    if (exponent < 150 && magnitude != 0) {
      const uint32_t significand = (magnitude & 0x7fffffu) | 0x800000u;
      const unsigned shift = exponent < 127 ? 24 : 150 - exponent;
      const uint32_t remainder = significand & ((uint32_t(1) << shift) - 1);
      bool increment;
      if constexpr (RMode == LINX_RTO) {
        increment = remainder != 0 && (result & 1) == 0;
      } else {
        // RHB ties go towards +infinity, including negative inputs. Below
        // 0.5 even a maximal significand cannot round away from zero.
        const uint32_t half = uint32_t(1) << (shift - 1);
        increment = exponent >= 126 &&
                    (negative ? remainder > half : remainder >= half);
      }
      if (increment) result += negative ? uint32_t(-1) : uint32_t(1);
    }
  }
  return result;
}

template <int RMode, int Sat>
__attribute__((always_inline)) inline int32_t scalar_cvt_f32_s32(float src) {
  static_assert(linx_valid_rmode<RMode>::value, "invalid cvt RMode");
  static_assert(linx_valid_sat<Sat>::value, "invalid cvt Sat");
  uint32_t bits;
  __builtin_memcpy(&bits, &src, sizeof(bits));
  const uint64_t input = bits;
  uint64_t output;
  // Use XLEN carriers, not i32 CopyFromReg or SIMT vr constraints. Volatile
  // preserves the conversion's sticky numeric-status effects even if the
  // C++ caller discards the result.
  if constexpr (RMode == LINX_RNONE || RMode == LINX_RNE)
    asm volatile("BSTART.STD FALL\nfcvtn.fs2sw %1, ->%0\nBSTOP\n"
                 : "=r"(output) : "r"(input));
  else if constexpr (RMode == LINX_RDN)
    asm volatile("BSTART.STD FALL\nfcvtm.fs2sw %1, ->%0\nBSTOP\n"
                 : "=r"(output) : "r"(input));
  else if constexpr (RMode == LINX_RUP)
    asm volatile("BSTART.STD FALL\nfcvtp.fs2sw %1, ->%0\nBSTOP\n"
                 : "=r"(output) : "r"(input));
  else if constexpr (RMode == LINX_RNA)
    asm volatile("BSTART.STD FALL\nfcvta.fs2sw %1, ->%0\nBSTOP\n"
                 : "=r"(output) : "r"(input));
  else
    asm volatile("BSTART.STD FALL\nfcvtz.fs2sw %1, ->%0\nBSTOP\n"
                 : "=r"(output) : "r"(input));
  const uint32_t encoded = scalar_cvt_f32_s32_finish<RMode, Sat>(bits, output);
  int32_t result;
  __builtin_memcpy(&result, &encoded, sizeof(result));
  return result;
}

} // namespace pto::detail
#endif
