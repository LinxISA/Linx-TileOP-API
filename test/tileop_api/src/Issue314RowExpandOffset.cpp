// Exercise the shared RMode spelling through every row-expand emitter.
#include <jcore/template_asm.hpp>
#include <common/pto_tile_region_inline_asm.hpp>

using namespace pto;

template <typename Matrix, typename Carrier, typename MatrixParent,
          typename CarrierParent, int Offset>
__attribute__((always_inline)) inline void expand_paths(
    Matrix &dst, Matrix &matrix, Carrier &carrier,
    MatrixParent &matrix_parent, CarrierParent &carrier_parent) {
  auto matrices = TPARTVIEW<Matrix, 1, 2>(matrix_parent);
  auto carriers = TPARTVIEW<Carrier, 1, 2>(carrier_parent);
  auto matrix_view = matrices[0][1];
  auto carrier_view = carriers[0][1];
  TROWEXPAND<Matrix, Carrier, Offset>(dst, carrier);
#define CHECK_PATHS(Op)                                                       \
  Op<Matrix, Matrix, Carrier, Offset>(dst, matrix, carrier);                   \
  Op<Matrix, MatrixParent, Matrix, CarrierParent, Carrier, Offset>(            \
      dst, matrix_view, carrier_view);                                       \
  Op<Matrix, MatrixParent, Matrix, Carrier, Offset>(dst, matrix_view, carrier); \
  Op<Matrix, Matrix, CarrierParent, Carrier, Offset>(dst, matrix, carrier_view)
  CHECK_PATHS(TROWEXPANDADD);
  CHECK_PATHS(TROWEXPANDSUB);
  CHECK_PATHS(TROWEXPANDMUL);
  CHECK_PATHS(TROWEXPANDDIV);
  CHECK_PATHS(TROWEXPANDMAX);
  CHECK_PATHS(TROWEXPANDMIN);
  CHECK_PATHS(TROWEXPANDEXPDIF);
#undef CHECK_PATHS
}

#define OFFSET_CASE(Name, Layout, Type, Rows, CellCols, Offset)                \
  extern "C" __attribute__((noinline)) void Name(                            \
      VecTile##Layout<Type, Rows, 32> &dst,                                   \
      VecTile##Layout<Type, Rows, 32> &matrix,                                \
      VecTile##Layout<Type, Rows, CellCols> &carrier,                         \
      VecTile##Layout<Type, Rows, 64> &matrix_parent,                         \
      VecTile##Layout<Type, Rows, 2 * CellCols> &carrier_parent) {             \
    expand_paths<VecTile##Layout<Type, Rows, 32>,                             \
                 VecTile##Layout<Type, Rows, CellCols>,                      \
                 VecTile##Layout<Type, Rows, 64>,                            \
                 VecTile##Layout<Type, Rows, 2 * CellCols>, Offset>(          \
        dst, matrix, carrier, matrix_parent, carrier_parent);                 \
  }

OFFSET_CASE(m32_bf16_0, M32, __bf16, 32, 2, 0)
OFFSET_CASE(m32_bf16_2, M32, __bf16, 32, 2, 2)
OFFSET_CASE(m16_bf16_0, M16, __bf16, 16, 4, 0)
OFFSET_CASE(m16_bf16_2, M16, __bf16, 16, 4, 2)
OFFSET_CASE(m16_bf16_4, M16, __bf16, 16, 4, 4)
OFFSET_CASE(m16_bf16_6, M16, __bf16, 16, 4, 6)
OFFSET_CASE(m32_u8_3, M32, uint8_t, 32, 4, 3)
OFFSET_CASE(m16_u8_7, M16, uint8_t, 16, 8, 7)
#undef OFFSET_CASE
