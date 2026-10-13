#include <common/pto_tileop.hpp>

using namespace pto;

#ifndef PTO_HIERARCHY_DISABLE_F16
void assemble_f16_hierarchy_from_element_for(const __half *input,
                                              __half *output) {
  constexpr unsigned kElements = 2048;
  constexpr unsigned kLeafElements = 64;
  TileArray<ElementTile<__half, 128>, 1, 16> level2;

  for (unsigned group = 0; group < 16; ++group) {
    TileArray<ElementTile<__half, kLeafElements>, 1, 2> level1;
    for (unsigned part = 0; part < 2; ++part) {
      ElementTile<__half, kLeafElements> leaf;
      auto &elements = TPARTELEMENT(leaf);
      const unsigned global_part = group * 2 + part;
#pragma pto element for
      for (unsigned element = 0; element < kLeafElements; ++element) {
        const unsigned logical =
            (element / 2) * 64 + 2 * global_part + element % 2;
        elements[element] = input[logical];
      }
      TMOV(level1[0][part], leaf);
    }
    auto closed =
        TASSEMBLY<ElementTile<__half, 128>>(std::move(level1));
    TMOV(level2[0][group], closed);
  }

  auto result = TASSEMBLY<ElementTile<__half, kElements>>(std::move(level2));
  TSTORE(output, result, kElements);
}
#endif

#ifndef PTO_HIERARCHY_DISABLE_F32
void assemble_f32_hierarchy_from_element_for(const float *input,
                                              float *output) {
  constexpr unsigned kElements = 16384;
  constexpr unsigned kLeafElements = 32;
  TileArray<ElementTile<float, 2048>, 1, 8> level3;

  for (unsigned outer = 0; outer < 8; ++outer) {
    TileArray<ElementTile<float, 128>, 1, 16> level2;
    for (unsigned middle = 0; middle < 16; ++middle) {
      TileArray<ElementTile<float, kLeafElements>, 1, 4> level1;
      for (unsigned part = 0; part < 4; ++part) {
        ElementTile<float, kLeafElements> leaf;
        auto &elements = TPARTELEMENT(leaf);
        const unsigned global_part = (outer * 16 + middle) * 4 + part;
        const float *part_input = input + global_part;
#pragma pto element for
        for (unsigned element = 0; element < kLeafElements; ++element)
          elements[element] = part_input[element * 512];
        TMOV(level1[0][part], leaf);
      }
      auto closed128 =
          TASSEMBLY<ElementTile<float, 128>>(std::move(level1));
      TMOV(level2[0][middle], closed128);
    }
    auto closed2048 =
        TASSEMBLY<ElementTile<float, 2048>>(std::move(level2));
    TMOV(level3[0][outer], closed2048);
  }

  auto result =
      TASSEMBLY<ElementTile<float, kElements>>(std::move(level3));
  TSTORE(output, result, kElements);
}
#endif

#ifdef PTO_HIERARCHY_RUNTIME_CHECK
// 原始位 transport 和重叠写回：只有完整层级关闭后才可消费快照。
extern "C" {
alignas(64) __half hierarchy_f16_input[2049];
alignas(64) __half hierarchy_f16_output[2048];
alignas(64) float hierarchy_f32_input[16385];
alignas(64) float hierarchy_f32_output[16384];
}
int main() {
  for (unsigned i = 0; i < 2049; ++i) {
    uint16_t bits = static_cast<uint16_t>(i * 40503u + 0x35a7u);
    __builtin_memcpy(hierarchy_f16_input + i, &bits, sizeof(bits));
  }
  for (unsigned i = 0; i < 16385; ++i) {
    uint32_t bits = i * 104729u + 0x80003039u;
    __builtin_memcpy(hierarchy_f32_input + i, &bits, sizeof(bits));
  }
  [[clang::noinline]]
  assemble_f16_hierarchy_from_element_for(hierarchy_f16_input, hierarchy_f16_output);
  [[clang::noinline]]
  assemble_f32_hierarchy_from_element_for(hierarchy_f32_input, hierarchy_f32_output);
  [[clang::noinline]]
  assemble_f16_hierarchy_from_element_for(hierarchy_f16_input, hierarchy_f16_input + 1);
  [[clang::noinline]]
  assemble_f32_hierarchy_from_element_for(hierarchy_f32_input, hierarchy_f32_input + 1);
  return 0;
}
#endif
