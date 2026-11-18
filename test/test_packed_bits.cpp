// Exercise the raw word API against both production carrier representations.
// Define PTO_TEST_FLOAT_BF16X2 to model the compiler's float storage carrier.
#ifdef PTO_TEST_FLOAT_BF16X2
#define PTO_LINX_COMPAT_TYPES_PROVIDED 1
struct __bf16x2 { float data; };
#define __bf16x2_TYPE "bf16x2"
#define __bf16x2_STORAGE(value) ((value).data)
#endif

#include "common/pto_packed_bits.hpp"

#include <cassert>
#include <cstdint>

int main() {
  // Includes packed words whose float carriers are infinities, quiet/signaling
  // NaNs, signed zero, and subnormal values. Their payloads must remain intact.
  const uint32_t words[] = {
      0, 0x80000000, 0x00000001, 0x007fffff, 0x00800000,
      0x7f800000, 0xff800000, 0x7fc12345, 0x7f800001, 0xffffffff,
      0x40003f80, 0x40804000,
  };
  for (uint32_t word : words) {
    const auto value = pto::bf16x2_from_bits(word);
    assert(pto::bf16x2_to_bits(value) == word);
    assert(pto::bf16x2_lane_bits<0>(value) == (word & 0xffff));
    assert(pto::bf16x2_lane_bits<1>(value) == (word >> 16));
  }

  // Cover every BF16 encoding in both positions with differing adjacent lanes.
  for (uint32_t lane = 0; lane < 0x10000; ++lane) {
    const auto other = static_cast<uint16_t>(lane ^ 0xa55a);
    const auto value = pto::bf16x2_pack_bits(static_cast<uint16_t>(lane), other);
    assert(pto::bf16x2_lane_bits<0>(value) == lane);
    assert(pto::bf16x2_lane_bits<1>(value) == other);
    assert(pto::bf16x2_to_bits(value) == (lane | (uint32_t(other) << 16)));
  }
  assert(pto::bf16x2_to_bits(pto::bf16x2_pack_bits(0x3f80, 0x4000)) ==
         0x40003f80);
  assert(pto::bf16x2_to_bits(pto::bf16x2_pack_bits(0x4000, 0x4080)) ==
         0x40804000);
}
