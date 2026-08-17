// TCMP<TMode>/TCMPS<TMode> compile-time comparison-mode test.
// Verifies each of the six ISA CmpMode values compiles and that the
// deprecated no-mode forms resolve to EQ.
#include <common/pto_tileop.hpp>

using namespace pto;

using Src = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;
using Dst = Tile<Location::Vec, int32_t, 16, 16, BLayout::RowMajor>;

__attribute__((noinline)) void tcmp_modes(Dst &d, Src &a, Src &b) {
  TCMP<CmpMode::EQ>(d, a, b);
  TCMP<CmpMode::NE>(d, a, b);
  TCMP<CmpMode::LT>(d, a, b);
  TCMP<CmpMode::GT>(d, a, b);
  TCMP<CmpMode::LE>(d, a, b);
  TCMP<CmpMode::GE>(d, a, b);
  // deprecated EQ-default forms
  TCMP(d, a, b);
  TCMPS(d, a, 1.0f);
}

__attribute__((noinline)) void tcmps_modes(Dst &d, Src &a) {
  TCMPS<CmpMode::EQ>(d, a, 0.0f);
  TCMPS<CmpMode::NE>(d, a, 0.0f);
  TCMPS<CmpMode::LT>(d, a, 0.0f);
  TCMPS<CmpMode::GT>(d, a, 0.0f);
  TCMPS<CmpMode::LE>(d, a, 0.0f);
  TCMPS<CmpMode::GE>(d, a, 0.0f);
}

void use(void *) {}

int main() {
  Src a;
  Src b;
  Dst d;
  tcmp_modes(d, a, b);
  tcmps_modes(d, a);
  use(&d);
  return 0;
}
