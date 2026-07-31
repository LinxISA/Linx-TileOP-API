#ifndef MATMUL_HPP
#define MATMUL_HPP

#include "common/pto_tile.hpp"
#include "jcore/template_asm.hpp"

using namespace pto;

template <typename tile_shape_A, typename tile_shape_B, typename tile_shape_C>
void __vec__ MatMul_Vec_Impl(
    typename tile_shape_C::TileDType __out__ dst,
    const typename tile_shape_A::TileDType __in__ src0,
    const typename tile_shape_B::TileDType __in__ src1) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  typename tile_shape_C::DType data = 0;

  #pragma clang loop unroll(full)
  for (size_t k = 0; k < tile_shape_A::ValidCol; ++k){
    size_t idx_0 = index<tile_shape_A>(j, k);
    size_t idx_1 = index<tile_shape_B>(k, i);
    data += blkv_get_tile_ptr(src0)[idx_0] * 
            blkv_get_tile_ptr(src1)[idx_1];
  }
  size_t idx = index<tile_shape_C>(j, i);
  blkv_get_tile_ptr(dst)[idx] = data;
}

// Matrix Multiply: C[MxN] = A[M×K] x B[KxN]
template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_B,
          is_tile_data_v tile_shape_C>
void MATMUL_Impl(tile_shape_C &dst, tile_shape_A &src0, tile_shape_B &src1) {
  static_assert(tile_shape_A::Cols == tile_shape_B::Rows,
                "MATMUL requires A.Cols == B.Rows");
  size_t M = dst.GetValidRow();
  size_t N = dst.GetValidCol();
  size_t K = src0.GetValidCol();
  static_assert(tile_shape_C::Loc != Location::Acc,
                "MATMUL output must be an ordinary Tile");
  pto_matmul_detail::matmul(dst, src0, src1, M, N, K);
}

template <typename tile_shape_A, typename tile_shape_AX,
          typename tile_shape_B, typename tile_shape_BX,
          typename tile_shape_C>
void __vec__ MatMulMx_Vec_Impl(
    typename tile_shape_C::TileDType __out__ dst,
    const typename tile_shape_A::TileDType __in__ src0,
    const typename tile_shape_AX::TileDType __in__ src0_scale,
    const typename tile_shape_B::TileDType __in__ src1,
    const typename tile_shape_BX::TileDType __in__ src1_scale) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  typename tile_shape_C::DType data = 0;

  #pragma clang loop unroll(full)
  for (size_t k = 0; k < tile_shape_A::ValidCol; ++k) {
    size_t idx_0  = index<tile_shape_A>(j, k);
    size_t idx_0x = index<tile_shape_AX>(j, k);
    size_t idx_1  = index<tile_shape_B>(k, i);
    size_t idx_1x = index<tile_shape_BX>(k, i);

    data += (blkv_get_tile_ptr(src0)[idx_0] *
             blkv_get_tile_ptr(src0_scale)[idx_0x]) *
            (blkv_get_tile_ptr(src1)[idx_1] *
             blkv_get_tile_ptr(src1_scale)[idx_1x]);
  }

  size_t idx = index<tile_shape_C>(j, i);
  blkv_get_tile_ptr(dst)[idx] = data;
}

// Matrix Multiply MX:
// C[MxN] = scale(A, AX) x scale(B, BX)
template <is_tile_data_v tile_shape_A,  is_tile_data_v tile_shape_AX,
          is_tile_data_v tile_shape_B,  is_tile_data_v tile_shape_BX,
          is_tile_data_v tile_shape_C>
void MATMULMX_Impl(tile_shape_C &dst,
                   tile_shape_A &src0, tile_shape_AX &src0_scale,
                   tile_shape_B &src1, tile_shape_BX &src1_scale) {
  static_assert(tile_shape_A::Cols == tile_shape_B::Rows,
                "Error! Cube A:Columns != Cube B:Rows");

  size_t M = dst.GetValidRow();
  size_t N = dst.GetValidCol();
  size_t K = src0.GetValidCol();

  static_assert(tile_shape_C::Loc != Location::Acc,
                "MATMULMX output must be an ordinary Tile");
  pto_matmul_detail::matmul_mx(dst, src0, src0_scale, src1,
                               src1_scale, M, N, K);
}


template <typename tile_shape_A,
          typename tile_shape_B, typename tile_shape_BX,
          typename tile_shape_C>
void __vec__ MatMulMxb_Vec_Impl(
    typename tile_shape_C::TileDType __out__ dst,
    const typename tile_shape_A::TileDType __in__ src0,
    const typename tile_shape_B::TileDType __in__ src1,
    const typename tile_shape_BX::TileDType __in__ src1_scale) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  typename tile_shape_C::DType data = 0;

  #pragma clang loop unroll(full)
  for (size_t k = 0; k < tile_shape_A::ValidCol; ++k) {
    size_t idx_0  = index<tile_shape_A>(j, k);
    size_t idx_1  = index<tile_shape_B>(k, i);
    size_t idx_1x = index<tile_shape_BX>(k, i);

    data += blkv_get_tile_ptr(src0)[idx_0] *
            (blkv_get_tile_ptr(src1)[idx_1] *
             blkv_get_tile_ptr(src1_scale)[idx_1x]);
  }

  size_t idx = index<tile_shape_C>(j, i);
  blkv_get_tile_ptr(dst)[idx] = data;
}

// Matrix Multiply MXB:
// C[MxN] = A x scale(B, BX)
template <is_tile_data_v tile_shape_A,
          is_tile_data_v tile_shape_B, is_tile_data_v tile_shape_BX,
          is_tile_data_v tile_shape_C>
void MATMULMXB_Impl(tile_shape_C &dst,
                    tile_shape_A &src0,
                    tile_shape_B &src1, tile_shape_BX &src1_scale) {
  static_assert(tile_shape_A::Cols == tile_shape_B::Rows,
                "Error! Cube A:Columns != Cube B:Rows");

  size_t M = dst.GetValidRow();
  size_t N = dst.GetValidCol();
  size_t K = src0.GetValidCol();

  static_assert(tile_shape_C::Loc != Location::Acc,
                "MATMULMXB output must be an ordinary Tile");
  pto_matmul_detail::matmul_mxb(dst, src0, src1, src1_scale, M, N, K);
}

#endif
