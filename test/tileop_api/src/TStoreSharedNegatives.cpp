// Negative cases for Shared->GM TSTORE / TSTORE.SPART: each must fail to
// compile with a clear static_assert diagnostic (driven by run_negatives.sh).
#include <common/pto_tileop.hpp>

using namespace pto;

using T = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<8, 256>>;
using GM16 = global_tensor<int16_t, RowMajor<8, 256>>;
// ColMajor (non-NORM) source
using TCol = Tile<Location::Vec, float, 8, 256, BLayout::ColMajor>;
// 32B (too small) and 16 KiB (too large) sources
using T32 = Tile<Location::Vec, float, 1, 8, BLayout::RowMajor>;
using T16K = Tile<Location::Vec, float, 64, 64, BLayout::RowMajor>;
using GM16K = global_tensor<float, RowMajor<64, 64>>;

#if defined(SHOULD_FAIL_dtype_full)
void n(GM16 &g, T &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE(g, sh); }
#endif
#if defined(SHOULD_FAIL_dtype_part)
void n(GM16 &g, T &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE_PART<12>(g, sh); }
#endif
#if defined(SHOULD_FAIL_layout_full)
void n(GM &g, TCol &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE(g, sh); }
#endif
#if defined(SHOULD_FAIL_layout_part)
void n(GM &g, TCol &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE_PART<12>(g, sh); }
#endif
#if defined(SHOULD_FAIL_mask0)
void n(GM &g, T &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE_PART<0>(g, sh); }
#endif
#if defined(SHOULD_FAIL_mask16)
void n(GM &g, T &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE_PART<16>(g, sh); }
#endif
#if defined(SHOULD_FAIL_mask3)
void n(GM &g, T &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE_PART<3>(g, sh); }
#endif
#if defined(SHOULD_FAIL_size_small)
void n(GM &g, T32 &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE(g, sh); }
#endif
#if defined(SHOULD_FAIL_size_large)
void n(GM16K &g, T16K &t) { auto sh = TMOV_L2S_INSERT(t); TSTORE_PART<12>(g, sh); }
#endif

void use(void *) {}
int main() { T t; GM g((float *)0x1000); TCol tc; (void)t; (void)g; (void)tc; use(&g); return 0; }
