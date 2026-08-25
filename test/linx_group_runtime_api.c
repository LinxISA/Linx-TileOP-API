#include <common/linx_group_runtime.h>

int __linx_group_worker_main(uint32_t pe_id, void *context) {
  return context == 0 ? (int)pe_id : 0;
}

int call_group_runtime_c(void *context) { return linx_group_run(context); }
