#ifndef PTO_PACKED_BITS_HPP
#define PTO_PACKED_BITS_HPP

#include <stdint.h>

// -mlxbc supplies the target storage types before TileOP headers. Other
// consumers may use the compatibility types or provide their own type shim.
#if !defined(__bf16x2_TYPE) && !defined(PTO_LINX_COMPAT_TYPES_PROVIDED)
#include "jcore/linx_compat_types.hpp"
#endif

namespace pto {
namespace detail {

inline auto &bf16x2_storage(__bf16x2 &value) {
#ifdef __bf16x2_STORAGE
  return __bf16x2_STORAGE(value);
#else
  return value.data;
#endif
}

inline const auto &bf16x2_storage(const __bf16x2 &value) {
#ifdef __bf16x2_STORAGE
  return __bf16x2_STORAGE(value);
#else
  return value.data;
#endif
}

} // namespace detail

// These helpers move raw encodings; they do not convert floating-point values.
// The compiler's __bf16x2 carrier is float, while compatibility carriers may be
// uint32_t. Byte copies preserve all bits in either representation, including
// NaN payloads, without floating-point arithmetic or aliasing through a cast.
inline uint32_t bf16x2_to_bits(const __bf16x2 &value) {
  static_assert(sizeof(detail::bf16x2_storage(value)) == sizeof(uint32_t),
                "BF16x2 storage must occupy 32 bits");
  uint32_t bits;
  __builtin_memcpy(&bits, &detail::bf16x2_storage(value), sizeof(bits));
  return bits;
}

inline __bf16x2 bf16x2_from_bits(uint32_t bits) {
  __bf16x2 value{};
  static_assert(sizeof(detail::bf16x2_storage(value)) == sizeof(bits),
                "BF16x2 storage must occupy 32 bits");
  __builtin_memcpy(&detail::bf16x2_storage(value), &bits, sizeof(bits));
  return value;
}

// Lane numbering describes the encoded word, independently of host byte order.
// Lane 0 occupies bits[15:0], and lane 1 occupies bits[31:16].
inline __bf16x2 bf16x2_pack_bits(uint16_t lane0, uint16_t lane1) {
  return bf16x2_from_bits(static_cast<uint32_t>(lane0) |
                          (static_cast<uint32_t>(lane1) << 16));
}

template <unsigned Lane>
inline uint16_t bf16x2_lane_bits(const __bf16x2 &value) {
  static_assert(Lane < 2, "BF16x2 lane must be 0 or 1");
  // Keep an invalid instantiation free of an out-of-range shift diagnostic.
  return static_cast<uint16_t>(bf16x2_to_bits(value) >> ((Lane & 1) * 16));
}

} // namespace pto

#endif // PTO_PACKED_BITS_HPP
