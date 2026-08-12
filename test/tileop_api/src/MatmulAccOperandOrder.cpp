#include <common/pto_tileop.hpp>

using namespace pto;

using D = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using A = TileLeft<float, 32, 64>;
using B = TileRight<float, 64, 32>;

void matmul_acc_operand_order(D &d, D &c, A &a, B &b) {
  TMATMUL_ACC(d, c, a, b);
}

int main() { return 0; }
