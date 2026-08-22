#include "common/pto_tile.hpp"

using namespace pto;

static_assert(is_valid_pe_mask(0));
static_assert(is_valid_pe_mask(1));
static_assert(is_valid_pe_mask(2));
static_assert(is_valid_pe_mask(4));
static_assert(is_valid_pe_mask(8));
static_assert(is_valid_pe_mask(12));
static_assert(is_valid_pe_mask(14));
static_assert(is_valid_pe_mask(15));
static_assert(!is_valid_pe_mask(3));
static_assert(!is_valid_pe_mask(5));
static_assert(pe_mode_from_mask(15) == 7);

static_assert(static_cast<unsigned>(LayoutCvtEnum::ND2M32) == 21);
static_assert(static_cast<unsigned>(LayoutCvtEnum::ND2M16) == 22);
static_assert(static_cast<unsigned>(LayoutCvtEnum::ND2N8) == 23);
static_assert(static_cast<unsigned>(LayoutCvtEnum::M322ND) == 24);
static_assert(static_cast<unsigned>(LayoutCvtEnum::M162ND) == 25);
static_assert(static_cast<unsigned>(LayoutCvtEnum::N82ND) == 26);

using M16 = CubeTileM16<float, 16, 32>;
using M16Partial = CubeTileM16<float, 8, 32>;
using M32 = CubeTileM32<float, 32, 32>;
using N8 = CubeTileN8<float, 32, 16>;
using M16S4 = CubeTileM16<__int4x2, 16, 32>;
static_assert(M16::CubeCellBytes == 128);
static_assert(M16::CubeRequiredBytes == 2048);
static_assert(M16Partial::CubeRequiredBytes == 2048);
static_assert(M16Partial::LogicalTileBytes == 2048);
static_assert(M32::CubeRequiredBytes == 4096);
static_assert(N8::CubeRequiredBytes == 2048);
static_assert(M16S4::CubeElementBits == 4);
static_assert(M16S4::CubeCellCols == 16);
static_assert(M16::CubeStorageIndex(1, 2) == 34);
static_assert(N8::CubeStorageIndex(5, 9) == 293);
static_assert(SharedMatrixLeft<float, 16, 16>::BFractal == BLayout::RowMajor);
static_assert(SharedMatrixRight<float, 16, 16>::BFractal == BLayout::RowMajor);

constexpr FixpAttr transposed = FixpAttr::keep_acc().transpose_a().transpose_b();
static_assert(transposed.TransA && transposed.TransB);
static_assert(transposed.encoding() == 0x000021a3u);
constexpr auto transposed_options =
    fixp::keep_acc().transpose_a().transpose_b();
static_assert(transposed_options.Attr.TransA && transposed_options.Attr.TransB);

int main() { return 0; }
