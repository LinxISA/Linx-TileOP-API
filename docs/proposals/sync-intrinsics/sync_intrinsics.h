#ifndef WSE_MESSAGE_INTRINSICS_V0_H
#define WSE_MESSAGE_INTRINSICS_V0_H

/* Design declarations only. No implementation, opcode or wire ABI is assigned.
 * WSE_DEVICE is supplied by the target compiler; empty for host syntax checks.
 * Scalar messaging/synchronization APIs use lowercase __sync_* names.
 * TileOP APIs keep their existing uppercase names; outputs remain first.
 * This is draft v0.3, not an existing Linx compiler builtin family.
 * Naming baseline: Linx-TileOP-API ad5a2310664041be1f2fad5be1dd8e9d682f23df.
 */
#include <stdint.h>
#ifndef WSE_DEVICE
#define WSE_DEVICE
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WSE_OK = 0, WSE_PENDING, WSE_WOULD_BLOCK, WSE_INVALID,
    WSE_NOT_FOUND, WSE_STALE, WSE_CONFLICT, WSE_DENIED,
    WSE_BUSY, WSE_ABORTED, WSE_FAILED, WSE_BUFFER_TOO_SMALL
} wse_status_t;

/* Opaque, context-bound local tokens. Their integer layout is not wire format.
 * Request: remote operation result. Event: a specific completion predicate.
 * Zero-initialized tokens are invalid; no implicit already-complete event.
 */
typedef struct { uint64_t opaque; } wse_request_t;
typedef struct { uint64_t opaque; } wse_event_t;
typedef struct { uint64_t opaque; } wse_service_t;
typedef struct { uint64_t opaque; } wse_window_t;

typedef struct {
    uint64_t domain_id;
    uint64_t invocation_id;
    uint64_t object_id;
    uint64_t generation;
    uint64_t visibility_domain;
} wse_sync_key_t;

typedef struct {
    uint64_t logical_task;
    uint32_t pe;
    uint32_t port;
} wse_endpoint_t;

typedef struct {
    uint64_t contribution_id;
    wse_endpoint_t producer;
} wse_contribution_t;

typedef struct {
    uint64_t consumer_slot;
    wse_endpoint_t consumer;
} wse_consumer_t;

typedef struct {
    wse_window_t window;
    uint64_t byte_offset;
    uint64_t byte_extent;
    uint64_t layout_id;
} wse_region_t;

/* Every list is a set; duplicate identities are invalid. INIT copies metadata,
 * not payload. Zero contributions require an independently valid empty-result
 * contract. Endpoint bindings authorize producers/consumers, not just counts.
 */
typedef struct {
    wse_sync_key_t key;
    wse_region_t region;
    const wse_contribution_t *contributions;
    uint32_t contribution_count;
    const wse_consumer_t *consumers;
    uint32_t consumer_count;
} wse_sync_desc_t;

typedef struct {
    uint64_t member_id;
    wse_endpoint_t endpoint;
} wse_group_member_t;

typedef struct {
    wse_sync_key_t key;
    const wse_group_member_t *members;
    uint32_t member_count; /* nonzero */
} wse_group_desc_t;

typedef enum {
    WSE_COLLECTING, WSE_READY, WSE_FREE, WSE_RETIRED,
    WSE_ABORTING, WSE_ENTRY_FAILED
} wse_sync_state_t;

typedef struct {
    wse_sync_key_t key;
    wse_sync_state_t state;
    uint32_t expected_contributions;
    uint32_t completed_contributions;
    uint32_t expected_consumers;
    uint32_t released_consumers;
    wse_status_t error;
} wse_sync_snapshot_t;

/* Nonblocking submission contract:
 * WSE_OK means accepted locally, not remote completion. On rejection no output
 * token changes and no remote side effect occurs. The implementation retries an
 * accepted request with the same identity; callers must not resubmit it.
 * Input metadata/nested storage and asynchronous output buffers are borrowed
 * until request terminal. Output token pointers are written during the call
 * only. Event-only adapters capture inputs by value and have no request.
 * All required event/request storage is reserved before acceptance.
 * A failed request terminal-fails its still-pending output events; completed
 * event outcomes never regress. Accepted dependencies retain internal refs.
 */

/* INIT: only the designated owner may create this exact generation. */
WSE_DEVICE wse_status_t __sync_init_async(
    wse_request_t *request,
    const wse_sync_desc_t *desc);
/* QUERY: snapshot is valid after successful request completion; no read lease. */
WSE_DEVICE wse_status_t __sync_query_async(
    wse_sync_snapshot_t *snapshot,
    wse_request_t *request,
    const wse_sync_key_t *key);
/* RETIRE: WSE_BUSY until data references and protocol obligations can retire. */
WSE_DEVICE wse_status_t __sync_retire_async(
    wse_request_t *request,
    const wse_sync_key_t *key);

/* SUBSCRIBE: ready is allocated at acceptance, before any READY can arrive.
 * Request completion means subscription accepted; ready means DATA_READY.
 * Cannot create new consumer obligations or reactivate a released slot.
 */
WSE_DEVICE wse_status_t __sync_subscribe_async(
    wse_event_t *ready,
    wse_request_t *request,
    const wse_sync_key_t *key,
    uint64_t consumer_slot);
/* Same SUBSCRIBE service, BUFFER_FREE predicate for this exact generation;
 * grants no payload read lease or permission to reuse another window.
 */
WSE_DEVICE wse_status_t __sync_watch_free_async(
    wse_event_t *free_event,
    wse_request_t *request,
    const wse_sync_key_t *key);

/* DONE.release: visible must be a successful, matching destination-visibility
 * event from the data engine. Pending events defer emission without blocking
 * the caller. A failed/mismatched event fails this request, never increments.
 */
WSE_DEVICE wse_status_t __sync_done_release_async(
    wse_request_t *request,
    const wse_sync_key_t *key,
    uint64_t contribution_id,
    wse_event_t visible);
/* RELEASE.release: reads_done covers all outstanding reads of this lease;
 * caller relinquishes future reads on acceptance. Failure requires recovery,
 * not assuming the lease is reusable. Successful request != BUFFER_FREE.
 */
WSE_DEVICE wse_status_t __sync_release_async(
    wse_request_t *request,
    const wse_sync_key_t *key,
    uint64_t consumer_slot,
    wse_event_t reads_done);

/* INIT for GROUP objects; same key namespace, incompatible with DATA objects. */
WSE_DEVICE wse_status_t __sync_group_init_async(
    wse_request_t *request,
    const wse_group_desc_t *desc);
/* ARRIVE: captures one member arrival after optional prior_done; group_ready is
 * separate from the arrival acknowledgement. NULL means pure rendezvous with
 * no payload visibility promise. Non-NULL event must match declared prior work.
 * An accepted call captures the event value and retains an internal reference;
 * unlike other input metadata, prior_done storage is needed only for this call.
 */
WSE_DEVICE wse_status_t __sync_group_arrive_async(
    wse_event_t *group_ready,
    wse_request_t *request,
    const wse_sync_key_t *group,
    uint64_t member_id,
    const wse_event_t *prior_done);

typedef struct {
    wse_endpoint_t destination;
    uint64_t application_message_id; /* unique per sender/invocation, stable on retry */
    uint64_t task_tag;
    const void *payload;
    uint32_t payload_bytes; /* bounded by target capability, not bulk DMA */
} wse_task_msg_t;

/* TASK_NOTIFY: successful request means accepted by destination mailbox,
 * not task executed. No implicit payload-buffer acquire beyond inline bytes.
 */
WSE_DEVICE wse_status_t __sync_task_notify_async(
    wse_request_t *request,
    const wse_task_msg_t *message);

typedef struct {
    wse_endpoint_t owner;
    uint64_t resource_id;
    uint64_t lease_id; /* one exact resource grant, includes its incarnation */
} wse_resource_return_t;

/* RESOURCE_RETURN: idempotently returns this entire lease; not an arbitrary +N. */
WSE_DEVICE wse_status_t __sync_resource_return_async(
    wse_request_t *request,
    const wse_resource_return_t *resource,
    wse_event_t use_done);

/* ABORT: request success means accepted failure transition. Quiescent event
 * success separately means old accesses drained and the bound storage is safe.
 */
WSE_DEVICE wse_status_t __sync_abort_async(
    wse_event_t *quiescent,
    wse_request_t *request,
    const wse_sync_key_t *key,
    uint32_t reason);

typedef struct {
    wse_sync_key_t key;
    uint64_t report_id;
    wse_status_t code;
    uint64_t detail;
} wse_error_report_t;

/* ERROR: routed to the object's owner; acknowledgement is not successful data. */
WSE_DEVICE wse_status_t __sync_error_report_async(
    wse_request_t *request,
    const wse_error_report_t *error);

/* Test/wait do not consume tokens. PENDING has no acquire effect.
 * Request wait: operation-specific result and visibility of output metadata.
 * Event acquire: only the event's declared region/scope, not a global fence.
 * Wait blocks the issuing execution context, not all PE contexts in a Core.
 * Release requires terminal state and no concurrent local users, else BUSY.
 * Successful release invalidates the caller's token and every copied alias;
 * internally retained dependency references remain alive until their users end.
 */
WSE_DEVICE wse_status_t __sync_request_test(wse_request_t request);
WSE_DEVICE wse_status_t __sync_request_wait(wse_request_t request);
WSE_DEVICE wse_status_t __sync_event_test_acquire(wse_event_t event);
WSE_DEVICE wse_status_t __sync_event_wait_acquire(wse_event_t event);
WSE_DEVICE wse_status_t __sync_request_release(wse_request_t request);
WSE_DEVICE wse_status_t __sync_event_release(wse_event_t event);

/* Local data-engine event adapters (not HSCB messages). On acceptance they
 * capture all older matching accesses by this issuer into a closed dependency
 * set. Cannot be used to assert completion of unknown remote work.
 * Output token storage is used only during this call. The window binding and
 * referenced storage stay pinned through event termination and any held leases.
 */
WSE_DEVICE wse_status_t __sync_l1_publish_async(
    wse_event_t *visible,
    wse_window_t window,
    uint64_t byte_offset,
    uint64_t byte_extent);
WSE_DEVICE wse_status_t __sync_l1_reads_complete_async(
    wse_event_t *reads_done,
    wse_window_t window,
    uint64_t byte_offset,
    uint64_t byte_extent);

typedef enum {
    WSE_MESSAGE_TASK, WSE_MESSAGE_RESOURCE, WSE_MESSAGE_ERROR
} wse_message_kind_t;
/* Transport metadata, distinct from caller-assigned application IDs and
 * claim-specific RX tokens. See msgb-ssr-ack.md for the proposed bit layout.
 */
typedef struct {
    uint32_t source_endpoint;
    uint32_t source_epoch;
    uint64_t sequence;
} wse_transport_message_id_t;

typedef struct {
    wse_message_kind_t kind;
    wse_endpoint_t source;
    uint64_t invocation_id;
    uint64_t application_message_id;
    wse_transport_message_id_t transport_message_id;
    uint64_t tag;
    uint32_t payload_bytes;
} wse_message_info_t;

/* Generic application mailbox. Atomically copies and dequeues one message;
 * PENDING means empty. TOO_SMALL fills info with required size, does not pop.
 * READY/FREE/GROUP_READY go to the dependency engine, never this mailbox.
 */
WSE_DEVICE wse_status_t __sync_message_try_recv(
    void *payload,
    wse_message_info_t *info,
    uint32_t capacity_bytes,
    uint32_t port);

/* Synchronizer/firmware-only notification emission API. A define is not an
 * authorization boundary: the backend checks service ownership and entry state.
 * notification_id identifies an existing subscription/arrival, never an
 * arbitrary caller-selected remote ROB tag or an unverified completion claim.
 * Retransmission keeps notification_id and reuses its recorded outcome; it
 * cannot create a new grant after the object's state has advanced.
 */
#ifdef WSE_SYNC_SERVICE_API
typedef struct {
    wse_sync_key_t key;
    uint64_t notification_id;
} wse_notification_t;

/* DATA_READY: validates DATA kind, READY predicate and active consumer lease. */
WSE_DEVICE wse_status_t __sync_ready_async(
    wse_request_t *request,
    wse_service_t service,
    const wse_notification_t *notification);
/* BUFFER_FREE: validates no outstanding data references and FREE predicate. */
WSE_DEVICE wse_status_t __sync_free_async(
    wse_request_t *request,
    wse_service_t service,
    const wse_notification_t *notification);
/* GROUP_READY: validates group membership/phase and all required arrivals. */
WSE_DEVICE wse_status_t __sync_group_ready_async(
    wse_request_t *request,
    wse_service_t service,
    const wse_notification_t *notification);
#endif

#ifdef __cplusplus
}
#endif
#endif
