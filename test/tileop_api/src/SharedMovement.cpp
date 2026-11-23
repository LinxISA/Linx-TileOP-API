#include <common/pto_tileop.hpp>

using namespace pto;

using RowMajorTile = Tile<Location::Vec, float, 8, 32>;
using WriterTile = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor, 4, 32>;
using Fragment = Tile<Location::Vec, float, 4, 32>;
using FeatureTile = Tile<Location::Vec, __half, 64, 64>;
using FeatureGM = global_tensor<__half, RowMajor<64, 64>>;

void publish_and_broadcast(RowMajorTile &dst, const RowMajorTile &src) {
  SharedTile<RowMajorTile> shared;
  TMOV<8>(shared, src);
  TMOV(dst, shared);
}

void assemble_and_extract(Fragment &dst, const WriterTile &src,
                          uintptr_t offset_units) {
  SharedTile<RowMajorTile> shared;
  auto writer = range::assemble_init_last<4>(shared, offset_units);
  TMOV<12>(writer, src);
  TMOV<12>(dst, range::shared_nd_subview<4>(shared, offset_units));
}

void timg2col_single_pe(FeatureGM &src, FeatureGM &output,
                       TIMG2COLParams params) {
  SharedTile<FeatureTile> shared;
  TIMG2COL<DN2ND, 0b0010>(shared, src, params);
  TSTORE<0b0010>(output, shared);
}
