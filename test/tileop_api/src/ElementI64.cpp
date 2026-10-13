#include <common/pto_tileop.hpp>

#include <bit>
#include <cstdint>

namespace {

template <typename T, unsigned Count> struct Buffer { T data[Count]; };

constexpr uint64_t pattern(unsigned index) {
  constexpr uint64_t edge[] = {
      UINT64_C(0), UINT64_C(0x8000000000000000), UINT64_MAX,
      UINT64_C(0x1122334455667788), UINT64_C(0x5566778811223344),
      UINT64_C(0xffffffff00000000), UINT64_C(0x00000000ffffffff),
      UINT64_C(1)};
  return edge[index % 8] ^ (UINT64_C(0x0101010101010101) * (index / 8));
}

template <typename T> consteval Buffer<T, 32> input_fixture() {
  Buffer<T, 32> result{};
  for (unsigned index = 0; index < 32; ++index) {
    if constexpr (std::is_same_v<T, uint64_t>)
      result.data[index] = pattern(index);
    else
      result.data[index] = std::bit_cast<int64_t>(pattern(index));
  }
  return result;
}

template <typename T> consteval Buffer<T, 32> output_fixture() {
  Buffer<T, 32> result{};
  constexpr uint64_t sentinel = UINT64_C(0xdeadbeef5a5aa5a5);
  for (auto &element : result.data) {
    if constexpr (std::is_same_v<T, uint64_t>)
      element = sentinel;
    else
      element = std::bit_cast<int64_t>(sentinel);
  }
  return result;
}

consteval Buffer<uint32_t, 32> plane_fixture(bool high_plane) {
  Buffer<uint32_t, 32> result{};
  for (unsigned row = 0; row < 32; ++row)
    result.data[row] =
        pto::ElementTile<uint64_t, 32>::CubePhysicalPlaneWord(
            pattern(row), high_plane);
  return result;
}

template <unsigned Count>
consteval Buffer<uint8_t, Count> byte_fixture(uint8_t value) {
  Buffer<uint8_t, Count> result{};
  for (auto &element : result.data)
    element = value;
  return result;
}

} // namespace

extern "C" {
alignas(4096) Buffer<uint8_t, 4096> element_i64_u64_input_guard_before =
    byte_fixture<4096>(0x5a);
alignas(4096) Buffer<uint64_t, 32> element_i64_u64_input =
    input_fixture<uint64_t>();
alignas(8) Buffer<uint8_t, 64> element_i64_u64_input_guard_after =
    byte_fixture<64>(0xa5);
alignas(4096) Buffer<uint8_t, 4096> element_i64_u64_output_guard_before =
    byte_fixture<4096>(0x5a);
alignas(4096) Buffer<uint64_t, 32> element_i64_u64_output =
    output_fixture<uint64_t>();
alignas(8) Buffer<uint8_t, 64> element_i64_u64_output_guard_after =
    byte_fixture<64>(0xa5);
alignas(4096) Buffer<uint8_t, 4096> element_i64_s64_input_guard_before =
    byte_fixture<4096>(0x5a);
alignas(4096) Buffer<int64_t, 32> element_i64_s64_input =
    input_fixture<int64_t>();
alignas(8) Buffer<uint8_t, 64> element_i64_s64_input_guard_after =
    byte_fixture<64>(0xa5);
alignas(4096) Buffer<uint8_t, 4096> element_i64_s64_output_guard_before =
    byte_fixture<4096>(0x5a);
alignas(4096) Buffer<int64_t, 32> element_i64_s64_output =
    output_fixture<int64_t>();
alignas(8) Buffer<uint8_t, 64> element_i64_s64_output_guard_after =
    byte_fixture<64>(0xa5);
alignas(64) Buffer<uint32_t, 32> element_i64_expected_low =
    plane_fixture(false);
alignas(64) Buffer<uint32_t, 32> element_i64_expected_high =
    plane_fixture(true);
}

int main() {
  using U64Leaf = pto::ElementTile<uint64_t, 32>;
  using S64Leaf = pto::ElementTile<int64_t, 32>;
  static_assert(U64Leaf::LogicalTileBytes == 256 &&
                U64Leaf::TilesizeCode == 2);
  static_assert(S64Leaf::LogicalTileBytes == 256 &&
                S64Leaf::TilesizeCode == 2);

  U64Leaf unsigned_tile;
  S64Leaf signed_tile;
  TLOAD(unsigned_tile, element_i64_u64_input.data, 32);
  TSTORE(element_i64_u64_output.data, unsigned_tile, 32);
  TLOAD(signed_tile, element_i64_s64_input.data, 32);
  TSTORE(element_i64_s64_output.data, signed_tile, 32);

  return 0;
}
