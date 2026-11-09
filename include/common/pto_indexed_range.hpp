#ifndef PTO_INDEXED_RANGE_HPP
#define PTO_INDEXED_RANGE_HPP

#include <cstdint>

#if defined(__linx_indexed_tlsu_range) && __linx_indexed_tlsu_range == 1
#define PTO_INDEXED_RANGE_CONSTRAINT "R"
#define PTO_TILEOP_COMPILER_HAS_INDEXED_RANGE 1
#else
// Keep ordinary headers parseable on preceding compilers. Instantiation of a
// range overload is rejected by require_indexed_range_compiler below.
#define PTO_INDEXED_RANGE_CONSTRAINT "r"
#define PTO_TILEOP_COMPILER_HAS_INDEXED_RANGE 0
#endif

namespace pto {
namespace detail {
template <typename Operand>
constexpr void require_indexed_range_compiler() {
  static_assert(PTO_TILEOP_COMPILER_HAS_INDEXED_RANGE || sizeof(Operand) == 0,
                "IndexedByteRange requires LLVM b19e8c0e184c range support");
}
} // namespace detail
// PTO-ISA PR #424 at 1981bceffa82. Bounds are inclusive signed byte
// displacements of access starts, relative to the operation's GM base.
// Software certifies coverage and a nonwrapping address envelope. This is
// not an element count, row stride, or runtime bounds check.
struct IndexedByteRange {
  int64_t min;
  int64_t max;
};
} // namespace pto

#endif
