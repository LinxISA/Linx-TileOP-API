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
using NarrowRow = Tile<Location::Vec, float, 2, 1,
                        BLayout::RowMajor, 2, 1>;
static_assert(M16::CubeCellBytes == 128);
static_assert(M16::CubeRequiredBytes == 2048);
static_assert(M16Partial::CubeRequiredBytes == 2048);
static_assert(M16Partial::LogicalTileBytes == 2048);
static_assert(M32::CubeRequiredBytes == 4096);
static_assert(N8::CubeRequiredBytes == 2048);
static_assert(M16S4::CubeElementBits == 4);
static_assert(M16S4::CubeCellCols == 16);
static_assert(NarrowRow::StorageBytes == 128);
static_assert(NarrowRow::LogicalTileBytes == 128);
static_assert(NarrowRow::TilesizeCode == __tilesize_128B);
static_assert(M16::CubeStorageIndex(1, 2) == 34);
static_assert(N8::CubeStorageIndex(5, 9) == 293);
static_assert(SharedMatrixLeft<float, 16, 16>::BFractal == BLayout::RowMajor);
static_assert(SharedMatrixRight<float, 16, 16>::BFractal == BLayout::RowMajor);
using Local64K = Tile<Location::Vec, uint8_t, 256, 256,
                      BLayout::RowMajor>;
using Local128K = Tile<Location::Vec, uint8_t, 512, 256,
                       BLayout::RowMajor>;
using Local256K = Tile<Location::Vec, uint8_t, 1024, 256,
                       BLayout::RowMajor>;
using Shared256K = Tile<Location::Vec, uint8_t, 512, 512,
                        BLayout::RowMajor>;
static_assert(Local64K::TilesizeCode == __tilesize_64KB);
static_assert(Local64K::IsValidActiveSize);
static_assert(Local128K::TilesizeCode == __tilesize_128KB);
static_assert(Local128K::IsValidActiveSize);
static_assert(Local256K::TilesizeCode == __tilesize_256KB);
static_assert(Local256K::IsValidActiveSize);
static_assert(tile_type_traits<typename Shared256K::TileDType>::
                  IsValidSharedActiveSize);
#ifdef __linx
static_assert(sizeof(typename M16::TileDType) == 4096);
static_assert(sizeof(typename M16::TileDType::RegisterType) == 4096);
static_assert(sizeof(typename Local64K::TileDType) == 4096);
static_assert(M16::LogicalTileBytes == 2048);
static_assert(tile_type_traits<typename M16::TileDType>::TilesizeCode ==
              __tilesize_2KB);
static_assert(tile_type_traits<typename Local64K::TileDType>::TilesizeCode ==
              __tilesize_64KB);
static_assert(tile_type_traits<typename Shared256K::TileDType>::TilesizeCode ==
              __tilesize_256KB);
#endif

constexpr FixpAttr transposed = FixpAttr::keep_acc().transpose_a().transpose_b();
static_assert(transposed.TransA && transposed.TransB);
static_assert(transposed.encoding() == 0x000021a3u);
constexpr FixpAttr cscaled = FixpAttr::keep_acc().cscale_enable();
static_assert(cscaled.CScaleEn);
static_assert(cscaled.encoding() == 0x00002223u);
using CScaleTile = Tile<Location::Vec, uint8_t, 32, 32, BLayout::CubeM32,
                        32, 1>;
using CScaleOptions = decltype(
    fixp::keep_acc().cscale(std::declval<CScaleTile &>()));
static_assert(CScaleOptions::Attr.CScaleEn);
static_assert(std::is_same_v<typename CScaleOptions::CScaleTile, CScaleTile>);
constexpr auto transposed_options =
    fixp::keep_acc().transpose_a().transpose_b();
static_assert(transposed_options.Attr.TransA && transposed_options.Attr.TransB);

using SignedA = CubeTileM16<int8_t, 16, 32>;
using SignedB = CubeTileN8<int16_t, 32, 16>;
using SignedD = CubeAccumulatorM16<int32_t, 16, 16>;
using UnsignedA = CubeTileM16<uint8_t, 16, 32>;
using UnsignedB = CubeTileN8<uint16_t, 32, 16>;
using UnsignedD = CubeAccumulatorM16<uint32_t, 16, 16>;
using FloatA = CubeTileM16<__half, 16, 32>;
using FloatB = CubeTileN8<__bf16, 32, 16>;
using FloatD = CubeAccumulatorM16<float, 16, 16>;

static_assert(matrix_input_pair_legal<SignedA, SignedB>());
static_assert(matrix_input_pair_legal<UnsignedA, UnsignedB>());
static_assert(matrix_input_pair_legal<FloatA, FloatB>());
static_assert(!matrix_input_pair_legal<SignedA, UnsignedB>());
static_assert(matrix_accumulator_type_legal<SignedA, SignedB, SignedD>());
static_assert(matrix_accumulator_type_legal<UnsignedA, UnsignedB, UnsignedD>());
static_assert(matrix_accumulator_type_legal<FloatA, FloatB, FloatD>());
static_assert(matrix_output_type_legal<FixpAttr::keep_acc(), SignedA, SignedB,
                                       SignedD>());
static_assert(matrix_output_type_legal<FixpAttr::keep_acc(), UnsignedA,
                                       UnsignedB, UnsignedD>());
static_assert(matrix_output_type_legal<FixpAttr::keep_acc(), FloatA, FloatB,
                                       FloatD>());

static_assert(matrix_mx_input_supported(__type_fp16));
static_assert(matrix_mx_input_supported(__type_bf16));
static_assert(matrix_mx_input_supported(__type_fp8_e4m3));
static_assert(matrix_mx_input_supported(__type_fp8_e5m2));
static_assert(matrix_mx_input_supported(__type_fp4_e2m1x2));
static_assert(matrix_mx_input_supported(__type_fp4_e1m2x2));
static_assert(!matrix_mx_input_needs_scale(__type_fp16));
static_assert(!matrix_mx_input_needs_scale(__type_bf16));
static_assert(matrix_mx_input_needs_scale(__type_fp8_e4m3));
static_assert(matrix_mx_input_needs_scale(__type_fp8_e5m2));
static_assert(matrix_mx_input_needs_scale(__type_fp4_e2m1x2));
static_assert(matrix_mx_input_needs_scale(__type_fp4_e1m2x2));

int main() { return 0; }
