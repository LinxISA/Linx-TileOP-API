#ifndef LINX_GROUP_RUNTIME_H
#define LINX_GROUP_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One static ELF provides one group worker body. A direct symbol avoids a
 * function-pointer relocation in the first gfrun ABI. */
int __linx_group_worker_main(uint32_t pe_id, void *context);

/* Run the ELF's worker body on the fixed four-PE group. PE0 calls this after
 * hosted libc and input initialization; PE1..PE3 are already parked in the
 * entry exported below. The first non-zero PE status is returned. */
int linx_group_run(void *context);

/* Static hosted ELF ABI symbol consumed by gfrun. This entry never enters
 * libc and never returns. */
__attribute__((noreturn, used, visibility("default"))) void
__linx_group_worker_start(void);

#ifdef __cplusplus
}
#endif

#endif
