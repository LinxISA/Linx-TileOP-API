// PTO-SPEC #388: explicit nonzero TCVT modes are accepted for same-type
// FP16/BF16/FP32/FP64 and are encoded unchanged in B.DATR.  The numerical
// round-to-integral result is supplied by the target implementation.
#include <common/pto_tileop.hpp>

using namespace pto;

using F16 = Tile<Location::Vec, __half, 16, 16, BLayout::RowMajor>;
using BF16 = Tile<Location::Vec, __bf16, 16, 16, BLayout::RowMajor>;
using F32 = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using F64 = Tile<Location::Vec, double, 16, 16, BLayout::RowMajor>;

template <int RMode, typename TileT>
__attribute__((noinline)) void same_type_round(TileT &dst, TileT &src) {
  TCVT<RMode>(dst, src);
}

void exercise_same_type_rounding(F16 &f16d, F16 &f16s, BF16 &bf16d,
                                 BF16 &bf16s, F32 &f32d, F32 &f32s,
                                 F64 &f64d, F64 &f64s) {
  same_type_round<LINX_RNE>(f16d, f16s);
  same_type_round<LINX_RTZ>(bf16d, bf16s);
  same_type_round<LINX_RDN>(f32d, f32s);
  same_type_round<LINX_RUP>(f64d, f64s);
  same_type_round<LINX_RNA>(f16d, f16s);
  same_type_round<LINX_RHB>(bf16d, bf16s);
  // RTO remains legal outside the same-type floating specialization.
  using F32ToF16 = Tile<Location::Vec, __half, 16, 16, BLayout::RowMajor>;
  F32ToF16 cross_dst;
  TCVT<LINX_RTO>(cross_dst, f32s);
}

int main() { return 0; }
