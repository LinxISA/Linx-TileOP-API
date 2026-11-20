#define PTO_RUNTIME_INTRA
#include "common/pto_runtime.h"

// Test inputs are fixed GM state, not preloaded physical Tile/register state.
volatile unsigned startup_report[40];
__thread volatile unsigned startup_private = 0x52;

__attribute__((constructor)) static void startup_process_constructor(void)
{
    ++startup_report[0];
    startup_report[2] = 0x12340000;
}
int __pto_runtime_invocation_initialize(const struct pto_runtime_group_context *context)
{
    (void)context;
    ++startup_report[1];
    startup_report[3] = startup_report[2] + startup_report[1];
    if (startup_report[4]) return -5;
    return 0;
}
int main(void)
{
    const unsigned pe = __builtin_linx_get_thread_idx();
    volatile unsigned private_stack = 0x100 + pe;
    startup_report[8 + pe] = __builtin_linx_get_thread_idx();
    startup_report[12 + pe] = startup_report[3] + pe;
    startup_private += pe + 1;
    startup_report[16 + pe] = startup_private;
    startup_report[20 + pe] = private_stack;
    const uintptr_t address = (uintptr_t)&private_stack;
    startup_report[24 + 2 * pe] = (unsigned)address;
    startup_report[25 + 2 * pe] = (unsigned)(address >> 32);
    ++startup_report[32 + pe];
    return 0;
}
