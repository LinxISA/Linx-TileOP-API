#include <common/pto_tileop.hpp>

#include <cstdint>

using namespace pto;
using u32 = uint32_t;

template <typename T, unsigned N> struct Buffer { T data[N]; };

consteval Buffer<u32, 32> source_fixture() {
  Buffer<u32, 32> result{};
  for (unsigned row = 0; row < 4; ++row)
    for (unsigned col = 0; col < 8; ++col)
      result.data[row * 8 + col] = 100 * row + col + 1;
  return result;
}

consteval Buffer<u32, 32> output_fixture() {
  Buffer<u32, 32> result{};
  for (u32 &value : result.data)
    value = UINT32_C(0xdeadbeef);
  return result;
}

consteval Buffer<uint8_t, 64> guard(uint8_t value) {
  Buffer<uint8_t, 64> result{};
  for (uint8_t &byte : result.data)
    byte = value;
  return result;
}

extern "C" {
alignas(64) Buffer<uint8_t, 64> tcolsum_input_before = guard(0x5a);
alignas(64) Buffer<u32, 32> tcolsum_input = source_fixture();
alignas(64) Buffer<uint8_t, 64> tcolsum_input_after = guard(0xa5);
alignas(64) Buffer<uint8_t, 64> tcolsum_output_before = guard(0x5a);
alignas(64) Buffer<u32, 32> tcolsum_output = output_fixture();
alignas(64) Buffer<uint8_t, 64> tcolsum_output_after = guard(0xa5);
}

__attribute__((noinline)) void run_tcolsum() {
  using SourceGM = global_tensor<u32, RowMajor<4, 8>>;
  using OutputGM = global_tensor<u32, RowMajor<4, 8>>;
  using Source = Tile<Location::Vec, u32, 4, 8, BLayout::RowMajor, 3, 7>;
  using Result = Tile<Location::Vec, u32, 4, 8, BLayout::RowMajor, 1, 7>;

  SourceGM source_gm(tcolsum_input.data);
  OutputGM output_gm(tcolsum_output.data);
  Source source;
  Result result;
  TLOAD(source, source_gm);
  TCOLSUM(result, source);
  TSTORE(output_gm, result);
}

int main() {
  run_tcolsum();
  return 0;
}
