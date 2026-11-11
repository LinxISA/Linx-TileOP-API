#ifndef PTO_MSGB_REGISTERS_H
#define PTO_MSGB_REGISTERS_H
#include <stdint.h>
#include <stddef.h>
/* Opt-in draft PTO PR #426, d537afa. Not part of the released 0.59 ABI. */
enum {
  PTO_MSGBCR = 0x830, PTO_MSGBD1 = 0x831, PTO_MSGBD10 = 0x83a,
  PTO_MSGBRXCLAIM = 0x83b, PTO_MSGBRXSTATE = 0x83c,
  PTO_MSGBRXTOKEN = 0x83d, PTO_MSGBRXMSGID_HI = 0x83e,
  PTO_MSGBRXMSGID_LO = 0x83f, PTO_MSGBRXCONTROL = 0x840,
  PTO_MSGBRXRELEASE = 0x841, PTO_MSGBACK = 0x842,
  PTO_MSGBACKSTAT = 0x843, PTO_MSGBACKID = 0x844,
  PTO_MSGBTXID_HI = 0x845, PTO_MSGBTXID_LO = 0x846,
  PTO_MSGBTXSTAT = 0x847, PTO_MSGBENDPOINT = 0x848,
  PTO_MSGBCFGSEL = 0x849, PTO_MSGBCFG = 0x84a, PTO_MSGBDOMAIN = 0x84b
};
enum {
  PTO_MSGB_NONE, PTO_MSGB_OK, PTO_MSGB_DUPLICATE, PTO_MSGB_EMPTY,
  PTO_MSGB_BUSY, PTO_MSGB_NO_CLAIM, PTO_MSGB_NOT_OWNER,
  PTO_MSGB_TOKEN_MISMATCH, PTO_MSGB_STALE_EPOCH, PTO_MSGB_BAD_TOKEN,
  PTO_MSGB_NOT_READ, PTO_MSGB_EXHAUSTED, PTO_MSGB_BAD_COMMAND, PTO_MSGB_IO_ERROR
};
#define PTO_MSGB_VALID UINT64_C(0x100)
#define PTO_MSGB_OWNED UINT64_C(0x100)
#define PTO_MSGB_ACKED UINT64_C(0x200)
#define PTO_MSGB_WORDS 10u
static inline int pto_msgb_control_make(uint32_t destination, uint32_t count,
                                        uint64_t *control) {
  if (!control || count > PTO_MSGB_WORDS) return 0;
  *control = (uint64_t)destination | ((uint64_t)count << 32);
  return 1;
}
static inline int pto_msgb_control_valid(uint64_t control) {
  return (control >> 36) == 0 && ((control >> 32) & 15) <= PTO_MSGB_WORDS;
}
static inline int pto_msgb_config_make(uint32_t source_epoch, uint16_t rx_epoch,
                                       uint64_t *config) {
  if (!config || !source_epoch || !rx_epoch) return 0;
  *config = ((uint64_t)source_epoch << 32) | ((uint64_t)rx_epoch << 16) | 1;
  return 1;
}
#endif
