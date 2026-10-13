#include <common/pto_tileop.hpp>

using namespace pto;

void assemble_f16_hierarchy(const __half *input, __half *output) {
  constexpr unsigned kElements = 2048;
  constexpr unsigned kLeafElements = 64;
  TileArray<ElementTile<__half, 128>, 1, 16> level2;

  for (unsigned group = 0; group < 16; ++group) {
    TileArray<ElementTile<__half, kLeafElements>, 1, 2> level1;
    for (unsigned part = 0; part < 2; ++part) {
      ElementTile<__half, kLeafElements> leaf;
      const unsigned global_part = group * 2 + part;
      global_tensor<__half, RowMajor<32, 64>> source(
          input + 2 * global_part);
      TLOAD(leaf, source);
      TMOV(level1[0][part], leaf);
    }
    auto closed =
        TASSEMBLY<ElementTile<__half, 128>>(std::move(level1));
    TMOV(level2[0][group], closed);
  }

  auto result = TASSEMBLY<ElementTile<__half, kElements>>(std::move(level2));
  TSTORE(output, result, kElements);
}

void assemble_f32_hierarchy(const float *input, float *output) {
  constexpr unsigned kElements = 16384;
  constexpr unsigned kLeafElements = 32;
  TileArray<ElementTile<float, 2048>, 1, 8> level3;

  for (unsigned outer = 0; outer < 8; ++outer) {
    TileArray<ElementTile<float, 128>, 1, 16> level2;
    for (unsigned middle = 0; middle < 16; ++middle) {
      TileArray<ElementTile<float, kLeafElements>, 1, 4> level1;
      for (unsigned part = 0; part < 4; ++part) {
        ElementTile<float, kLeafElements> leaf;
        const unsigned global_part = (outer * 16 + middle) * 4 + part;
        global_tensor<float, RowMajor<32, 512>> source(input + global_part);
        TLOAD(leaf, source);
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
