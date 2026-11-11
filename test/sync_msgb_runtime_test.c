#define WSE_SYNC_SERVICE_API
#include "runtime/sync_runtime.h"
#include <assert.h>
#include <string.h>
#define ENDPOINTS 3
static pto_sync_runtime runtimes[ENDPOINTS];
static pto_sync_runtime *current;
pto_sync_runtime *pto_sync_current_runtime(void) { return current; }
typedef struct {
  unsigned id;
  msgb_ack_model_t rx;
  msgb_ack_context_t context;
  uint64_t stage[10], sequence, stat, hi;
  unsigned acquire_calls, drops, fail_ack, sends, uncertain_send, uncertain_claim, uncertain_release;
} bank;
static bank banks[ENDPOINTS];
static wse_status_t engine_status[64];
static uint64_t next_cookie;
static pto_msgb_io_status read_reg(void *opaque, uint32_t address, uint64_t *out) {
  bank *b = (bank *)opaque;
  if (address == PTO_MSGBACKSTAT && b->fail_ack) {
    b->fail_ack = 0; return PTO_MSGB_IO_RECOVERY;
  }
  switch (address) {
  case PTO_MSGBRXSTATE: *out = msgb_ssrget_rxstate(&b->rx, &b->context); break;
  case PTO_MSGBRXTOKEN: *out = msgb_ssrget_rxtoken(&b->rx, &b->context); break;
  case PTO_MSGBRXMSGID_HI: *out = msgb_transport_message_id_hi(b->rx.snapshot.message.message_id); break;
  case PTO_MSGBRXMSGID_LO: *out = msgb_transport_message_id_lo(b->rx.snapshot.message.message_id); break;
  case PTO_MSGBRXCONTROL: *out = b->rx.snapshot.message.control; break;
  case PTO_MSGBACKSTAT: *out = msgb_ssrget_msgbackstat(&b->context); break;
  case PTO_MSGBACKID: *out = msgb_ssrget_msgbackid(&b->context); break;
  case PTO_MSGBTXSTAT: *out = b->stat; break;
  case PTO_MSGBTXID_HI: *out = b->hi; break;
  case PTO_MSGBTXID_LO: *out = b->sequence; break;
  default:
    if (address < PTO_MSGBD1 || address > PTO_MSGBD10 ||
        msgb_ssrget_msgbd(&b->rx, &b->context, address - PTO_MSGBD1, out) != MSGB_ACK_OK)
      return PTO_MSGB_IO_REJECTED;
  }
  return PTO_MSGB_IO_OK;
}
static pto_msgb_io_status write_reg(void *opaque, uint32_t address, uint64_t value) {
  bank *b = (bank *)opaque;
  if (address >= PTO_MSGBD1 && address <= PTO_MSGBD10) b->stage[address - PTO_MSGBD1] = value;
  else if (address == PTO_MSGBRXCLAIM) (void)msgb_ssrset_rxclaim(&b->rx, &b->context, value);
  else if (address == PTO_MSGBRXRELEASE) (void)msgb_ssrset_rxrelease(&b->rx, &b->context, value);
  else if (address == PTO_MSGBACK) (void)msgb_ssrset_msgback(&b->rx, &b->context, value);
  else return PTO_MSGB_IO_REJECTED;
  if (address == PTO_MSGBRXCLAIM && b->uncertain_claim) {
    b->uncertain_claim = 0; return PTO_MSGB_IO_RECOVERY;
  }
  if (address == PTO_MSGBRXRELEASE && b->uncertain_release) {
    b->uncertain_release = 0; return PTO_MSGB_IO_RECOVERY;
  }
  return PTO_MSGB_IO_OK;
}
static pto_msgb_io_status send_reg(void *opaque, uint64_t control) {
  bank *b = (bank *)opaque;
  unsigned destination = (unsigned)control;
  msgb_transport_message_id_t id;
  assert(destination < ENDPOINTS && pto_msgb_control_valid(control));
  ++b->sends;
  if (banks[destination].rx.queue_head.valid) { b->stat = 0x104; return PTO_MSGB_IO_OK; }
  id.source_endpoint = b->id; id.source_epoch = 1; id.tx_sequence = ++b->sequence;
  assert(msgb_ack_enqueue(&banks[destination].rx, id, control, b->stage,
                           (uint32_t)((control >> 32) & 15)) == MSGB_ACK_OK);
  b->hi = msgb_transport_message_id_hi(id); b->stat = 0x101;
  if (b->uncertain_send) { b->uncertain_send = 0; return PTO_MSGB_IO_RECOVERY; }
  return PTO_MSGB_IO_OK;
}
static int resolve_endpoint(void *ctx, wse_endpoint_t e, uint32_t *id) {
  (void)ctx;
  if (e.logical_task >= ENDPOINTS || e.port || e.pe) return 0;
  *id = (uint32_t)e.logical_task; return 1;
}
static int source_endpoint(void *ctx, uint32_t id, wse_endpoint_t *e) {
  (void)ctx;
  if (id >= ENDPOINTS) return 0;
  e->logical_task = id; e->pe = e->port = 0; return 1;
}
static wse_status_t capture(void *ctx, int reads, wse_window_t window,
                             uint64_t off, uint64_t n, uint64_t *cookie) {
  (void)ctx; (void)reads;
  assert(window.opaque == 1 && off == 0 && n == 64);
  *cookie = ++next_cookie; engine_status[*cookie] = WSE_PENDING; return WSE_OK;
}
static wse_status_t poll(void *ctx, uint64_t cookie) { (void)ctx; return engine_status[cookie]; }
static int validate(void *ctx, uint64_t cookie, const wse_sync_key_t *key, int reads) {
  (void)ctx; (void)reads; return cookie && key->visibility_domain == 9;
}
static void drop(void *ctx, uint64_t cookie) { (void)cookie; ++((bank *)ctx)->drops; }
static int quiet(void *ctx, const wse_sync_key_t *key) { (void)ctx; (void)key; return 1; }
static wse_status_t acquire(void *ctx, const wse_sync_key_t *key) {
  assert(key->visibility_domain == 9); ++((bank *)ctx)->acquire_calls; return WSE_OK;
}
static void pump(void) {
  unsigned i, step;
  for (step = 0; step < 80; ++step) for (i = 0; i < ENDPOINTS; ++i) {
    wse_status_t status;
    current = &runtimes[i]; status = pto_sync_progress(current);
    assert(status == WSE_OK || status == WSE_WOULD_BLOCK || status == WSE_BUSY);
  }
}
static void choose(unsigned id) { current = &runtimes[id]; }
static void request_ok(unsigned endpoint, wse_request_t request) {
  pump(); choose(endpoint); assert(__sync_request_test(request) == WSE_OK);
  assert(__sync_request_release(request) == WSE_OK);
  assert(__sync_request_test(request) == WSE_STALE);
}
static void setup(void) {
  unsigned i;
  memset(runtimes, 0, sizeof(runtimes)); memset(banks, 0, sizeof(banks));
  for (i = 0; i < ENDPOINTS; ++i) {
    pto_msgb_io io; pto_sync_platform platform;
    banks[i].id = i;
    assert(msgb_ack_model_init(&banks[i].rx, 1) == MSGB_ACK_OK);
    msgb_ack_context_init(&banks[i].context, i + 1);
    io.context = &banks[i]; io.read = read_reg; io.write = write_reg; io.send = send_reg;
    memset(&platform, 0, sizeof(platform)); platform.context = &banks[i];
    platform.resolve = resolve_endpoint; platform.source = source_endpoint; platform.capture = capture; platform.poll = poll;
    platform.validate = validate; platform.drop = drop;
    platform.quiescent = quiet; platform.acquire = acquire;
    assert(pto_sync_runtime_init(&runtimes[i], io, platform, i, i + 1, 1));
  }
}
static void data_lifecycle(void) {
  wse_contribution_t producers[2] = {{10, {0, 0, 0}}, {11, {0, 0, 0}}};
  wse_consumer_t consumers[2] = {{20, {1, 0, 0}}, {21, {2, 0, 0}}};
  wse_sync_desc_t desc = {{7, 1, 100, 1, 9}, {{1}, 0, 64, 0}, producers, 2, consumers, 2};
  wse_request_t req, duplicate, sub, release, query;
  wse_event_t ready1, ready2, visible, reads, free_event;
  wse_sync_snapshot_t snapshot;
  unsigned i;
  setup();
  for (i = 0; i < ENDPOINTS; ++i)
    assert(pto_sync_bind_data(&runtimes[i], 1, 0, 99, &desc) == WSE_OK);
  choose(0); assert(__sync_init_async(&req, &desc) == WSE_OK); request_ok(0, req);
  assert(__sync_watch_free_async(&free_event, &req, &desc.key) == WSE_OK); request_ok(0, req);
  choose(1); assert(__sync_subscribe_async(&ready1, &sub, &desc.key, 20) == WSE_OK);
  request_ok(1, sub); assert(__sync_event_test_acquire(ready1) == WSE_PENDING);
  choose(0); assert(__sync_l1_publish_async(&visible, desc.region.window, 0, 64) == WSE_OK);
  assert(__sync_done_release_async(&req, &desc.key, 10, visible) == WSE_OK);
  pump(); choose(0); assert(__sync_request_test(req) == WSE_PENDING);
  engine_status[next_cookie] = WSE_OK; request_ok(0, req);
  assert(__sync_done_release_async(&duplicate, &desc.key, 10, visible) == WSE_OK);
  request_ok(0, duplicate);
  assert(__sync_query_async(&snapshot, &query, &desc.key) == WSE_OK); request_ok(0, query);
  assert(snapshot.completed_contributions == 1 && snapshot.state == WSE_COLLECTING);
  assert(__sync_done_release_async(&req, &desc.key, 11, visible) == WSE_OK); request_ok(0, req);
  assert(__sync_event_release(visible) == WSE_OK);
  choose(1); assert(__sync_event_test_acquire(ready1) == WSE_OK);
  assert(banks[1].acquire_calls == 1);
  assert(__sync_l1_reads_complete_async(&reads, desc.region.window, 0, 64) == WSE_OK);
  engine_status[next_cookie] = WSE_OK;
  assert(__sync_release_async(&release, &desc.key, 20, reads) == WSE_OK);
  /* Relinquishment applies at local acceptance, before any owner progress. */
  req.opaque = 123;
  assert(__sync_subscribe_async(&ready2, &req, &desc.key, 20) == WSE_STALE);
  assert(req.opaque == 123);
  request_ok(1, release);
  assert(__sync_event_release(reads) == WSE_OK);
  choose(0); assert(__sync_event_test_acquire(free_event) == WSE_PENDING);
  /* The late consumer's fixed obligation prevented reuse. */
  choose(2); assert(__sync_subscribe_async(&ready2, &sub, &desc.key, 21) == WSE_OK);
  request_ok(2, sub); assert(__sync_event_test_acquire(ready2) == WSE_OK);
  assert(__sync_l1_reads_complete_async(&reads, desc.region.window, 0, 64) == WSE_OK);
  engine_status[next_cookie] = WSE_OK;
  assert(__sync_release_async(&release, &desc.key, 21, reads) == WSE_OK); request_ok(2, release);
  assert(__sync_event_release(reads) == WSE_OK);
  choose(0); assert(__sync_event_test_acquire(free_event) == WSE_OK);
  assert(__sync_retire_async(&req, &desc.key) == WSE_OK); request_ok(0, req);
  choose(1); req.opaque = 123;
  assert(__sync_subscribe_async(&ready1, &req, &desc.key, 20) == WSE_STALE);
  assert(req.opaque == 123);
}
static void failed_dependency_and_group(void) {
  wse_group_member_t members[2] = {{10, {0, 0, 0}}, {11, {1, 0, 0}}};
  wse_group_desc_t group = {{7, 1, 101, 1, 9}, members, 2};
  wse_request_t req, req0, req1; wse_event_t ready0, ready1;
  unsigned i;
  setup();
  for (i = 0; i < ENDPOINTS; ++i)
    assert(pto_sync_bind_group(&runtimes[i], 2, 0, 99, &group) == WSE_OK);
  choose(0); assert(__sync_group_init_async(&req, &group) == WSE_OK); request_ok(0, req);
  assert(__sync_group_arrive_async(&ready0, &req0, &group.key, 10, NULL) == WSE_OK);
  request_ok(0, req0); assert(__sync_event_test_acquire(ready0) == WSE_PENDING);
  choose(1); assert(__sync_group_arrive_async(&ready1, &req1, &group.key, 11, NULL) == WSE_OK);
  request_ok(1, req1); assert(__sync_event_test_acquire(ready1) == WSE_OK);
  choose(0); assert(__sync_event_test_acquire(ready0) == WSE_OK);
  /* The declaration permits a pure rendezvous without payload visibility. */
  assert(banks[0].acquire_calls == 0 && banks[1].acquire_calls == 0);
  {
    wse_event_t prior, failed; wse_window_t window = {1};
    assert(__sync_l1_publish_async(&prior, window, 0, 64) == WSE_OK);
    assert(__sync_group_arrive_async(&failed, &req, &group.key, 10, &prior) == WSE_OK);
    engine_status[next_cookie] = WSE_FAILED;
    pump(); choose(0);
    assert(__sync_request_test(req) == WSE_FAILED);
    assert(__sync_event_test_acquire(failed) == WSE_FAILED);
    assert(__sync_event_release(prior) == WSE_OK);
  }
}
static void abort_and_error(void) {
  wse_contribution_t contribution = {10, {0, 0, 0}};
  wse_consumer_t consumer = {20, {1, 0, 0}};
  wse_sync_desc_t desc = {{7, 1, 100, 1, 9}, {{1}, 0, 64, 0}, &contribution, 1, &consumer, 1};
  wse_request_t req, sub; wse_event_t ready, drained;
  unsigned i;
  setup();
  for (i = 0; i < ENDPOINTS; ++i)
    assert(pto_sync_bind_data(&runtimes[i], 1, 0, 99, &desc) == WSE_OK);
  choose(0); assert(__sync_init_async(&req, &desc) == WSE_OK); request_ok(0, req);
  choose(1); assert(__sync_subscribe_async(&ready, &sub, &desc.key, 20) == WSE_OK); request_ok(1, sub);
  choose(0); assert(__sync_abort_async(&drained, &req, &desc.key, 5) == WSE_OK); request_ok(0, req);
  assert(__sync_event_test_acquire(drained) == WSE_OK);
  choose(1); assert(__sync_event_test_acquire(ready) == WSE_ABORTED);
  {
    wse_notification_t n; wse_service_t service = {98};
    n.key = desc.key; n.notification_id = ready.opaque;
    assert(__sync_ready_async(&req, service, &n) == WSE_DENIED);
  }
  choose(0); assert(__sync_retire_async(&req, &desc.key) == WSE_OK); request_ok(0, req);
  /* A new request identity cannot resurrect an already retired key. */
  assert(__sync_init_async(&req, &desc) == WSE_OK); pump(); choose(0);
  assert(__sync_request_test(req) == WSE_STALE);
  setup();
  for (i = 0; i < ENDPOINTS; ++i)
    assert(pto_sync_bind_data(&runtimes[i], 1, 0, 99, &desc) == WSE_OK);
  choose(0); assert(__sync_init_async(&req, &desc) == WSE_OK); request_ok(0, req);
  choose(1); assert(__sync_subscribe_async(&ready, &sub, &desc.key, 20) == WSE_OK); request_ok(1, sub);
  {
    wse_error_report_t error = {{7, 1, 100, 1, 9}, 500, WSE_FAILED, 999};
    assert(__sync_error_report_async(&req, &error) == WSE_OK); request_ok(1, req);
    assert(__sync_event_test_acquire(ready) == WSE_FAILED);
  }
}
static void task_and_ack_recovery(void) {
  const char payload[] = "hello"; char out[8] = {0};
  wse_task_msg_t message = {{1, 0, 0}, 1000, 77, payload, sizeof(payload)};
  wse_request_t req; wse_message_info_t info;
  setup(); choose(0);
  assert(__sync_task_notify_async(&req, &message) == WSE_OK);
  banks[1].fail_ack = 1; banks[0].uncertain_send = 1;
  banks[1].uncertain_claim = 1;
  pump();
  assert(runtimes[0].endpoint.tx_pending && banks[0].sends == 1);
  assert(pto_sync_resume_tx(&runtimes[0]) == WSE_OK);
  assert(banks[0].sends == 1);
  assert(runtimes[1].rx_recovery && !runtimes[1].mail_count);
  assert(pto_sync_resume_rx(&runtimes[1]) == WSE_OK);
  pump();
  assert(runtimes[1].endpoint.ack_pending && runtimes[1].mail_count == 1);
  assert(pto_sync_resume_ack(&runtimes[1]) == WSE_OK);
  banks[1].uncertain_send = 1;
  pump();
  assert(runtimes[1].endpoint.tx_pending && runtimes[1].out_count == 1);
  assert(pto_sync_resume_tx(&runtimes[1]) == WSE_OK);
  assert(!runtimes[1].out_count && banks[1].sends == 1);
  banks[1].uncertain_release = 1;
  pump();
  assert(runtimes[1].rx_recovery && runtimes[1].endpoint.rx_acked);
  assert(pto_sync_resume_rx(&runtimes[1]) == WSE_OK);
  request_ok(0, req);
  choose(1);
  assert(__sync_message_try_recv(out, &info, 2, 0) == WSE_BUFFER_TOO_SMALL);
  assert(info.payload_bytes == sizeof(payload) && out[0] == 0);
  assert(__sync_message_try_recv(out, &info, sizeof(out), 0) == WSE_OK);
  assert(!memcmp(out, payload, sizeof(payload)) && info.application_message_id == 1000);
  choose(0); assert(__sync_task_notify_async(&req, &message) == WSE_OK); request_ok(0, req);
  choose(1); assert(__sync_message_try_recv(out, &info, sizeof(out), 0) == WSE_PENDING);
  choose(0); message.task_tag++;
  assert(__sync_task_notify_async(&req, &message) == WSE_OK);
  pump(); choose(0); assert(__sync_request_test(req) == WSE_CONFLICT);
  choose(1); assert(__sync_message_try_recv(out, &info, sizeof(out), 0) == WSE_PENDING);
  pto_sync_begin_drain(current);
  req.opaque = 123;
  assert(__sync_task_notify_async(&req, &message) == WSE_BUSY && req.opaque == 123);
  assert(pto_sync_quiescent(current));
}
static void capacity_and_retained_service_result(void) {
  wse_task_msg_t message = {{1, 0, 0}, 1000, 77, NULL, 0};
  wse_request_t req, pending;
  unsigned i;
  setup(); choose(0);
  /* Full receipts reject new work, but must still receive its own replies. */
  for (i = 0; i < PTO_SYNC_SLOTS; ++i) {
    runtimes[1].receipts[i].valid = 1;
    runtimes[1].receipts[i].sender = 2;
    runtimes[1].receipts[i].token = i + 1;
  }
  assert(__sync_task_notify_async(&req, &message) == WSE_OK);
  pump(); choose(0); assert(__sync_request_test(req) == WSE_BUSY);
  assert(!runtimes[1].endpoint.rx_pending && !runtimes[1].mail_count);
  assert(__sync_request_release(req) == WSE_OK);
  /* Slot and generation exhaustion never modify output tokens. */
  for (i = 0; i < PTO_SYNC_SLOTS; ++i)
    assert(__sync_task_notify_async(&req, &message) == WSE_OK);
  req.opaque = 123;
  assert(__sync_task_notify_async(&req, &message) == WSE_WOULD_BLOCK);
  assert(req.opaque == 123);
  setup(); choose(0); current->next_generation = UINT16_MAX;
  assert(__sync_task_notify_async(&req, &message) == WSE_WOULD_BLOCK);
  assert(req.opaque == 123);
  {
    wse_group_member_t member0 = {10, {0, 0, 0}};
    wse_group_desc_t group = {{7, 1, 101, 1, 9}, &member0, 1};
    wse_event_t ready;
    wse_notification_t notification;
    wse_service_t capability = {99};
    setup();
    assert(pto_sync_bind_group(&runtimes[0], 2, 0, 99, &group) == WSE_OK);
    choose(0);
    assert(__sync_group_init_async(&req, &group) == WSE_OK); request_ok(0, req);
    assert(__sync_group_arrive_async(&ready, &req, &group.key, 10, NULL) == WSE_OK);
    request_ok(0, req);
    assert(__sync_event_release(ready) == WSE_OK);
    notification.key = group.key; notification.notification_id = ready.opaque;
    assert(__sync_group_ready_async(&pending, capability, &notification) == WSE_OK);
    request_ok(0, pending);
    assert(__sync_event_test_acquire(ready) == WSE_STALE);
    pto_sync_begin_drain(current);
    assert(__sync_retire_async(&req, &group.key) == WSE_OK); request_ok(0, req);
    assert(pto_sync_quiescent(current));
  }
}
int main(void) {
  data_lifecycle(); failed_dependency_and_group(); abort_and_error(); task_and_ack_recovery(); capacity_and_retained_service_result(); return 0;
}
