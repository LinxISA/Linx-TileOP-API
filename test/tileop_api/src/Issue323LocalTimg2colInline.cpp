// Compile/assembly fixtures for #323; model execution is a separate check.
// Each entry owns its Tile so no partially defined Tile crosses its ABI.
#include <common/pto_tileop.hpp>

using namespace pto;

// NHWC [1,4,4,3], 1x1, no padding, unit dilation and stride.
constexpr uint64_t image4x4 = 4ULL | (4ULL << 16) | (3ULL << 32) |
                            (1ULL << 48) | (1ULL << 56);
constexpr uint64_t unitSteps = (1ULL << 32) | (1ULL << 37) |
                              (1ULL << 42) | (1ULL << 48);
using Image = global_tensor<float, RowMajor<16, 3>>;
using Output = global_tensor<float, RowMajor<16, 8>>;

extern "C" void m16_params_empty(float *input, float *output) {
  Image src(input);
  Output out(output);
  CubeTileM16<float, 16, 8> tile;
  TIMG2COL(tile, src, TIMG2COLParams{image4x4, unitSteps, 0});
  if (get_thread_idx() == 0)
    TSTORE(out, tile);
}

extern "C" void m16_words_empty(float *input, float *output) {
  Image src(input);
  Output out(output);
  CubeTileM16<float, 16, 8> tile;
  TIMG2COL(tile, src, image4x4, unitSteps, 0);
  if (get_thread_idx() == 0)
    TSTORE(out, tile);
}

// A taller image supplies every requested output window, including full
// groups. Dynamic valid rows let the one-row tail store only its own rows.
template <int M, int GroupRows>
__attribute__((always_inline)) inline
void group_case(float *input, float *output) {
  constexpr uint64_t image = 16ULL | (8ULL << 16) | (3ULL << 32) |
                             (1ULL << 48) | (1ULL << 56);
  global_tensor<float, RowMajor<128, 3>> src(input);
  const unsigned pe = get_thread_idx();
  const unsigned start = pe * M;
  const unsigned rows = GroupRows <= start ? 0 :
                        (GroupRows - start < M ? GroupRows - start : M);
  using Shard = std::conditional_t<M == 16,
      CubeTileM16<float, 16, 8, -1, 8>, CubeTileM32<float, 32, 8, -1, 8>>;
  Shard tile(rows);
  // Producer participation is unconditional, including on empty-shard PEs.
  TIMG2COL(tile, src, image, unitSteps, 0, GroupRows);
  if (rows != 0) {
    global_tensor<float, RowMajor<-1, 8>> out(output + start * 8, rows);
    TSTORE(out, tile);
  }
}

extern "C" void m16_group_empty(float *input, float *output) {
  group_case<16, 16>(input, output);
}
extern "C" void m16_group_full(float *input, float *output) {
  group_case<16, 64>(input, output);
}
extern "C" void m16_group_tail(float *input, float *output) {
  group_case<16, 17>(input, output);
}
extern "C" void m32_group_empty(float *input, float *output) {
  group_case<32, 32>(input, output);
}
extern "C" void m32_group_full(float *input, float *output) {
  group_case<32, 128>(input, output);
}
