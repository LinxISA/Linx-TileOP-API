// Real-model regression: signed byte offsets and canonical U8 mask lanes.
#include <common/pto_tileop.hpp>
using namespace pto;
#ifndef MASK_OPERATION
#define MASK_OPERATION 0
#endif
using D = Tile<Location::Vec, uint32_t, 1, 32>;
using I = Tile<Location::Vec, int32_t, 1, 32>;
using M = Tile<Location::Vec, uint8_t, 1, 128, BLayout::RowMajor, 1, 32>;
using G = global_tensor<uint32_t, RowMajor<1, 32>>;

alignas(128) uint32_t memory[32];
alignas(128) int32_t offsets[32];
alignas(128) uint8_t enabled[32];
extern "C" {
alignas(128) uint32_t observed[4][32];
}

__attribute__((always_inline)) inline void indexed_mask_run(uint32_t *output) {
  D data;
  I index;
  M mask;
  global_tensor<int32_t, RowMajor<1, 32>> index_gm(offsets);
  global_tensor<uint8_t, RowMajor<1, 32>> mask_gm(enabled);
  G input(memory + 16), result(output);
  TLOAD(index, index_gm);
  TLOAD(mask, mask_gm);
#if MASK_OPERATION < 2
#if MASK_OPERATION == 0
  MGATHER_MASK<D, I, M, G, TmaPadValue::Zero>(data, input, index, mask);
#else
  MGATHER_MASK<D, I, M, G, TmaPadValue::Zero>(
      data, input, index, mask, IndexedByteRange{-64, 60});
#endif
  TSTORE(result, data);
#else
  G values(memory), target(output + 16);
  TLOAD(data, values);
#if MASK_OPERATION == 2
  MSCATTER_MASK(target, data, index, mask);
#else
  MSCATTER_MASK(target, data, index, mask, IndexedByteRange{-64, 60});
#endif
#endif
}

extern "C" int main() {
  for (unsigned j = 0; j < 32; ++j) {
    memory[j] = j + 100;
    offsets[j] = (int(j) - 16) * 4;
  }
  for (unsigned pattern = 0; pattern < 4; ++pattern) {
    for (unsigned j = 0; j < 32; ++j) {
      observed[pattern][j] = 93;
      enabled[j] = pattern == 0 ? 0 : pattern == 1 ? 1 :
                   pattern == 2 ? j % 2 : j >= 16;
    }
#ifdef MASK_NONCANONICAL
    enabled[31] = 2; // Late bad byte must still reject the complete operation.
#endif
    indexed_mask_run(observed[pattern]);
  }
  return 0;
}
