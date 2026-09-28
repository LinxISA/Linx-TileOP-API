#include "../data.hpp"
#include <common/pto_tileop.hpp>

#ifdef LINX_PMC
#include "../linxStartEnd.hpp"
#endif

template <typename T, int descending>
void test_tci(T *dst, T start) {
  constexpr uint64_t cols = 256 / sizeof(T);
  using gm_shape = global_tensor<T, RowMajor<1, cols>>;
  using tile_shape =
      Tile<Location::Vec, T, 1, cols, BLayout::RowMajor, 1, cols>;

  gm_shape result(dst);
  tile_shape sequence;
  TCI<tile_shape, T, descending>(sequence, start);
  TSTORE(result, sequence);
}

template <typename TileT, int row_step, int col_step>
void test_tci_2d(typename TileT::DType *dst,
                 typename TileT::DType start) {
  using gm_shape = global_tensor<typename TileT::DType,
                                 RowMajor<TileT::Rows, TileT::Cols>>;
  gm_shape result(dst);
  TileT sequence;
  TCI_2D<TileT, typename TileT::DType, row_step, col_step>(sequence, start);
  TSTORE(result, sequence);
}

using TCI2DM16 = CubeTileM16<int32_t, 16, 16, 3, 5>;
using TCI2DM32 = CubeTileM32<uint16_t, 32, 32, 7, 9>;

int main() {
  constexpr uint64_t count16 = 256 / sizeof(uint16_t);
  constexpr uint64_t count32 = 256 / sizeof(uint32_t);

  int16_t *s16 = static_cast<int16_t *>(malloc(count16 * sizeof(int16_t)));
  uint16_t *u16 = static_cast<uint16_t *>(malloc(count16 * sizeof(uint16_t)));
  int32_t *s32 = static_cast<int32_t *>(malloc(count32 * sizeof(int32_t)));
  uint32_t *u32 = static_cast<uint32_t *>(malloc(count32 * sizeof(uint32_t)));
  check_mem_alloc(s16);
  check_mem_alloc(u16);
  check_mem_alloc(s32);
  check_mem_alloc(u32);

#ifdef LINX_PMC
  PMC_START();
#endif

  test_tci<int16_t, 0>(s16, static_cast<int16_t>(64));
  test_tci<uint16_t, 1>(u16, static_cast<uint16_t>(64));
  test_tci<int32_t, 0>(s32, static_cast<int32_t>(64));
  test_tci<uint32_t, 1>(u32, static_cast<uint32_t>(64));

  int32_t cube_m16[16 * 16] = {};
  uint16_t cube_m32[32 * 32] = {};
  test_tci_2d<TCI2DM16, 1, 0>(cube_m16, 10);
  test_tci_2d<TCI2DM32, -1, 1>(cube_m32, 20);

#ifdef LINX_PMC
  PMC_END();
#endif

  OutArray(s16, count16);
  OutArray(u16, count16);
  OutArray(s32, count32);
  OutArray(u32, count32);

  free(s16);
  free(u16);
  free(s32);
  free(u32);
  return 0;
}
