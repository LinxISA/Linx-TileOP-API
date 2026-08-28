#ifndef PTO_TILEOP_HPP
#define PTO_TILEOP_HPP

#include "common/pto_tile.hpp"
#include "common/pto_tile_region.hpp"
#include "common/tileop_api.hpp"
#include "common/global_iterator.hpp"
#include "common/tile_tensor_impl.hpp"
#include "common/debug_utils.hpp"

#ifdef __linx
#include "common/pto_tile_region_inline_asm.hpp"
#endif

// Returns the current PE ID (0..3).
// Aligned with website manual get_thread_idx(). Lowered to SSR_GET reading
// the read-only PEID SSR (0x0802).
inline uint32_t get_thread_idx() {
  return __builtin_linx_get_thread_idx();
}

// API convenience alias; this is not a separate architectural operation.
inline uint32_t get_thread_id() {
  return get_thread_idx();
}

#endif
