#ifndef DEBUG_UIILS_HPP
#define DEBUG_UIILS_HPP

#if defined(__linx) && defined(PTO_ENABLE_TILE_DEBUG)
#include "jcore/utils.hpp"
#elif defined(__ARM_FEATURE_SME)
#include "aarch64/utils.hpp"
#elif defined(__cpu_sim__)
#include "cpu_sim/utils.hpp"
#endif

namespace pto {
#if defined(__linx) && !defined(PTO_ENABLE_TILE_DEBUG)
template <typename tile_shape>
void print_tile(tile_shape &) = delete;
#else
template <typename tile_shape>
void print_tile(tile_shape &tile) {
  print_tile_Impl(tile);
}
#endif

} // namespace pto

#endif
