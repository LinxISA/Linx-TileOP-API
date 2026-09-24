// PTO-ISA #291: the unified Local layout model. Exercises the four TileOP
// behaviours the spec change closes:
//   1. elementwise TEPL operations select the operand Local layout through
//      B.DATR.Layout, so a CUBE_M16/M32 source keeps its physical CUBE
//      representation instead of collapsing to the NORM (RowMajor) default;
//   2. TCVT is legal on Vec-location CUBE_M16/M32 tiles (issue #267): the
//      CUBE layout is preserved while the element width may change;
//   3. GMOV copies same-layout CUBE_M16/M32 peers (CUBE_N8 stays illegal);
//   4. Matrix Bias uses the Local CUBE_N8 layout.
#include <common/pto_tileop.hpp>

using namespace pto;

// --- 1. elementwise layout selection ---
using RowMajorVec = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using CubeM16Vec = VecTileM16<float, 16, 16>;
using CubeM32Vec = VecTileM32<float, 32, 32>;

__attribute__((noinline)) void tadd_rowmajor(RowMajorVec &d, RowMajorVec &a,
                                             RowMajorVec &b) {
  TADD(d, a, b);
}

__attribute__((noinline)) void tadd_cube_m16(CubeM16Vec &d, CubeM16Vec &a,
                                             CubeM16Vec &b) {
  TADD(d, a, b);
}

__attribute__((noinline)) void tadd_cube_m32(CubeM32Vec &d, CubeM32Vec &a,
                                             CubeM32Vec &b) {
  TMUL(d, a, b);
}

// Tile-scalar and unary forms share the same B.DATR Layout selector.
__attribute__((noinline)) void tmuls_cube_m32(CubeM32Vec &d, CubeM32Vec &a) {
  TMULS(d, a, 2.0f);
}

__attribute__((noinline)) void tabs_cube_m16(CubeM16Vec &d, CubeM16Vec &a) {
  TABS(d, a);
}

__attribute__((noinline)) void texpands_cube_m32(CubeM32Vec &d) {
  TEXPANDS(d, 1.0f);
}

// --- 2. Vec-location CUBE_M16/M32 TCVT (issue #267) ---
using VecM16Fp32 = VecTileM16<float, 16, 16>;
using VecM16Fp16 = VecTileM16<__half, 16, 16>;
using VecM32Fp32 = VecTileM32<float, 32, 32>;
using VecM32Fp16 = VecTileM32<__half, 32, 32>;
using VecM32E8M0 = VecTileM32<__fp8_e8m0, 32, 32>;

__attribute__((noinline)) void tcvt_vec_m16(VecM16Fp16 &dst, VecM16Fp32 &src) {
  TCVT(dst, src);
}

__attribute__((noinline)) void tcvt_vec_m32(VecM32Fp16 &dst, VecM32Fp32 &src) {
  TCVT(dst, src);
}

__attribute__((noinline)) void tcvt_vec_m32_e8m0(VecM32E8M0 &dst,
                                                 VecM32Fp32 &src) {
  // MX scale quantization needs the floor rounding mode (OCP MX 6.3).
  TCVT<LINX_RDN>(dst, src);
}

// The Matrix-location forms remain available; location is no longer part of
// the TCVT legality predicate, only the layout and the valid shape are.
using MatrixM16Fp32 = CubeTileM16<float, 16, 16>;
using MatrixM16Fp16 = CubeTileM16<__half, 16, 16>;

__attribute__((noinline)) void tcvt_matrix_m16(MatrixM16Fp16 &dst,
                                               MatrixM16Fp32 &src) {
  TCVT(dst, src);
}

// --- 3. GMOV same-layout peer copies ---
__attribute__((noinline)) void gmov_rowmajor(RowMajorVec &d, RowMajorVec &s) {
  GMOV<15>(d, 0, s);
}

__attribute__((noinline)) void gmov_cube_m16(CubeM16Vec &d, CubeM16Vec &s) {
  GMOV<15>(d, 0, s);
}

__attribute__((noinline)) void gmov_cube_m32(CubeM32Vec &d, CubeM32Vec &s) {
  GMOV<15>(d, 0, s);
}

// --- 3a. reduce/expand (SFU) selects the operand layout too ---
using Red32 = VecTileM32<float, 32, 1>;
using Red16 = VecTileM16<float, 16, 1>;
using RedRowMajor = Tile<Location::Vec, float, 32, 1, BLayout::RowMajor>;
using RedCol32 = VecTileM32<float, 1, 32>;   // 1 x N for TCOL*

__attribute__((noinline)) void rowmax_cube_m32(Red32 &d, CubeM32Vec &s) {
  TROWMAX(d, s);
}

__attribute__((noinline)) void rowsum_cube_m16(Red16 &d, CubeM16Vec &s) {
  TROWSUM(d, s);
}

__attribute__((noinline)) void rowargmax_cube_m32(Red32 &d, CubeM32Vec &s) {
  TROWARGMAX(d, s);
}

__attribute__((noinline)) void colmin_cube_m32(RedCol32 &d, CubeM32Vec &s) {
  TCOLMIN(d, s);
}

__attribute__((noinline)) void rowexpandadd_cube_m32(CubeM32Vec &d,
                                                     CubeM32Vec &s,
                                                     Red32 &broadcast) {
  TROWEXPANDADD(d, s, broadcast);
}

__attribute__((noinline)) void rowmax_rowmajor(RedRowMajor &d,
                                               RowMajorVec &s) {
  TROWMAX(d, s);
}

// --- 3b. CELL rearrangement always carries a CUBE layout ---
using CellM32F = CubeTileM32<float, 32, 32>;
using CellM32U8 = CubeTileM32<uint8_t, 32, 32>;
using CellM32U16 = CubeTileM32<uint16_t, 32, 32>;
using CellM32U32 = CubeTileM32<uint32_t, 32, 32>;
using CellM16F = CubeTileM16<float, 16, 16>;
using CellM16U8 = CubeTileM16<uint8_t, 16, 16>;
using CellM16U16 = CubeTileM16<uint16_t, 16, 16>;
using CellM16U32 = CubeTileM16<uint32_t, 16, 16>;

using PackM32U8Source = CubeTileM32<uint8_t, 32, 32>;
using PackM32U16Source = CubeTileM32<uint16_t, 32, 16>;
using PackM32U32Out = CubeTileM32<uint32_t, 32, 8>;
using PackM16U8Source = CubeTileM16<uint8_t, 16, 16>;
using PackM16U16Out = CubeTileM16<uint16_t, 16, 8>;

using PackM16U8Out = CubeTileM16<uint8_t, 16, 32>;
using PackM16U16Source = CubeTileM16<uint16_t, 16, 8>;

__attribute__((noinline)) void pack_to_u8(PackM16U8Out &d,
                                          CellM16U8 &a,
                                          PackM16U16Source &b) {
  TPACK(d, a, b, 0x00000001);
}

__attribute__((noinline)) void permute_cube_m32(CellM32F &d, CellM32F &a,
                                                CellM32F &b, CellM32U8 &idx) {
  TPERMUTE(d, a, b, idx);
}

__attribute__((noinline)) void shuf_cube_m16(CellM16F &d, CellM16F &s,
                                             CellM16U32 &ctrl) {
  TSHUF(d, s, ctrl, 0);
}

__attribute__((noinline)) void pack_cube_m32(CellM32U32 &d, CellM32U32 &a,
                                             CellM32U32 &b) {
  TPACK(d, a, b, 0x00000202);
}

__attribute__((noinline)) void unpack_cube_m32(CellM32U32 &d, CellM32U32 &a) {
  TUNPACK(d, a, 0x00000201);
}

// Issue #338 carrier-field forms: source backing widths are independent from
// the operation carrier type, while the logical CUBE shape remains shared.
__attribute__((noinline)) void pack_u8_u16_to_u32(PackM32U32Out &d,
                                                 PackM32U8Source &a,
                                                 PackM32U16Source &b) {
  TPACK(d, a, b, 0x00000201);
}

__attribute__((noinline)) void pack_u8_u8_to_u16(PackM16U16Out &d,
                                                PackM16U8Source &a,
                                                PackM16U8Source &b) {
  TPACK(d, a, b, 0x00000101);
}

using UnpackM16U16 = CubeTileM16<uint16_t, 16, 16>;
using UnpackM16U16Out = CubeTileM16<uint16_t, 16, 8>;

__attribute__((noinline)) void unpack_u16_to_u16(UnpackM16U16Out &d,
                                                 UnpackM16U16 &a) {
  TUNPACK(d, a, 0x00000201);
}

// --- 4. Matrix Bias uses the Local CUBE_N8 layout ---
using M16A = CubeTileM16<float, 16, 16>;
using M16B = CubeTileN8<float, 16, 16>;
using M16D = CubeAccumulatorM16<float, 16, 16>;
using M16Bias = CubeBias<float, 16>;

using M32A = CubeTileM32<float, 32, 32>;
using M32B = CubeTileN8<float, 32, 32>;
using M32D = CubeAccumulatorM32<float, 32, 32>;
using M32Bias = CubeBias<float, 32>;

static_assert(M16Bias::BFractal == BLayout::CubeN8 && M16Bias::ValidRow == 1,
              "CubeBias<_, 16> declares the CUBE_N8 layout");
static_assert(M32Bias::BFractal == BLayout::CubeN8 && M32Bias::ValidRow == 1,
              "CubeBias<_, 32> declares the CUBE_N8 layout");
static_assert(M16Bias::Loc == Location::Bias && M32Bias::Loc == Location::Bias,
              "CubeBias keeps the Bias operand role");

__attribute__((noinline)) void bias_m16(M16D &d, M16A &a, M16B &b,
                                        M16Bias &bias) {
  TMATMUL_BIAS(d, a, b, bias);
}

__attribute__((noinline)) void bias_m32(M32D &d, M32A &a, M32B &b,
                                        M32Bias &bias) {
  TMATMUL_BIAS(d, a, b, bias);
}

int main() {
  RowMajorVec rm_d, rm_a, rm_b;
  CubeM16Vec m16_d, m16_a, m16_b;
  CubeM32Vec m32_d, m32_a, m32_b;
  tadd_rowmajor(rm_d, rm_a, rm_b);
  tadd_cube_m16(m16_d, m16_a, m16_b);
  tadd_cube_m32(m32_d, m32_a, m32_b);
  tmuls_cube_m32(m32_d, m32_a);
  tabs_cube_m16(m16_d, m16_a);
  texpands_cube_m32(m32_d);

  VecM16Fp32 v16s;
  VecM16Fp16 v16d;
  VecM32Fp32 v32s;
  VecM32Fp16 v32d;
  VecM32E8M0 v32e;
  tcvt_vec_m16(v16d, v16s);
  tcvt_vec_m32(v32d, v32s);
  tcvt_vec_m32_e8m0(v32e, v32s);
  MatrixM16Fp32 m16s;
  MatrixM16Fp16 m16d;
  tcvt_matrix_m16(m16d, m16s);

  gmov_rowmajor(rm_d, rm_a);
  gmov_cube_m16(m16_d, m16_a);
  gmov_cube_m32(m32_d, m32_a);

  Red32 red32;
  Red16 red16;
  RedRowMajor red_rm;
  rowmax_cube_m32(red32, m32_a);
  rowsum_cube_m16(red16, m16_a);
  rowargmax_cube_m32(red32, m32_a);
  RedCol32 redcol32;
  colmin_cube_m32(redcol32, m32_a);
  rowexpandadd_cube_m32(m32_d, m32_a, red32);
  rowmax_rowmajor(red_rm, rm_a);

  CellM32F cell_f_d, cell_f_a, cell_f_b;
  CellM32U8 cell_u8;
  CellM32U16 cell_u16;
  CellM32U32 cell_u32_d, cell_u32_a, cell_u32_b;
  CellM16F cell16_f_d, cell16_f_a;
  CellM16U8 cell16_u8;
  CellM16U16 cell16_u16;
  CellM16U32 cell16_u32;
  permute_cube_m32(cell_f_d, cell_f_a, cell_f_b, cell_u8);
  shuf_cube_m16(cell16_f_d, cell16_f_a, cell16_u32);
  pack_cube_m32(cell_u32_d, cell_u32_a, cell_u32_b);
  unpack_cube_m32(cell_u32_d, cell_u32_a);
  PackM32U32Out pack_m32_d;
  PackM32U8Source pack_m32_u8;
  PackM32U16Source pack_m32_u16;
  pack_u8_u16_to_u32(pack_m32_d, pack_m32_u8, pack_m32_u16);
  PackM16U16Out pack_m16_d;
  PackM16U8Source pack_m16_u8;
  pack_u8_u8_to_u16(pack_m16_d, pack_m16_u8, pack_m16_u8);
  PackM16U8Out pack_u8_dst;
  PackM16U16Source pack_u16_src;
  pack_to_u8(pack_u8_dst, cell16_u8, pack_u16_src);
  UnpackM16U16 unpack16_src;
  UnpackM16U16Out unpack16_dst;
  unpack_u16_to_u16(unpack16_dst, unpack16_src);

  M16A a16;
  M16B b16;
  M16D d16;
  M16Bias bias16;
  M32A a32;
  M32B b32;
  M32D d32;
  M32Bias bias32;
  bias_m16(d16, a16, b16, bias16);
  bias_m32(d32, a32, b32, bias32);
  return 0;
}
