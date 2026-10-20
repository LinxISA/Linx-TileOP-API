#ifndef PTO_PE_RUNTIME_HPP
#define PTO_PE_RUNTIME_HPP

// Public, compiler-facing part of the core PE rendezvous contract.  The
// launcher owns context creation and the PE stacks; this header deliberately
// does not describe a ready/done mailbox or an exit ABI.

#include <stdint.h>

// The launcher passes the same entry address and descriptor to all
// participating PEs. PE-local stack/TLS setup is complete before entry. This
// data-only ABI is shared by compiler-rt, musl and bare launchers without
// re-entering libc on secondary PEs.
struct pto_runtime_group_context {
  uint32_t magic;
  uint16_t version;
  uint16_t participant_count;
  // Both point to four-element arrays indexed by architectural PE id.
  volatile uint32_t *state;
  volatile int32_t *result;
  volatile uint32_t *error;
};

enum : uint32_t {
  PTO_RUNTIME_GROUP_CONTEXT_MAGIC = 0x50544f47u, // "PTOG"
  PTO_RUNTIME_GROUP_CONTEXT_VERSION = 1u,
  PTO_RUNTIME_GROUP_STATE_READY = 0u,
  PTO_RUNTIME_GROUP_STATE_RUNNING = 1u,
  PTO_RUNTIME_GROUP_STATE_DONE = 2u,
  PTO_RUNTIME_GROUP_STATE_ABORT = 3u,
};

extern "C" void __linx_group_entry(const pto_runtime_group_context *)
    __attribute__((weak, noreturn));
extern "C" int __linx_group_pe_main(
    unsigned, const pto_runtime_group_context *) __attribute__((weak));
extern "C" void __pto_runtime_group_complete(
    const pto_runtime_group_context *, int) __attribute__((weak, noreturn));
extern "C" void __linx_group_worker_start(void)
    __attribute__((weak, noreturn));

// PTO ISA SystemRegister_THREAD_ID is SSR address 0x0026.  This is a
// read-only architectural value; it is not the retired PEID address used by
// older experimental launchers.
inline uint32_t get_thread_idx() { return __builtin_linx_get_thread_idx(); }

inline uint32_t get_thread_id() { return get_thread_idx(); }

#ifndef PTO_TILEOP_SYNC_CORE_VIOLATION
#define PTO_TILEOP_SYNC_CORE_VIOLATION() __builtin_trap()
#endif

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

// Define this to 1 in a platform-specific launcher build when that launcher
// publishes the optional context-count hook.  It is intentionally opt-in:
// PR #957 does not standardise a guest context-count ABI.
#ifndef PTO_TILEOP_SYNC_CORE_REQUIRE_CONTEXTS
#define PTO_TILEOP_SYNC_CORE_REQUIRE_CONTEXTS 0
#endif

#if PTO_TILEOP_SYNC_CORE_REQUIRE_CONTEXTS
extern "C" unsigned __pto_runtime_pe_context_count(void)
    __attribute__((weak));
#endif

/// Rendezvous all PEs in the current core at this static site.
///
/// The participant set and release semantics are implemented by SYNC.CORE in
/// the platform.  Every participating PE must execute the same site in the
/// same dynamic order.  No hosted libc entry point is re-entered here.
__attribute__((always_inline)) inline void SYNC_CORE() {
#if PTO_TILEOP_SYNC_CORE_REQUIRE_CONTEXTS
  const unsigned contexts = __pto_runtime_pe_context_count
                                ? __pto_runtime_pe_context_count()
                                : 0;
  if (contexts != 4 || get_thread_idx() >= 4) {
    PTO_TILEOP_SYNC_CORE_VIOLATION();
    return;
  }
#endif
  PTO_TILEOP_SYNC_CORE_EMIT();
}

#endif
