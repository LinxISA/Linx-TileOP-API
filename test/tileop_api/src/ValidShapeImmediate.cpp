#include <jcore/template_asm.hpp>

using S = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using D = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, -1, -1>;
using SR = SharedTile<S>;
using DR = SharedTile<D>;
using GM = Global<float>;

static void static_path(SR &dst, GM &src) {
  TLOAD(dst, src);
}

static void dynamic_path(DR &dst, GM &src) {
  TLOAD(dst, src);
}

int main() { return 0; }
