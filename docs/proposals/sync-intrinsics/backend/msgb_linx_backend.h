#ifndef PTO_MSGB_LINX_BACKEND_H
#define PTO_MSGB_LINX_BACKEND_H
#include "msgb_endpoint.h"
/* Including the portable headers never claims target support. */
#if defined(__clang__) && defined(__linx)
#if __has_builtin(__builtin_linx_msgb_read) && \
    __has_builtin(__builtin_linx_msgb_write) && \
    __has_builtin(__builtin_linx_msgb_send)
#define PTO_MSGB_COMPILER_AVAILABLE 1
/* Successful return means the instruction retired. A precise hardware fault
 * does not return a fabricated protocol status: the platform resumes/traps it.
 * The compiler emits a fresh SYS occurrence for each new call to send. */
static inline pto_msgb_io_status pto_msgb_linx_read(void *ctx, uint32_t address,
                                                   uint64_t *value) {
  (void)ctx;
  switch (address) {
  case 0x830: *value = __builtin_linx_msgb_read(0x830); return PTO_MSGB_IO_OK;
  case 0x831: *value = __builtin_linx_msgb_read(0x831); return PTO_MSGB_IO_OK;
  case 0x832: *value = __builtin_linx_msgb_read(0x832); return PTO_MSGB_IO_OK;
  case 0x833: *value = __builtin_linx_msgb_read(0x833); return PTO_MSGB_IO_OK;
  case 0x834: *value = __builtin_linx_msgb_read(0x834); return PTO_MSGB_IO_OK;
  case 0x835: *value = __builtin_linx_msgb_read(0x835); return PTO_MSGB_IO_OK;
  case 0x836: *value = __builtin_linx_msgb_read(0x836); return PTO_MSGB_IO_OK;
  case 0x837: *value = __builtin_linx_msgb_read(0x837); return PTO_MSGB_IO_OK;
  case 0x838: *value = __builtin_linx_msgb_read(0x838); return PTO_MSGB_IO_OK;
  case 0x839: *value = __builtin_linx_msgb_read(0x839); return PTO_MSGB_IO_OK;
  case 0x83a: *value = __builtin_linx_msgb_read(0x83a); return PTO_MSGB_IO_OK;
  case 0x83c: *value = __builtin_linx_msgb_read(0x83c); return PTO_MSGB_IO_OK;
  case 0x83d: *value = __builtin_linx_msgb_read(0x83d); return PTO_MSGB_IO_OK;
  case 0x83e: *value = __builtin_linx_msgb_read(0x83e); return PTO_MSGB_IO_OK;
  case 0x83f: *value = __builtin_linx_msgb_read(0x83f); return PTO_MSGB_IO_OK;
  case 0x840: *value = __builtin_linx_msgb_read(0x840); return PTO_MSGB_IO_OK;
  case 0x843: *value = __builtin_linx_msgb_read(0x843); return PTO_MSGB_IO_OK;
  case 0x844: *value = __builtin_linx_msgb_read(0x844); return PTO_MSGB_IO_OK;
  case 0x845: *value = __builtin_linx_msgb_read(0x845); return PTO_MSGB_IO_OK;
  case 0x846: *value = __builtin_linx_msgb_read(0x846); return PTO_MSGB_IO_OK;
  case 0x847: *value = __builtin_linx_msgb_read(0x847); return PTO_MSGB_IO_OK;
  case 0x848: *value = __builtin_linx_msgb_read(0x848); return PTO_MSGB_IO_OK;
  case 0x849: *value = __builtin_linx_msgb_read(0x849); return PTO_MSGB_IO_OK;
  case 0x84a: *value = __builtin_linx_msgb_read(0x84a); return PTO_MSGB_IO_OK;
  case 0x84b: *value = __builtin_linx_msgb_read(0x84b); return PTO_MSGB_IO_OK;
  default: return PTO_MSGB_IO_REJECTED;
  }
}
static inline pto_msgb_io_status pto_msgb_linx_write(void *ctx, uint32_t address,
                                                    uint64_t value) {
  (void)ctx;
  switch (address) {
  case 0x831: __builtin_linx_msgb_write(0x831, value); return PTO_MSGB_IO_OK;
  case 0x832: __builtin_linx_msgb_write(0x832, value); return PTO_MSGB_IO_OK;
  case 0x833: __builtin_linx_msgb_write(0x833, value); return PTO_MSGB_IO_OK;
  case 0x834: __builtin_linx_msgb_write(0x834, value); return PTO_MSGB_IO_OK;
  case 0x835: __builtin_linx_msgb_write(0x835, value); return PTO_MSGB_IO_OK;
  case 0x836: __builtin_linx_msgb_write(0x836, value); return PTO_MSGB_IO_OK;
  case 0x837: __builtin_linx_msgb_write(0x837, value); return PTO_MSGB_IO_OK;
  case 0x838: __builtin_linx_msgb_write(0x838, value); return PTO_MSGB_IO_OK;
  case 0x839: __builtin_linx_msgb_write(0x839, value); return PTO_MSGB_IO_OK;
  case 0x83a: __builtin_linx_msgb_write(0x83a, value); return PTO_MSGB_IO_OK;
  case 0x83b: __builtin_linx_msgb_write(0x83b, value); return PTO_MSGB_IO_OK;
  case 0x841: __builtin_linx_msgb_write(0x841, value); return PTO_MSGB_IO_OK;
  case 0x842: __builtin_linx_msgb_write(0x842, value); return PTO_MSGB_IO_OK;
  case 0x849: __builtin_linx_msgb_write(0x849, value); return PTO_MSGB_IO_OK;
  case 0x84a: __builtin_linx_msgb_write(0x84a, value); return PTO_MSGB_IO_OK;
  case 0x84b: __builtin_linx_msgb_write(0x84b, value); return PTO_MSGB_IO_OK;
  default: return PTO_MSGB_IO_REJECTED;
  }
}
static inline pto_msgb_io_status pto_msgb_linx_send(void *ctx, uint64_t control) {
  (void)ctx; __builtin_linx_msgb_send(control); return PTO_MSGB_IO_OK;
}
static inline pto_msgb_io pto_msgb_linx_io(void) {
  pto_msgb_io io = {NULL, pto_msgb_linx_read, pto_msgb_linx_write,
                     pto_msgb_linx_send};
  return io;
}
#endif
#endif
#ifndef PTO_MSGB_COMPILER_AVAILABLE
#define PTO_MSGB_COMPILER_AVAILABLE 0
#endif
/* Manager-only, after kernel launch on each target PE. This function does not
 * choose epochs. Configuration faults remain platform exceptions. */
static inline int pto_msgb_configure(pto_msgb_io io, uint32_t acr,
                                     uint64_t domain, uint32_t source_epoch,
                                     uint16_t rx_epoch) {
  uint64_t config, actual;
  if (!pto_msgb_io_valid(io) || acr > 15 ||
      !pto_msgb_config_make(source_epoch, rx_epoch, &config)) return 0;
  if (io.write(io.context, PTO_MSGBCFGSEL, acr) != PTO_MSGB_IO_OK ||
      io.write(io.context, PTO_MSGBDOMAIN, domain) != PTO_MSGB_IO_OK ||
      io.write(io.context, PTO_MSGBCFG, config) != PTO_MSGB_IO_OK ||
      io.read(io.context, PTO_MSGBCFG, &actual) != PTO_MSGB_IO_OK)
    return 0;
  return actual == config;
}
#endif
