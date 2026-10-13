#include <common/pto_tileop.hpp>

#include <bit>
#include <cstdint>

#ifndef STORAGE_ELEMENTS
#define STORAGE_ELEMENTS 1024
#endif
#ifndef STORAGE_HIERARCHY
#define STORAGE_HIERARCHY 0
#endif
#ifndef STORAGE_BITS
#define STORAGE_BITS 64
#endif
#ifndef STORAGE_SIGNED
#define STORAGE_SIGNED 0
#endif

using Unsigned = std::conditional_t<STORAGE_BITS == 64, uint64_t, uint32_t>;
using Element = std::conditional_t<STORAGE_SIGNED,
    std::conditional_t<STORAGE_BITS == 64, int64_t, int32_t>, Unsigned>;
constexpr unsigned Count = STORAGE_ELEMENTS;
template <typename T, unsigned N> struct Buffer { T data[N]; };

consteval Buffer<Element, Count> fixture(bool input) {
  Buffer<Element, Count> result{};
  constexpr uint64_t edges[] = {0, UINT64_MAX, UINT64_C(0x8000000000000000),
      UINT64_C(0x1122334455667788), UINT64_C(0xffffffff00000000),
      UINT64_C(0x00000000ffffffff), UINT64_C(0x5566778811223344), 1};
  for (unsigned i = 0; i < Count; ++i) {
    const Unsigned bits = input
        ? static_cast<Unsigned>(edges[i % 8] ^
            (UINT64_C(0x0101010101010101) * (i / 8)))
        : static_cast<Unsigned>(UINT64_C(0xdeadbeef5a5aa5a5));
    result.data[i] = std::bit_cast<Element>(bits);
  }
  return result;
}
consteval Buffer<uint8_t, 64> guard(uint8_t value) {
  Buffer<uint8_t, 64> result{};
  for (auto &byte : result.data) byte = value;
  return result;
}
extern "C" {
alignas(64) Buffer<uint8_t, 64> storage_input_before = guard(0x5a);
alignas(64) Buffer<Element, Count> storage_input = fixture(true);
alignas(64) Buffer<uint8_t, 64> storage_input_after = guard(0xa5);
alignas(64) Buffer<uint8_t, 64> storage_roundtrip_before = guard(0x5a);
alignas(64) Buffer<Element, Count> storage_roundtrip = fixture(false);
alignas(64) Buffer<uint8_t, 64> storage_roundtrip_after = guard(0xa5);
alignas(64) Buffer<uint8_t, 64> storage_reversed_before = guard(0x5a);
alignas(64) Buffer<Element, Count> storage_reversed = fixture(false);
alignas(64) Buffer<uint8_t, 64> storage_reversed_after = guard(0xa5);
}

template<unsigned N> int run_storage() {
  constexpr unsigned Columns = N / 32;
  using namespace pto;
  using Parent = ElementTile<Element, N>;
  using Leaf = ElementTile<Element, 32>;
  Parent parent;
  TLOAD(parent, storage_input.data, N);
  TSTORE(storage_roundtrip.data, parent, N);
  auto parts = TPARTVIEW<32>(parent, N);
  // Reverse whole logical columns. Native64 views must retain both planes.
  if constexpr (N <= 512 && !STORAGE_HIERARCHY) {
    TileArray<Leaf, 1, Columns> assembly;
    for (unsigned part = 0; part < Columns; ++part) {
      auto source = parts.part(Columns - 1 - part);
      Leaf leaf;
      TMOV(leaf, source);
      TMOV(assembly[0][part], leaf);
    }
    auto result = TASSEMBLY<Parent>(std::move(assembly));
    TSTORE(storage_reversed.data, result, N);
  } else {
    static_assert(N >= 256);
    constexpr unsigned ChildElements = N / 2;
    constexpr unsigned ChildColumns = ChildElements / 32;
    TileArray<ElementTile<Element, ChildElements>, 1, 2> assembly;
    for (unsigned group = 0; group < 2; ++group) {
      TileArray<Leaf, 1, ChildColumns> child;
      for (unsigned part = 0; part < ChildColumns; ++part) {
        auto source = parts.part(Columns - 1 - (group * ChildColumns + part));
        Leaf leaf;
        TMOV(leaf, source);
        TMOV(child[0][part], leaf);
      }
      auto closed = TASSEMBLY<ElementTile<Element, ChildElements>>(std::move(child));
      TMOV(assembly[0][group], closed);
    }
    auto result = TASSEMBLY<Parent>(std::move(assembly));
    TSTORE(storage_reversed.data, result, N);
  }
  return 0;
}

int main() { return run_storage<Count>(); }
