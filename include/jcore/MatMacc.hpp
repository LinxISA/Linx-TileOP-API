#ifndef MATMACC_HPP
#define MATMACC_HPP

#include "common/pto_tile.hpp"
#include "jcore/template_asm.hpp"

using namespace pto;

template <typename tile_shape_A, typename tile_shape_B, typename tile_shape_C>
void __vec__ MatMacc_Vec_Impl(
    typename tile_shape_C::TileDType __out__ dst,
    const typename tile_shape_A::TileDType __in__ src0,
    const typename tile_shape_B::TileDType __in__ src1,
    const typename tile_shape_C::TileDType __in__ acc) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();
  typename tile_shape_C::DType data = 
    blkv_get_tile_ptr(acc)[index<tile_shape_C>(j, i)];

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

// Matrix Multiply and Accumulate: C[MxN] += A[M×K] x B[KxN]
template <is_tile_data_v tile_shape_A, is_tile_data_v tile_shape_B,
          is_tile_data_v tile_shape_C>
void MATMACC_Impl(tile_shape_C &dst, tile_shape_A &src0, tile_shape_B &src1) {
  static_assert(tile_shape_A::Cols == tile_shape_B::Rows,
                "MATMACC requires A.Cols == B.Rows");
  size_t M = dst.GetValidRow();
  size_t N = dst.GetValidCol();
  size_t K = src0.GetValidCol();
  static_assert(tile_shape_C::Loc != Location::Acc,
                "MATMACC output must be an ordinary Tile");
  pto_matmul_detail::matmul_acc(dst, src0, src1, dst, M, N, K);
}

template <typename tile_shape_A, typename tile_shape_AX,
          typename tile_shape_B, typename tile_shape_BX,
          typename tile_shape_C>
void __vec__ MatMaccMx_Vec_Impl(
    typename tile_shape_C::TileDType __out__ dst,
    const typename tile_shape_A::TileDType __in__ src0,
    const typename tile_shape_AX::TileDType __in__ src0x,
    const typename tile_shape_B::TileDType __in__ src1,
    const typename tile_shape_BX::TileDType __in__ src1x,
    const typename tile_shape_C::TileDType __in__ acc) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  typename tile_shape_C::DType data =
      blkv_get_tile_ptr(acc)[index<tile_shape_C>(j, i)];

  #pragma clang loop unroll(full)
  for (size_t k = 0; k < tile_shape_A::ValidCol; ++k) {
    size_t idx_0  = index<tile_shape_A>(j, k);
    size_t idx_0x = index<tile_shape_AX>(j, k);
    size_t idx_1  = index<tile_shape_B>(k, i);
    size_t idx_1x = index<tile_shape_BX>(k, i);

    data += (blkv_get_tile_ptr(src0)[idx_0] * blkv_get_tile_ptr(src0x)[idx_0x]) *
            (blkv_get_tile_ptr(src1)[idx_1] * blkv_get_tile_ptr(src1x)[idx_1x]);
  }

  size_t idx = index<tile_shape_C>(j, i);
  blkv_get_tile_ptr(dst)[idx] = data;
}

// Matrix Multiply MX and Accumulate:
// C[MxN] += scale(A, AX) x scale(B, BX)
template <is_tile_data_v tile_shape_A,  is_tile_data_v tile_shape_AX,
          is_tile_data_v tile_shape_B,  is_tile_data_v tile_shape_BX,
          is_tile_data_v tile_shape_C>
void MATMACCMX_Impl(tile_shape_C &dst,
                    tile_shape_A &src0,  tile_shape_AX &src0x,
                    tile_shape_B &src1,  tile_shape_BX &src1x) {
  static_assert(tile_shape_A::Cols == tile_shape_B::Rows,
                "Error! Cude A:Columns != Cude B:Rows");

  size_t M = dst.GetValidRow();
  size_t N = dst.GetValidCol();
  size_t K = src0.GetValidCol();

  static_assert(tile_shape_C::Loc != Location::Acc,
                "MATMACCMX output must be an ordinary Tile");
  pto_matmul_detail::matmul_mx_acc(dst, src0, src0x, src1, src1x,
                                   dst, M, N, K);
}

template <typename tile_shape_A,
          typename tile_shape_B, typename tile_shape_BX,
          typename tile_shape_C>
void __vec__ MatMaccMxb_Vec_Impl(
    typename tile_shape_C::TileDType __out__ dst,
    const typename tile_shape_A::TileDType __in__ src0,
    const typename tile_shape_B::TileDType __in__ src1,
    const typename tile_shape_BX::TileDType __in__ src1x,
    const typename tile_shape_C::TileDType __in__ acc) {
  size_t i = blkv_get_index_x();
  size_t j = blkv_get_index_y();

  typename tile_shape_C::DType data =
      blkv_get_tile_ptr(acc)[index<tile_shape_C>(j, i)];

  #pragma clang loop unroll(full)
  for (size_t k = 0; k < tile_shape_A::ValidCol; ++k) {
    size_t idx_0  = index<tile_shape_A>(j, k);
    size_t idx_1  = index<tile_shape_B>(k, i);
    size_t idx_1x = index<tile_shape_BX>(k, i);


    data += blkv_get_tile_ptr(src0)[idx_0] *
            (blkv_get_tile_ptr(src1)[idx_1] * blkv_get_tile_ptr(src1x)[idx_1x]);
  }

  size_t idx = index<tile_shape_C>(j, i);
  blkv_get_tile_ptr(dst)[idx] = data;
}

// Matrix Multiply MXB and Accumulate:
// C[MxN] += A x scale(B, BX)
template <is_tile_data_v tile_shape_A,
          is_tile_data_v tile_shape_B,  is_tile_data_v tile_shape_BX,
          is_tile_data_v tile_shape_C>
void MATMACCMXB_Impl(tile_shape_C &dst,
                     tile_shape_A &src0,
                     tile_shape_B &src1, tile_shape_BX &src1x) {
  static_assert(tile_shape_A::Cols == tile_shape_B::Rows,
                "Error! Cude A:Columns != Cude B:Rows");

  size_t M = dst.GetValidRow();
  size_t N = dst.GetValidCol();
  size_t K = src0.GetValidCol();

  static_assert(tile_shape_C::Loc != Location::Acc,
                "MATMACCMXB output must be an ordinary Tile");
  pto_matmul_detail::matmul_mxb_acc(dst, src0, src1, src1x,
                                    dst, M, N, K);
}

#endif
