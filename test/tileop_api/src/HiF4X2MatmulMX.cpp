#include <common/pto_tileop.hpp>
using namespace pto;
using D = CubeAccumulatorM16<float, 16, 16>;
using C = CubeAccumulatorM16<float, 16, 16>;
using A = CubeTileM16<__fp4_hif4x2, 16, 64>;
using B = CubeTileN8<__fp4_hif4x2, 64, 16>;
using SA = Tile<Location::Scaling, uint32_t, 16, 1, BLayout::RowMajor, 16, 1>;
using SB = Tile<Location::Scaling, uint32_t, 1, 16, BLayout::RowMajor, 1, 16>;
using SharedA = SharedMatrixLeft<__fp4_hif4x2, 16, 64>;
using SharedB = SharedMatrixRight<__fp4_hif4x2, 64, 16>;

template <typename AT, typename SAT, typename BT, typename SBT>
void do_hif4x2(D &d, C &c, AT &a, SAT &sa, BT &b, SBT &sb) {
  TMATMUL_MX<3>(d, a, sa, b, sb, fixp::keep_acc());
  TMATMUL_MX_ACC<3>(d, c, a, sa, b, sb, fixp::keep_acc());
}

// Four independent storage combinations prove that each scale follows its
// corresponding primary (A or B) rather than inheriting the other side.
void hif4x2_local_local(float *o, __fp4_hif4x2 *ai, __fp4_hif4x2 *bi,
                        uint32_t *sai, uint32_t *sbi) {
  global_tensor<float, RowMajor<16, 16>> gd(o);
  global_tensor<__fp4_hif4x2, RowMajor<16, 64>> ga(ai);
  global_tensor<__fp4_hif4x2, RowMajor<64, 16>> gb(bi);
  global_tensor<uint32_t, RowMajor<16, 1>> gsa(sai);
  global_tensor<uint32_t, RowMajor<1, 16>> gsb(sbi);
  D d; C c; A a; B b; SA sa; SB sb;
  TLOAD_CUBE(a, ga); TLOAD_CUBE(b, gb); TLOAD(sa, gsa); TLOAD(sb, gsb);
  do_hif4x2(d, c, a, sa, b, sb); TSTORE_CUBE(gd, d);
}

void hif4x2_local_shared(float *o, __fp4_hif4x2 *ai, __fp4_hif4x2 *bi,
                         uint32_t *sai, uint32_t *sbi) {
  global_tensor<float, RowMajor<16, 16>> gd(o);
  global_tensor<__fp4_hif4x2, RowMajor<16, 64>> ga(ai);
  global_tensor<__fp4_hif4x2, RowMajor<64, 16>> gb(bi);
  global_tensor<uint32_t, RowMajor<16, 1>> gsa(sai);
  global_tensor<uint32_t, RowMajor<1, 16>> gsb(sbi);
  D d; C c; A a; SharedTile<SharedB> b; SA sa; SharedTile<SB> sb;
  TLOAD_CUBE(a, ga); TLOAD(b, gb); TLOAD(sa, gsa); TLOAD(sb, gsb);
  do_hif4x2(d, c, a, sa, b, sb); TSTORE_CUBE(gd, d);
}

void hif4x2_shared_local(float *o, __fp4_hif4x2 *ai, __fp4_hif4x2 *bi,
                         uint32_t *sai, uint32_t *sbi) {
  global_tensor<float, RowMajor<16, 16>> gd(o);
  global_tensor<__fp4_hif4x2, RowMajor<16, 64>> ga(ai);
  global_tensor<__fp4_hif4x2, RowMajor<64, 16>> gb(bi);
  global_tensor<uint32_t, RowMajor<16, 1>> gsa(sai);
  global_tensor<uint32_t, RowMajor<1, 16>> gsb(sbi);
  D d; C c; SharedTile<SharedA> a; SharedTile<SA> sa; B b; SB sb;
  TLOAD(a, ga); TLOAD(sa, gsa); TLOAD_CUBE(b, gb); TLOAD(sb, gsb);
  do_hif4x2(d, c, a, sa, b, sb); TSTORE_CUBE(gd, d);
}

void hif4x2_shared_shared(float *o, __fp4_hif4x2 *ai, __fp4_hif4x2 *bi,
                          uint32_t *sai, uint32_t *sbi) {
  global_tensor<float, RowMajor<16, 16>> gd(o);
  global_tensor<__fp4_hif4x2, RowMajor<16, 64>> ga(ai);
  global_tensor<__fp4_hif4x2, RowMajor<64, 16>> gb(bi);
  global_tensor<uint32_t, RowMajor<16, 1>> gsa(sai);
  global_tensor<uint32_t, RowMajor<1, 16>> gsb(sbi);
  D d; C c; SharedTile<SharedA> a; SharedTile<SA> sa;
  SharedTile<SharedB> b; SharedTile<SB> sb;
  TLOAD(a, ga); TLOAD(sa, gsa); TLOAD(b, gb); TLOAD(sb, gsb);
  do_hif4x2(d, c, a, sa, b, sb); TSTORE_CUBE(gd, d);
}