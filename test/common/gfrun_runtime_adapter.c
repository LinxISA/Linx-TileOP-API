// Explicit SSM.PTO v1 platform adapter for repository benchmarks.
// The compiler-rt group worker owns synchronization and result publication;
// this file owns only model admission, the main() callback, and completion.

#include <stdint.h>

struct pto_runtime_group_context;

extern int main(void);
extern void __linx_group_worker_start(void) __attribute__((noreturn));

// Writable slot populated by the gfrun launcher before any PE executes.
volatile uintptr_t __ssm_pto_group_context;

__attribute__((used, section(".note.ssm.pto"), aligned(4)))
static const uint32_t ssm_hosted_note[] = {
    8, 16, 0x50544f47, 0x2e4d5353, 0x004f5450,
    1, 4, 32, 0,
};

const struct pto_runtime_group_context *
__pto_runtime_group_context_get(void) {
  return (const struct pto_runtime_group_context *)__ssm_pto_group_context;
}

int __linx_group_pe_main(unsigned pe,
                         const struct pto_runtime_group_context *context) {
  (void)pe;
  (void)context;
  return main();
}

__attribute__((noreturn)) void __pto_runtime_group_complete(
    const struct pto_runtime_group_context *context, int status) {
  const uintptr_t service = 0x50544f01u;
  __asm__ volatile(
      "BSTART.SYS FALL\n"
      "ori %0, 0, ->a0\n"
      "ori %1, 0, ->a1\n"
      "ori %2, 0, ->x1\n"
      "acrc 0\n"
      "BSTOP\n"
      :
      : "r"(context), "r"((intptr_t)status), "r"(service)
      : "a0", "a1", "x1", "memory");
  __builtin_unreachable();
}

__attribute__((noreturn)) void _start(void) {
  __linx_group_worker_start();
}
