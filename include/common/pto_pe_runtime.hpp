#ifndef PTO_PE_RUNTIME_HPP
#define PTO_PE_RUNTIME_HPP

#include <stdint.h>

enum {
  PTO_RUNTIME_SINGLE_CONTEXTS = 1,
  PTO_RUNTIME_CORE_PE_CONTEXTS = 4,
};

// Returns the current PE ID (0..3). The compiler lowers this to SSR_GET of
// the read-only THREAD_ID SSR at the PTO ISA specified address 0x0026.
inline uint32_t get_thread_idx()
{
  return __builtin_linx_get_thread_idx();
}

// API convenience alias; this is not a separate architectural operation.
inline uint32_t get_thread_id()
{
  return get_thread_idx();
}

// Launch-contract hook. Normal musl startup returns 1. A dedicated launcher
// must define a strong implementation returning 4 only after it has created
// PE0..PE3 as four independent contexts. An absent hook means one context.
extern "C" unsigned __pto_runtime_pe_context_count(void)
    __attribute__((weak));

#ifndef PTO_TILEOP_SYNC_CORE_VIOLATION
#define PTO_TILEOP_SYNC_CORE_VIOLATION() __builtin_trap()
#endif

// Keep this header consumable by older Linx compilers used for API-surface
// checks. Such compilers know THREAD_ID but do not yet declare the SYNC.CORE
// builtin. A real Linx build may emit the instruction only when the frontend
// advertises that builtin; otherwise the call fails closed through the same
// violation path as an invalid launch contract.
#if defined(__linx) && defined(__clang__)
#if __has_builtin(__builtin_linx_sync_core)
#define PTO_TILEOP_COMPILER_HAS_SYNC_CORE 1
#endif
#endif

#ifndef PTO_TILEOP_COMPILER_HAS_SYNC_CORE
#define PTO_TILEOP_COMPILER_HAS_SYNC_CORE 0
#endif

#ifndef PTO_TILEOP_SYNC_CORE_EMIT
#if PTO_TILEOP_COMPILER_HAS_SYNC_CORE
#define PTO_TILEOP_SYNC_CORE_EMIT() __builtin_linx_sync_core()
#else
#define PTO_TILEOP_SYNC_CORE_EMIT() PTO_TILEOP_SYNC_CORE_VIOLATION()
#endif
#endif

/// Rendezvous PE0..PE3 in the current Core.
///
/// Every PE in the active kernel invocation must reach the same static call
/// in the same dynamic order. PE_MASK does not change the participant set.
/// The instruction is never issued for single-context or invalid-ID launches.
__attribute__((always_inline)) inline void SYNC_CORE()
{
  const unsigned contexts = __pto_runtime_pe_context_count
                                ? __pto_runtime_pe_context_count()
                                : 1;
  const unsigned pe_id = get_thread_idx();
  if (contexts != PTO_RUNTIME_CORE_PE_CONTEXTS ||
      pe_id >= PTO_RUNTIME_CORE_PE_CONTEXTS) {
    PTO_TILEOP_SYNC_CORE_VIOLATION();
    return;
  }
  PTO_TILEOP_SYNC_CORE_EMIT();
}

#endif
