#include <common/pto_tileop.hpp>

#include <cstdint>

#if defined(SHOULD_FAIL_I64_FP64_ELEMENT)
pto::ElementTile<double, 32> rejected;
#elif defined(SHOULD_FAIL_I64_UNSUPPORTED_PARENT)
pto::ElementTile<uint64_t, 2048> rejected;
#elif defined(SHOULD_FAIL_I64_WRITER_LIMIT)
pto::TileArray<pto::ElementTile<uint64_t, 32>, 1, 32> rejected;
#elif defined(SHOULD_FAIL_I64_M16)
pto::CubeTileM16<int64_t, 16, 1> rejected;
#elif defined(SHOULD_FAIL_I64_SHORT_M32)
pto::VecTileM32<uint64_t, 16, 1> rejected;
#elif defined(SHOULD_FAIL_I64_SUBVIEW_PAIR)
void rejected_subview() {
  pto::ElementTile<uint64_t, 32> parent;
  auto rejected = pto::range::subview<1>(parent);
  (void)rejected;
}
#elif defined(SHOULD_FAIL_I64_ASSEMBLE_PAIR)
void rejected_assemble() {
  pto::ElementTile<int64_t, 32> parent;
  auto rejected = pto::range::assemble_init_last<1, 1>(parent);
  (void)rejected;
}
#else
pto::ElementTile<uint64_t, 32> accepted_unsigned;
pto::ElementTile<int64_t, 32> accepted_signed;
pto::CubeTileN8<uint64_t, 2, 8> existing_n8;
pto::VecTileM32<uint64_t, 32, 1, 16, 1> accepted_short_valid_m32;
#endif
