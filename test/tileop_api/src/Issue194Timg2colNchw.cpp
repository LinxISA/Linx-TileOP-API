// Issue #194: TIMG2COL NCHW support. The direct-Local forms are pinned to
// the ND (NHWC) GM addressing by the schema's ND2M16/ND2M32 layouts. NCHW
// sources consume the SharedND output: B.DATR DN2ND selects the
// channel-major GM index (BundleTIMG2COLGMIndexDN), NORM keeps the ND index
// while publishing through Shared. A single-issuer variant drops the
// four-PE requirement and publishes the complete parent with one B.IOS.
#include <common/pto_tileop.hpp>

using namespace pto;

using SharedND = SharedTile<Tile<Location::Vec, __half, 64, 64,
                                     BLayout::RowMajor, 64, 64>>;
using FeatureGM = global_tensor<__half, RowMajor<64, 64>>;

__attribute__((noinline)) void timg2col_nchw_shared(SharedND &dst,
                                                    FeatureGM &src,
                                                    TIMG2COLParams params) {
  // B.DATR DN2ND -> channel*H*W + spatial GM addressing (NCHW source).
  TIMG2COL<DN2ND>(dst, src, params);
}

__attribute__((noinline)) void timg2col_nhwc_shared(SharedND &dst,
                                                    FeatureGM &src,
                                                    TIMG2COLParams params) {
  // B.DATR NORM -> spatial*Cin + channel GM addressing (NHWC source).
  TIMG2COL(dst, src, params);
}

__attribute__((noinline)) void timg2col_nchw_spart(SharedND &dst,
                                                   FeatureGM &src,
                                                   TIMG2COLParams params) {
  // Single-issuer NCHW materialization: one B.IOS, no B.ASSEMBLE protocol.
  TIMG2COL_SPART<DN2ND>(dst, src, params, 1);
}
