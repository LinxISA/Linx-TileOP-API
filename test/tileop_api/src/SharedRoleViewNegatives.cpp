#include <common/pto_tileop.hpp>
using namespace pto;
using Left = SharedTile<SharedMatrixLeft<float, 16, 16>>;
using Local = CubeTileM16<float, 16, 16>;
using BadLayout = SharedTile<Tile<Location::Left, float, 16, 16, BLayout::ColMajor>>;
#if defined(SHOULD_FAIL_SAME_ROLE)
void test(Left &x) { auto v = reinterpret_shared_tile<Location::Left>(x); (void)v; }
#elif defined(SHOULD_FAIL_LOCAL)
void test(Local &x) { auto v = reinterpret_shared_tile<Location::Right>(x); (void)v; }
#elif defined(SHOULD_FAIL_RVALUE)
void test() { auto v = reinterpret_shared_tile<Location::Right>(Left{}); (void)v; }
#elif defined(SHOULD_FAIL_LAYOUT)
void test(BadLayout &x) { auto v = reinterpret_shared_tile<Location::Right>(x); (void)v; }
#else
int main() { return 0; }
#endif