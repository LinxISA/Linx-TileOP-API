#include <common/linx_group_runtime.h>

extern "C" int __linx_group_worker_main(uint32_t peId, void *context) {
  return context == nullptr ? static_cast<int>(peId) : 0;
}

int call_group_runtime(void *context) { return linx_group_run(context); }
