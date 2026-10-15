#include <cstdlib>

static unsigned test_context_count;
static unsigned test_pe_id;
static unsigned test_sync_calls;
static unsigned test_violations;

extern "C" unsigned __pto_runtime_pe_context_count(void) {
  return test_context_count;
}

inline unsigned __builtin_linx_get_thread_idx() { return test_pe_id; }

#define PTO_TILEOP_SYNC_CORE_REQUIRE_CONTEXTS 1
#define PTO_TILEOP_SYNC_CORE_VIOLATION() (++test_violations)
#define PTO_TILEOP_SYNC_CORE_EMIT() (++test_sync_calls)
#include <common/pto_pe_runtime.hpp>

static void reset(unsigned contexts, unsigned pe_id) {
  test_context_count = contexts;
  test_pe_id = pe_id;
  test_sync_calls = 0;
  test_violations = 0;
}

int main() {
  reset(1, 0);
  SYNC_CORE();
  if (test_sync_calls != 0 || test_violations != 1)
    return EXIT_FAILURE;

  reset(4, 4);
  SYNC_CORE();
  if (test_sync_calls != 0 || test_violations != 1)
    return EXIT_FAILURE;

  for (unsigned pe_id = 0; pe_id != 4; ++pe_id) {
    reset(4, pe_id);
    SYNC_CORE();
    if (test_sync_calls != 1 || test_violations != 0)
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
