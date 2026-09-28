#include <common/pto_tileop.hpp>
using namespace pto;
using F16 = Tile<Location::Vec, __half, 16, 16>;
using F32 = Tile<Location::Vec, float, 16, 16>;
using I32 = Tile<Location::Vec, int32_t, 16, 16>;
using N8 = CubeTileN8<float, 16, 16>;
using M16 = VecTileM16<float, 16, 16>;
using BadShape = Tile<Location::Vec, float, 8, 16>;

#ifdef SHOULD_FAIL_TEXPDIF_TYPE
void bad(F32 &d, I32 &a, I32 &b) { TEXPDIF(d, a, b); }
#elif defined(SHOULD_FAIL_TEXPDIF_NARROW)
void bad(F16 &d, F32 &a, F32 &b) { TEXPDIF(d, a, b); }
#elif defined(SHOULD_FAIL_TEXPDIF_N8)
void bad(N8 &d, N8 &a, N8 &b) { TEXPDIF(d, a, b); }
#elif defined(SHOULD_FAIL_TEXPDIF_MIXED_LAYOUT)
void bad(F32 &d, F32 &a, M16 &b) { TEXPDIF(d, a, b); }
#elif defined(SHOULD_FAIL_TEXPDIF_SHAPE)
void bad(F32 &d, F32 &a, BadShape &b) { TEXPDIF(d, a, b); }
#endif

int main() { return 0; }