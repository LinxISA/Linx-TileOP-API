#ifndef PTO_SYNC_RUNTIME_H
#define PTO_SYNC_RUNTIME_H
#include "../sync_intrinsics.h"
#include "../backend/msgb_endpoint.h"
#ifdef __cplusplus
extern "C" {
#endif
#define PTO_SYNC_SLOTS 32
#define PTO_SYNC_OBJECTS 8
#define PTO_SYNC_MEMBERS 8
#define PTO_SYNC_OUTBOX 32
#define PTO_SYNC_MAILBOX 8
#define PTO_SYNC_INLINE_BYTES 32
/* One serialized executor per bank; platform supplies a context-local binding.
 * These are bounded software/reference capacities, never hardware limits. */
typedef struct pto_sync_runtime pto_sync_runtime;
typedef struct {
  void *context;
  int (*resolve)(void *, wse_endpoint_t, uint32_t *);
  int (*source)(void *, uint32_t, wse_endpoint_t *);
  /* Capture older local matching work; cookie lifetime extends through all
   * retained dependency references. poll reports a terminal status or PENDING.
   * validate must check destination visibility/read coverage, not just READY. */
  wse_status_t (*capture)(void *, int reads, wse_window_t, uint64_t, uint64_t,
                          uint64_t *cookie);
  wse_status_t (*poll)(void *, uint64_t cookie);
  int (*validate)(void *, uint64_t cookie, const wse_sync_key_t *, int reads);
  void (*drop)(void *, uint64_t cookie);
  /* Acquires the exact region/scope on READY. May defer with PENDING. */
  wse_status_t (*acquire)(void *, const wse_sync_key_t *);
  /* For ABORT this must include revocation/drain of every authorized reader,
   * including future accesses, not merely currently outstanding DMA. */
  int (*quiescent)(void *, const wse_sync_key_t *);
  /* Idempotent trusted resource owner, validates exact lease and sender. */
  int (*resource_validate)(void *, uint64_t cookie, uint64_t resource, uint64_t lease);
  wse_status_t (*resource_return)(void *, uint32_t sender, uint64_t resource,
                                  uint64_t lease);
  void (*yield)(void *);
} pto_sync_platform;
typedef struct {
  uint64_t token, cookie, dependency, event, object, actor, argument, detail;
  uint32_t destination, refs, bytes;
  unsigned char payload[PTO_SYNC_INLINE_BYTES];
  void *output;
  wse_status_t status;
  uint8_t live, event_kind, opcode, sent, is_event, caller_live;
} pto_sync_slot;
typedef struct {
  uint64_t identity;
  uint32_t endpoint;
} pto_sync_member;
typedef struct {
  int bound, initialized, group;
  uint64_t handle, capability;
  wse_sync_key_t key;
  wse_region_t region;
  uint32_t owner, count, consumers;
  pto_sync_member producers[PTO_SYNC_MEMBERS], readers[PTO_SYNC_MEMBERS];
  uint32_t done, released, relinquished;
  wse_sync_state_t state;
  wse_status_t error;
  uint64_t ready_token[PTO_SYNC_MEMBERS];
  uint32_t ready_destination[PTO_SYNC_MEMBERS];
  uint32_t ready_sent, free_sent;
  uint64_t free_token[PTO_SYNC_MEMBERS];
  uint32_t free_destination[PTO_SYNC_MEMBERS];
  uint64_t abort_token;
  uint32_t abort_destination;
} pto_sync_object;
typedef struct {
  uint32_t destination;
  uint64_t words[10];
} pto_sync_envelope;
typedef struct {
  wse_message_info_t info;
  unsigned char payload[PTO_SYNC_INLINE_BYTES];
} pto_sync_mail;
typedef struct {
  uint32_t sender;
  uint64_t token;
  uint64_t request[10], response[10];
  int valid;
} pto_sync_receipt;
typedef struct {
  uint32_t sender;
  uint64_t invocation, id, tag, payload[4];
  unsigned bytes;
  int valid;
} pto_sync_dedup;
struct pto_sync_runtime {
  pto_msgb_endpoint endpoint;
  pto_sync_platform platform;
  uint32_t endpoint_id, token_namespace, next_generation;
  uint64_t invocation;
  int draining, failed, progressing, rx_dispatched;
  int tx_kind, rx_recovery, rx_releasing;
  unsigned tx_slot;
  pto_sync_slot slots[PTO_SYNC_SLOTS];
  pto_sync_object objects[PTO_SYNC_OBJECTS];
  pto_sync_envelope outbox[PTO_SYNC_OUTBOX];
  uint32_t out_begin, out_count;
  pto_sync_mail mailbox[PTO_SYNC_MAILBOX];
  uint32_t mail_begin, mail_count;
  pto_sync_dedup tasks[PTO_SYNC_SLOTS];
  pto_sync_receipt receipts[PTO_SYNC_SLOTS];
};
/* Manager supplies a unique nonzero token_namespace, never reuses it while
 * aliases remain. Initialize only a new/quiescent executor, not fault recovery. */
int pto_sync_runtime_init(pto_sync_runtime *, pto_msgb_io, pto_sync_platform,
                          uint32_t endpoint, uint32_t token_namespace,
                          uint64_t invocation);
/* Deployment-time immutable bindings copied into each participating executor.
 * The handle is wire identity, capability authorizes owner-only emission. */
wse_status_t pto_sync_bind_data(pto_sync_runtime *, uint64_t handle,
                                uint32_t owner, uint64_t capability,
                                const wse_sync_desc_t *);
wse_status_t pto_sync_bind_group(pto_sync_runtime *, uint64_t handle,
                                 uint32_t owner, uint64_t capability,
                                 const wse_group_desc_t *);
wse_status_t pto_sync_progress(pto_sync_runtime *);
/* Only after platform recovery has completed the original instruction. */
wse_status_t pto_sync_resume_tx(pto_sync_runtime *);
wse_status_t pto_sync_resume_ack(pto_sync_runtime *);
wse_status_t pto_sync_resume_rx(pto_sync_runtime *);
void pto_sync_begin_drain(pto_sync_runtime *);
int pto_sync_quiescent(const pto_sync_runtime *);
/* Defined by the launcher, usually TLS or a PE/context-local pointer. */
pto_sync_runtime *pto_sync_current_runtime(void);
#ifdef __cplusplus
}
#endif
#endif
