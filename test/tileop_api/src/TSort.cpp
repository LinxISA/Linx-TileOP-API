// TSORT: stable per-row group sort producing sorted values (FP16/FP32) and
// group-local original U32 indices (PTO 0.58.1 TEPL Mode3 Fn12 / 0x06C).
#include <common/pto_tileop.hpp>

using namespace pto;

using ValF32 = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using ValF16 = Tile<Location::Vec, __half, 32, 32, BLayout::RowMajor>;
using Idx32 = Tile<Location::Vec, uint32_t, 32, 32, BLayout::RowMajor>;

__attribute__((noinline)) void sort_f32_32_asc(ValF32 &v, Idx32 &i, ValF32 &s) {
  TSORT(v, i, s);                       // sortWidth=32 ascending
}
__attribute__((noinline)) void sort_f32_16_desc(ValF32 &v, Idx32 &i, ValF32 &s) {
  TSORT(v, i, s, 16, true);             // sortWidth=16 descending
}
__attribute__((noinline)) void sort_f16_64_asc(ValF16 &v, Idx32 &i, ValF16 &s) {
  TSORT(v, i, s, 64, false);            // FP16 value (2KB) vs U32 index (4KB)
}
__attribute__((noinline)) void sort_f16_1_asc(ValF16 &v, Idx32 &i, ValF16 &s) {
  TSORT(v, i, s, 1, false);             // sortWidth=1
}

void use(void *) {}

int main() {
  ValF32 v32, s32;
  ValF16 v16, s16;
  Idx32 i32;
  sort_f32_32_asc(v32, i32, s32);
  sort_f32_16_desc(v32, i32, s32);
  sort_f16_64_asc(v16, i32, s16);
  sort_f16_1_asc(v16, i32, s16);
  use(&v32);
  return 0;
}
