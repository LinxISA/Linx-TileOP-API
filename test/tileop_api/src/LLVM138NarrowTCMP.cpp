#include <common/pto_tileop.hpp>

using namespace pto;

template <int Rows, int ValidCol = DYNAMIC, typename Scalar = int32_t>
using Narrow = Tile<Location::Vec, Scalar, Rows, 1,
                    BLayout::RowMajor, DYNAMIC, ValidCol>;

extern "C" void dynamic32(Narrow<32> &dst, size_t rows) {
  Narrow<32> left(rows, 1), right(rows, 1);
  TEXPANDS(left, 3);
  TEXPANDS(right, 2);
  TCMP<CmpMode::GT>(dst, left, right);
}

extern "C" void static32(Narrow<32, 1> &dst, size_t rows) {
  Narrow<32, 1> left(rows), right(rows);
  TEXPANDS(left, 3);
  TEXPANDS(right, 2);
  TCMP<CmpMode::GT>(dst, left, right);
}

extern "C" void dynamic64(Narrow<64> &dst, size_t rows) {
  Narrow<64> left(rows, 1), right(rows, 1);
  TEXPANDS(left, 3);
  TEXPANDS(right, 2);
  TCMP<CmpMode::GT>(dst, left, right);
}

extern "C" void static64(Narrow<64, 1> &dst, size_t rows) {
  Narrow<64, 1> left(rows), right(rows);
  TEXPANDS(left, 3);
  TEXPANDS(right, 2);
  TCMP<CmpMode::GT>(dst, left, right);
}

extern "C" void cross_type32(Narrow<32> &dst, size_t rows) {
  Narrow<32> left(rows, 1);
  Narrow<32, DYNAMIC, uint32_t> right(rows, 1);
  TEXPANDS(left, 3);
  TEXPANDS(right, 2u);
  TCMP<CmpMode::GT>(dst, left, right);
}

using Matrix = Tile<Location::Vec, int32_t, 4, 16>;

extern "C" void static_modes(Matrix &dst, Matrix &left, Matrix &right) {
  TCMP<CmpMode::EQ>(dst, left, right);
  TCMP<CmpMode::NE>(dst, left, right);
  TCMP<CmpMode::LT>(dst, left, right);
  TCMP<CmpMode::GT>(dst, left, right);
  TCMP<CmpMode::LE>(dst, left, right);
  TCMP<CmpMode::GE>(dst, left, right);
}
