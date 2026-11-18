#ifndef PTO_PACKED_BF16_HPP
#define PTO_PACKED_BF16_HPP

#include "common/pto_packed_bits.hpp"

namespace pto::detail {

// Software packed scalar profile, independent of FSU/Tile instructions and FRM.
// Finite arithmetic uses exact integers in units of 2^-266: the smallest
// BF16 product is 2^-266 and the largest product occupies fewer than 523 bits.
// Nine limbs also hold the numerator and midpoint comparisons used below.
struct bf16_integer {
  uint64_t limb[9]{};
};

inline bf16_integer bf16_integer_at(uint32_t significand, unsigned shift) {
  bf16_integer x{};
  const unsigned word = shift / 64, bit = shift % 64;
  x.limb[word] = uint64_t(significand) << bit;
  if (bit && word + 1 < 9)
    x.limb[word + 1] = uint64_t(significand) >> (64 - bit);
  return x;
}

inline int bf16_compare(const bf16_integer &a, const bf16_integer &b) {
  for (int i = 8; i >= 0; --i) {
    if (a.limb[i] != b.limb[i]) return a.limb[i] > b.limb[i] ? 1 : -1;
  }
  return 0;
}

inline bf16_integer bf16_sum(const bf16_integer &a, const bf16_integer &b) {
  bf16_integer x{};
  uint64_t carry = 0;
  for (unsigned i = 0; i < 9; ++i) {
    const uint64_t t = a.limb[i] + b.limb[i];
    const uint64_t u = t + carry;
    carry = (t < a.limb[i]) | (u < t);
    x.limb[i] = u;
  }
  return x;
}

// Requires a >= b. No signed overflow or shifts by a word width.
inline bf16_integer bf16_difference(const bf16_integer &a,
                                    const bf16_integer &b) {
  bf16_integer x{};
  uint64_t borrow = 0;
  for (unsigned i = 0; i < 9; ++i) {
    const uint64_t t = a.limb[i] - b.limb[i];
    x.limb[i] = t - borrow;
    borrow = (a.limb[i] < b.limb[i]) | (t < borrow);
  }
  return x;
}

inline bf16_integer bf16_twice(const bf16_integer &a) {
  bf16_integer x{};
  uint64_t carry = 0;
  for (unsigned i = 0; i < 9; ++i) {
    x.limb[i] = (a.limb[i] << 1) | carry;
    carry = a.limb[i] >> 63;
  }
  return x;
}

inline unsigned bf16_significand(uint16_t x) {
  return (x & 127) | ((x & 0x7f80) ? 128 : 0);
}
inline int bf16_exponent(uint16_t x) {
  const int exponent = (x >> 7) & 255;
  return exponent ? exponent - 134 : -133;
}
inline bool bf16_nan(uint16_t x) { return (x & 0x7fff) > 0x7f80; }
inline bool bf16_inf(uint16_t x) { return (x & 0x7fff) == 0x7f80; }
inline bool bf16_zero(uint16_t x) { return (x & 0x7fff) == 0; }
inline bf16_integer bf16_value(uint16_t x) {
  return bf16_integer_at(bf16_significand(x), unsigned(bf16_exponent(x) + 266));
}
inline bf16_integer bf16_product(uint16_t a, uint16_t b) {
  return bf16_integer_at(bf16_significand(a) * bf16_significand(b),
                        unsigned(bf16_exponent(a) + bf16_exponent(b) + 266));
}

template <int Mode, int Sat>
inline void bf16_controls() {
  static_assert(Mode >= 0 && Mode <= 7, "invalid bf16 RMode");
  static_assert(Mode != 7, "Janus packed BF16 RHB is unsupported");
  static_assert(Sat == 0 || Sat == 1, "invalid bf16 Sat");
  static_assert(Sat == 0, "Janus packed BF16 saturation is unsupported");
}

// cmp_half: discarded fraction compared with 1/2, exact: no discarded bits.
// Modes use LinxRMode/BlkvBf16RMode ordinals; RNONE resolves to RNE here.
template <int Mode>
inline bool bf16_increment(unsigned lower, bool negative, bool exact,
                           int cmp_half) {
  if (exact) return false;
  if constexpr (Mode == 2 || Mode == 6) return false;
  if constexpr (Mode == 3) return negative;
  if constexpr (Mode == 4) return !negative;
  if constexpr (Mode == 5) return cmp_half >= 0;
  return cmp_half > 0 || (cmp_half == 0 && (lower & 1));
}

template <int Mode>
inline uint16_t bf16_overflow(bool negative) {
  const bool finite = Mode == 2 || Mode == 6 ||
                      (Mode == 3 && !negative) || (Mode == 4 && negative);
  return uint16_t((negative ? 0x8000 : 0) | (finite ? 0x7f7f : 0x7f80));
}

inline bool bf16_bit(const bf16_integer &x, unsigned bit) {
  return (x.limb[bit / 64] >> (bit % 64)) & 1;
}
inline bool bf16_any_below(const bf16_integer &x, unsigned bit) {
  for (unsigned i = 0; i < bit / 64; ++i)
    if (x.limb[i]) return true;
  return (bit % 64) && (x.limb[bit / 64] & ((uint64_t(1) << (bit % 64)) - 1));
}

template <int Mode>
inline uint16_t bf16_round_integer(const bf16_integer &x, bool negative) {
  const uint16_t sign = negative ? 0x8000 : 0;
  int high = 575;
  while (high >= 0 && !bf16_bit(x, unsigned(high))) --high;
  if (high < 0) return sign;
  const bool normal = high >= 140;
  const unsigned shift = normal ? unsigned(high - 7) : 133;
  unsigned lower = 0;
  for (unsigned i = 0; i < 8; ++i)
    if (bf16_bit(x, shift + i)) lower |= 1u << i;
  const bool exact = !bf16_any_below(x, shift);
  const int half = !bf16_bit(x, shift - 1) ? -1 :
                    (bf16_any_below(x, shift - 1) ? 1 : 0);
  if constexpr (Mode == 6) { if (!exact) lower |= 1; }
  else if (bf16_increment<Mode>(lower, negative, exact, half)) ++lower;
  const int exponent = normal ? high - 139 : 0;
  if (exponent >= 255) return bf16_overflow<Mode>(negative);
  const unsigned encoding = normal ? unsigned(exponent * 128) + lower - 128 : lower;
  if (encoding >= 0x7f80) return bf16_overflow<Mode>(negative);
  return uint16_t(sign | encoding);
}

// Exact finite magnitude accumulation, rounded once at the BF16 boundary.
template <int Mode>
inline uint16_t bf16_accumulate(const bf16_integer &a, bool aneg,
                                const bf16_integer &b, bool bneg) {
  if (aneg == bneg) return bf16_round_integer<Mode>(bf16_sum(a, b), aneg);
  const int compare = bf16_compare(a, b);
  if (!compare) return Mode == 3 ? 0x8000 : 0;
  return compare > 0 ? bf16_round_integer<Mode>(bf16_difference(a, b), aneg) :
                       bf16_round_integer<Mode>(bf16_difference(b, a), bneg);
}

template <int Mode>
inline uint16_t bf16_from_float(float value) {
  uint32_t bits;
  __builtin_memcpy(&bits, &value, 4);
  // Canonical quiet NaN, including values whose payload is only in low bits.
  if ((bits & 0x7fffffff) > 0x7f800000) return 0x7fc0;
  unsigned lower = bits >> 16;
  const unsigned tail = bits & 0xffff;
  if constexpr (Mode == 6) { if (tail) lower |= 1; }
  else if (bf16_increment<Mode>(lower, bits >> 31, tail == 0,
                                tail < 0x8000 ? -1 : tail > 0x8000 ? 1 : 0))
    ++lower;
  return uint16_t(lower);
}

enum class packed_bf16_op { fadd, fsub, fmul, fdiv, fsqrt, fexp, fmadd, fmsub };

template <int Mode>
inline uint16_t bf16_add(uint16_t a, uint16_t b) {
  if (bf16_nan(a) || bf16_nan(b)) return 0x7fc0;
  if (bf16_inf(a) || bf16_inf(b)) {
    if (bf16_inf(a) && bf16_inf(b) && ((a ^ b) & 0x8000)) return 0x7fc0;
    return bf16_inf(a) ? a : b;
  }
  return bf16_accumulate<Mode>(bf16_value(a), a >> 15, bf16_value(b), b >> 15);
}

template <int Mode>
inline uint16_t bf16_multiply(uint16_t a, uint16_t b) {
  const bool negative = ((a ^ b) >> 15) != 0;
  if (bf16_nan(a) || bf16_nan(b) ||
      (bf16_inf(a) && bf16_zero(b)) || (bf16_inf(b) && bf16_zero(a))) return 0x7fc0;
  if (bf16_inf(a) || bf16_inf(b)) return uint16_t((negative ? 0x8000 : 0) | 0x7f80);
  return bf16_round_integer<Mode>(bf16_product(a, b), negative);
}

template <int Mode>
inline uint16_t bf16_fused(uint16_t a, uint16_t b, uint16_t c) {
  if (bf16_nan(a) || bf16_nan(b) || bf16_nan(c) ||
      (bf16_inf(a) && bf16_zero(b)) || (bf16_inf(b) && bf16_zero(a))) return 0x7fc0;
  const bool negative = ((a ^ b) >> 15) != 0;
  if (bf16_inf(a) || bf16_inf(b)) {
    if (bf16_inf(c) && negative != bool(c >> 15)) return 0x7fc0;
    return uint16_t((negative ? 0x8000 : 0) | 0x7f80);
  }
  if (bf16_inf(c)) return c;
  return bf16_accumulate<Mode>(bf16_product(a, b), negative, bf16_value(c), c >> 15);
}

// Select between adjacent finite encodings from exact magnitude comparisons.
// The virtual successor of max-finite is 2^128 (not an infinity operand).
template <int Mode, class Compare>
inline uint16_t bf16_search(bool negative, Compare compare) {
  unsigned lo = 0, hi = 0x7f80;
  while (lo + 1 < hi) {
    const unsigned mid = (lo + hi) / 2;
    if (compare(uint16_t(mid), false) <= 0) lo = mid;
    else hi = mid;
  }
  const int exact = compare(uint16_t(lo), false);
  unsigned result = lo;
  if (exact) {
    if constexpr (Mode == 6) result |= 1;
    else {
      // compare(midpoint, target) has the opposite sense of cmp_half.
      const int midpoint = compare(uint16_t(lo), true);
      if (bf16_increment<Mode>(lo, negative, false, -midpoint)) result = hi;
    }
  }
  return uint16_t((negative ? 0x8000 : 0) | result);
}

template <int Mode>
inline uint16_t bf16_divide(uint16_t a, uint16_t b) {
  const uint16_t sign = (a ^ b) & 0x8000;
  if (bf16_nan(a) || bf16_nan(b) || (bf16_zero(a) && bf16_zero(b)) ||
      (bf16_inf(a) && bf16_inf(b))) return 0x7fc0;
  if (bf16_inf(a) || bf16_zero(b)) return sign | 0x7f80;
  if (bf16_inf(b) || bf16_zero(a)) return sign;
  const unsigned denominator = bf16_significand(b);
  const auto numerator = bf16_integer_at(bf16_significand(a),
      unsigned(bf16_exponent(a) - bf16_exponent(b) + 266));
  return bf16_search<Mode>(sign != 0, [&](uint16_t lo, bool midpoint) {
    auto lower = bf16_integer_at(bf16_significand(lo) * denominator,
                                unsigned(bf16_exponent(lo) + 266));
    if (!midpoint) return bf16_compare(lower, numerator);
    const uint16_t hi = lo + 1;
    auto upper = bf16_integer_at(bf16_significand(hi) * denominator,
                                unsigned(bf16_exponent(hi) + 266));
    return bf16_compare(bf16_sum(lower, upper), bf16_twice(numerator));
  });
}

template <int Mode>
inline uint16_t bf16_sqrt(uint16_t x) {
  if (bf16_nan(x)) return 0x7fc0;
  if (bf16_zero(x)) return x;
  if (x & 0x8000) return 0x7fc0;
  if (bf16_inf(x)) return x;
  const auto value = bf16_value(x);
  return bf16_search<Mode>(false, [&](uint16_t lo, bool midpoint) {
    if (!midpoint) return bf16_compare(bf16_product(lo, lo), value);
    const uint16_t hi = lo + 1;
    const int exponent = bf16_exponent(lo);
    const unsigned sum = bf16_significand(lo) +
        (bf16_significand(hi) << (bf16_exponent(hi) - exponent));
    return bf16_compare(bf16_integer_at(sum * sum, unsigned(2 * exponent + 266)),
                        bf16_twice(bf16_twice(value)));
  });
}

inline uint16_t bf16_maximum(uint16_t a, uint16_t b) {
  if (bf16_nan(a) || bf16_nan(b)) return 0x7fc0;
  if (bf16_zero(a) && bf16_zero(b)) return uint16_t(a & b); // +0 wins
  if ((a ^ b) & 0x8000) return a & 0x8000 ? b : a;
  return a & 0x8000 ? (a < b ? a : b) : (a > b ? a : b);
}

template <packed_bf16_op Op, int Mode, int Sat>
inline __bf16x2 packed_bf16_binary(const __bf16x2 &a, const __bf16x2 &b) {
  bf16_controls<Mode, Sat>();
  const uint32_t aw = bf16x2_to_bits(a), bw = bf16x2_to_bits(b);
  uint16_t lane[2];
  for (unsigned i = 0; i < 2; ++i) {
    const uint16_t x = uint16_t(aw >> (16 * i)), y = uint16_t(bw >> (16 * i));
    if constexpr (Op == packed_bf16_op::fadd) lane[i] = bf16_add<Mode>(x, y);
    if constexpr (Op == packed_bf16_op::fsub) lane[i] = bf16_add<Mode>(x, y ^ 0x8000);
    if constexpr (Op == packed_bf16_op::fmul) lane[i] = bf16_multiply<Mode>(x, y);
    if constexpr (Op == packed_bf16_op::fdiv) lane[i] = bf16_divide<Mode>(x, y);
  }
  return bf16x2_pack_bits(lane[0], lane[1]);
}

template <packed_bf16_op Op, int Mode, int Sat>
inline __bf16x2 packed_bf16_unary(const __bf16x2 &x) {
  bf16_controls<Mode, Sat>();
  static_assert(Op == packed_bf16_op::fsqrt, "Janus packed BF16 exp is unsupported");
  if constexpr (Op == packed_bf16_op::fsqrt)
    return bf16x2_pack_bits(bf16_sqrt<Mode>(bf16x2_lane_bits<0>(x)),
                            bf16_sqrt<Mode>(bf16x2_lane_bits<1>(x)));
  else return {};
}

template <packed_bf16_op Op, int Mode, int Sat>
inline __bf16x2 packed_bf16_ternary(const __bf16x2 &a, const __bf16x2 &b,
                                  const __bf16x2 &c) {
  bf16_controls<Mode, Sat>();
  const uint32_t aw = bf16x2_to_bits(a), bw = bf16x2_to_bits(b), cw = bf16x2_to_bits(c);
  uint16_t lane[2];
  for (unsigned i = 0; i < 2; ++i) {
    const uint16_t z = uint16_t(cw >> (16 * i));
    lane[i] = bf16_fused<Mode>(uint16_t(aw >> (16 * i)), uint16_t(bw >> (16 * i)),
                              Op == packed_bf16_op::fmsub ? z ^ 0x8000 : z);
  }
  return bf16x2_pack_bits(lane[0], lane[1]);
}

inline __bf16x2 packed_bf16_max(const __bf16x2 &a, const __bf16x2 &b) {
  return bf16x2_pack_bits(bf16_maximum(bf16x2_lane_bits<0>(a), bf16x2_lane_bits<0>(b)),
                          bf16_maximum(bf16x2_lane_bits<1>(a), bf16x2_lane_bits<1>(b)));
}
} // namespace pto::detail

namespace pto {
// Numeric conversion is deliberately distinct from raw encoding helpers.
template <int RMode = 0, int Sat = 0>
inline __bf16x2 bf16x2_from_float(float lane0, float lane1) {
  detail::bf16_controls<RMode, Sat>();
  return bf16x2_pack_bits(detail::bf16_from_float<RMode>(lane0),
                          detail::bf16_from_float<RMode>(lane1));
}

template <unsigned Lane>
inline float bf16x2_to_float(const __bf16x2 &value) {
  const uint32_t bits = uint32_t(bf16x2_lane_bits<Lane>(value)) << 16;
  float result;
  __builtin_memcpy(&result, &bits, 4);
  return result;
}

namespace detail {
template <int Mode, int Sat, class Dst, class Src>
inline void packed_bf16_convert(Dst &dst, const Src &a, const Src &b) {
  dst = bf16x2_from_float<Mode, Sat>(a, b);
}
} // namespace detail
} // namespace pto
#endif
