#define WSE_SYNC_SERVICE_API
#include "sync_runtime.h"
#include <string.h>
/* Private codec v1: ten logical 64-bit words, never a serialized C struct.
 * w0 version/op/status/length, w1 invocation, w2 request token, w3 object
 * handle, w4 actor, w5 argument/event token, w6..9 inline data/query output.
 * Sender is always RX MSGID.source_endpoint, not caller-supplied payload.
 */
enum { INIT = 1, QUERY, RETIRE, SUBSCRIBE, DONE, RELEASE, ARRIVE, TASK,
       RESOURCE, ABORT, ERROR_REPORT, READY, FREE, GROUP_READY, RESULT,
       QUIESCENT };
enum { EVENT_READY = 1, EVENT_FREE, EVENT_GROUP, EVENT_DRAIN,
       EVENT_VISIBLE, EVENT_READS };
static uint32_t full_mask(uint32_t n) { return (UINT32_C(1) << n) - 1; }
static int key_equal(const wse_sync_key_t *a, const wse_sync_key_t *b) {
  return a->domain_id == b->domain_id && a->invocation_id == b->invocation_id &&
    a->object_id == b->object_id && a->generation == b->generation &&
    a->visibility_domain == b->visibility_domain;
}
static int region_equal(wse_region_t a, wse_region_t b) {
  return a.window.opaque == b.window.opaque && a.byte_offset == b.byte_offset &&
    a.byte_extent == b.byte_extent && a.layout_id == b.layout_id;
}
static pto_sync_object *by_key(pto_sync_runtime *r, const wse_sync_key_t *k) {
  unsigned i;
  if (!k) return NULL;
  for (i = 0; i < PTO_SYNC_OBJECTS; ++i)
    if (r->objects[i].bound && key_equal(&r->objects[i].key, k)) return &r->objects[i];
  return NULL;
}
static pto_sync_object *by_handle(pto_sync_runtime *r, uint64_t h) {
  unsigned i;
  for (i = 0; i < PTO_SYNC_OBJECTS; ++i)
    if (r->objects[i].bound && r->objects[i].handle == h) return &r->objects[i];
  return NULL;
}
static pto_sync_slot *lookup(pto_sync_runtime *r, uint64_t token) {
  unsigned index = (unsigned)(token & 0xffff);
  pto_sync_slot *s;
  if (!index || index > PTO_SYNC_SLOTS) return NULL;
  s = &r->slots[index - 1];
  return s->live && s->token == token ? s : NULL;
}
static pto_sync_slot *allocate(pto_sync_runtime *r, int event) {
  unsigned i;
  if (r->next_generation == UINT16_MAX) return NULL;
  for (i = 0; i < PTO_SYNC_SLOTS; ++i) if (!r->slots[i].live) {
    pto_sync_slot *s = &r->slots[i]; memset(s, 0, sizeof(*s));
    s->live = s->caller_live = 1; s->is_event = (uint8_t)event;
    s->token = ((uint64_t)r->token_namespace << 32) |
      ((uint64_t)++r->next_generation << 16) | (i + 1);
    s->status = WSE_PENDING; return s;
  }
  return NULL;
}
static void collect(pto_sync_runtime *r, pto_sync_slot *s) {
  if (!s || s->caller_live || s->refs || s->status == WSE_PENDING) return;
  /* Retain wire-event tombstones through invocation retirement: service
   * retries still need an authenticated terminal response after caller release. */
  if (s->is_event && s->event_kind >= EVENT_READY &&
      s->event_kind <= EVENT_DRAIN) return;
  if ((s->event_kind == EVENT_VISIBLE || s->event_kind == EVENT_READS) &&
      r->platform.drop) r->platform.drop(r->platform.context, s->cookie);
  s->live = 0;
}
static void finish(pto_sync_runtime *r, pto_sync_slot *s, wse_status_t status) {
  pto_sync_slot *event, *dep;
  if (!s || s->status != WSE_PENDING || status == WSE_PENDING) return;
  s->status = status;
  if (!s->is_event) {
    event = lookup(r, s->event);
    if (status != WSE_OK && event && event->status == WSE_PENDING)
      event->status = status;
    if (event && event->refs) { --event->refs; collect(r, event); }
    dep = lookup(r, s->dependency);
    if (dep && dep->refs) { --dep->refs; collect(r, dep); }
    s->dependency = 0;
  }
  collect(r, s);
}
static int resolve(pto_sync_runtime *r, wse_endpoint_t e, uint32_t *id) {
  return r->platform.resolve && r->platform.resolve(r->platform.context, e, id);
}
int pto_sync_runtime_init(pto_sync_runtime *r, pto_msgb_io io,
                          pto_sync_platform p, uint32_t endpoint,
                          uint32_t token_namespace, uint64_t invocation) {
  if (!r || !pto_msgb_io_valid(io) || !token_namespace || !invocation) return 0;
  memset(r, 0, sizeof(*r)); pto_msgb_endpoint_init(&r->endpoint, io);
  r->platform = p; r->endpoint_id = endpoint;
  r->token_namespace = token_namespace; r->invocation = invocation; return 1;
}
static pto_sync_object *binding_slot(pto_sync_runtime *r, uint64_t h,
                                      const wse_sync_key_t *key) {
  unsigned i;
  if (!h || !key || key->invocation_id != r->invocation ||
      by_handle(r, h) || by_key(r, key)) return NULL;
  for (i = 0; i < PTO_SYNC_OBJECTS; ++i) if (!r->objects[i].bound) return &r->objects[i];
  return NULL;
}
wse_status_t pto_sync_bind_data(pto_sync_runtime *r, uint64_t handle,
                                uint32_t owner, uint64_t capability,
                                const wse_sync_desc_t *d) {
  pto_sync_object o, *slot; unsigned i, j;
  if (!r || !d || !capability || !d->region.window.opaque ||
      d->region.byte_offset > UINT64_MAX - d->region.byte_extent ||
      d->contribution_count > PTO_SYNC_MEMBERS ||
      d->consumer_count > PTO_SYNC_MEMBERS ||
      (d->contribution_count && !d->contributions) ||
      (d->consumer_count && !d->consumers)) return WSE_INVALID;
  slot = binding_slot(r, handle, &d->key); if (!slot) return WSE_CONFLICT;
  memset(&o, 0, sizeof(o)); o.key = d->key; o.region = d->region;
  o.handle = handle; o.owner = owner; o.capability = capability; o.bound = 1;
  o.count = d->contribution_count; o.consumers = d->consumer_count;
  for (i = 0; i < o.count; ++i) {
    o.producers[i].identity = d->contributions[i].contribution_id;
    if (!resolve(r, d->contributions[i].producer, &o.producers[i].endpoint)) return WSE_DENIED;
    for (j = 0; j < i; ++j)
      if (o.producers[j].identity == o.producers[i].identity) return WSE_INVALID;
  }
  for (i = 0; i < o.consumers; ++i) {
    o.readers[i].identity = d->consumers[i].consumer_slot;
    if (o.readers[i].identity == UINT64_MAX) return WSE_INVALID;
    if (!resolve(r, d->consumers[i].consumer, &o.readers[i].endpoint)) return WSE_DENIED;
    for (j = 0; j < i; ++j)
      if (o.readers[j].identity == o.readers[i].identity) return WSE_INVALID;
  }
  *slot = o; return WSE_OK;
}
wse_status_t pto_sync_bind_group(pto_sync_runtime *r, uint64_t handle,
                                 uint32_t owner, uint64_t capability,
                                 const wse_group_desc_t *d) {
  pto_sync_object o, *slot; unsigned i, j;
  if (!r || !d || !capability || !d->member_count ||
      d->member_count > PTO_SYNC_MEMBERS || !d->members) return WSE_INVALID;
  slot = binding_slot(r, handle, &d->key); if (!slot) return WSE_CONFLICT;
  memset(&o, 0, sizeof(o)); o.key = d->key; o.handle = handle;
  o.owner = owner; o.capability = capability; o.bound = o.group = 1;
  o.count = o.consumers = d->member_count;
  for (i = 0; i < o.count; ++i) {
    o.producers[i].identity = d->members[i].member_id;
    if (!resolve(r, d->members[i].endpoint, &o.producers[i].endpoint)) return WSE_DENIED;
    for (j = 0; j < i; ++j)
      if (o.producers[j].identity == o.producers[i].identity) return WSE_INVALID;
    o.readers[i] = o.producers[i];
  }
  *slot = o; return WSE_OK;
}
static void enqueue(pto_sync_runtime *r, uint32_t destination, unsigned op,
                     wse_status_t status, uint64_t token, uint64_t object,
                     uint64_t actor, uint64_t arg) {
  pto_sync_envelope *e = &r->outbox[(r->out_begin + r->out_count) % PTO_SYNC_OUTBOX];
  memset(e, 0, sizeof(*e)); e->destination = destination;
  e->words[0] = 1 | ((uint64_t)op << 8) | ((uint64_t)status << 16);
  e->words[1] = r->invocation; e->words[2] = token; e->words[3] = object;
  e->words[4] = actor; e->words[5] = arg; ++r->out_count;
}
static int member(const pto_sync_member *m, unsigned n, uint64_t id, uint32_t sender) {
  unsigned i;
  for (i = 0; i < n; ++i) if (m[i].identity == id && m[i].endpoint == sender) return (int)i;
  return -1;
}
static int participant(const pto_sync_object *o, uint32_t sender) {
  unsigned i;
  if (o->owner == sender) return 1;
  for (i = 0; i < o->count; ++i) if (o->producers[i].endpoint == sender) return 1;
  for (i = 0; i < o->consumers; ++i) if (o->readers[i].endpoint == sender) return 1;
  return 0;
}
static int quiet(pto_sync_runtime *r, pto_sync_object *o) {
  return r->platform.quiescent && r->platform.quiescent(r->platform.context, &o->key);
}
static void notify(pto_sync_runtime *r, pto_sync_object *o) {
  unsigned i;
  /* Reserve the full fanout before changing predicates or dropping tokens. */
  if (PTO_SYNC_OUTBOX - r->out_count < 2 * PTO_SYNC_MEMBERS + 1) return;
  if (o->state == WSE_COLLECTING && o->done == full_mask(o->count) &&
      (o->group || o->count || quiet(r, o))) o->state = WSE_READY;
  for (i = 0; i < o->consumers; ++i) if (o->ready_token[i] &&
      !(o->ready_sent & (UINT32_C(1) << i)) &&
      (o->state == WSE_READY || o->state == WSE_ENTRY_FAILED || o->state == WSE_ABORTING)) {
    enqueue(r, o->ready_destination[i], o->group ? GROUP_READY : READY,
      o->state == WSE_READY ? WSE_OK : o->error,
      o->ready_token[i], o->handle, o->readers[i].identity, 0);
    o->ready_sent |= UINT32_C(1) << i;
  }
  if (!o->group && o->state == WSE_READY &&
      o->released == full_mask(o->consumers) && quiet(r, o)) o->state = WSE_FREE;
  for (i = 0; i < PTO_SYNC_MEMBERS; ++i) if (o->free_token[i] &&
      !(o->free_sent & (UINT32_C(1) << i)) &&
      (o->state == WSE_FREE || o->state == WSE_ENTRY_FAILED || o->state == WSE_ABORTING)) {
    enqueue(r, o->free_destination[i], FREE, o->state == WSE_FREE ? WSE_OK : o->error,
      o->free_token[i], o->handle, 0, 0); o->free_sent |= UINT32_C(1) << i;
  }
  if (o->abort_token && quiet(r, o)) {
    enqueue(r, o->abort_destination, QUIESCENT, WSE_OK, o->abort_token, o->handle, 0, 0);
    o->abort_token = 0;
  }
}
/* Invoked before RX ACK; all output resources have been reserved first. */
static wse_status_t dispatch(pto_sync_runtime *r, const pto_msgb_packet *p) {
  const uint64_t *w = p->words;
  unsigned op = (unsigned)((w[0] >> 8) & 255), i;
  wse_status_t status = WSE_OK;
  uint32_t sender = p->id.source_endpoint;
  pto_sync_object *o = by_handle(r, w[3]);
  pto_sync_slot *s;
  int index, receipt_index = -1;
  if (p->count != 10 || (w[0] & 255) != 1 || w[1] != r->invocation ||
      op < INIT || op > QUIESCENT || (w[0] >> 32) != 0) return WSE_INVALID;
  if (op == RESULT || op == READY || op == FREE || op == GROUP_READY || op == QUIESCENT) {
    status = (wse_status_t)((w[0] >> 16) & 255);
    if (status > WSE_BUFFER_TOO_SMALL || status == WSE_PENDING) return WSE_INVALID;
    s = lookup(r, w[2]);
    /* Old released token notifications are harmless; source still authenticated. */
    if (!s) return WSE_OK;
    if (s->destination != sender || s->object != w[3]) return WSE_DENIED;
    if ((op == RESULT && s->is_event) || (op != RESULT &&
        (!s->is_event ||
         (op == READY && s->event_kind != EVENT_READY) ||
         (op == FREE && s->event_kind != EVENT_FREE) ||
         (op == GROUP_READY && s->event_kind != EVENT_GROUP) ||
         (op == QUIESCENT && s->event_kind != EVENT_DRAIN)))) return WSE_INVALID;
    if (op != RESULT && w[5] && r->out_count == PTO_SYNC_OUTBOX) return WSE_WOULD_BLOCK;
    if (s->status != WSE_PENDING) {
      if (op != RESULT && w[5])
        enqueue(r, sender, RESULT, s->status, w[5], w[3], 0, 0);
      return WSE_OK;
    }
    if (op == READY && status == WSE_OK) {
      if (!o || !r->platform.acquire) return WSE_DENIED;
      status = r->platform.acquire(r->platform.context, &o->key);
      if (status == WSE_PENDING || status == WSE_WOULD_BLOCK) return WSE_WOULD_BLOCK;
    }
    if (op == RESULT) {
      if (s->is_event) return WSE_INVALID;
      if (s->opcode == QUERY && s->output && status == WSE_OK) {
        wse_sync_snapshot_t *out = (wse_sync_snapshot_t *)s->output;
        if (!o || w[6] > WSE_ENTRY_FAILED || w[9] > WSE_BUFFER_TOO_SMALL) return WSE_INVALID;
        out->key = o->key; out->state = (wse_sync_state_t)w[6];
        out->expected_contributions = o->count; out->expected_consumers = o->consumers;
        out->completed_contributions = (uint32_t)w[7];
        out->released_consumers = (uint32_t)w[8]; out->error = (wse_status_t)w[9];
      }
    } else if (!s->is_event ||
        (op == READY && s->event_kind != EVENT_READY) ||
        (op == FREE && s->event_kind != EVENT_FREE) ||
        (op == GROUP_READY && s->event_kind != EVENT_GROUP) ||
        (op == QUIESCENT && s->event_kind != EVENT_DRAIN)) return WSE_INVALID;
    finish(r, s, status);
    if (op != RESULT && w[5]) enqueue(r, sender, RESULT, status, w[5], w[3], 0, 0);
    return WSE_OK;
  }
  if (PTO_SYNC_OUTBOX - r->out_count < 2 * PTO_SYNC_MEMBERS + 2) return WSE_WOULD_BLOCK;
  for (i = 0; i < PTO_SYNC_SLOTS; ++i) {
    pto_sync_receipt *receipt = &r->receipts[i];
    if (!receipt->valid) receipt_index = (int)i;
    else if (receipt->sender == sender && receipt->token == w[2]) {
      pto_sync_envelope *reply;
      if (memcmp(receipt->request, w, sizeof(receipt->request))) return WSE_CONFLICT;
      reply = &r->outbox[(r->out_begin + r->out_count) % PTO_SYNC_OUTBOX];
      reply->destination = sender;
      memcpy(reply->words, receipt->response, sizeof(reply->words));
      ++r->out_count; return WSE_OK;
    }
  }
  if (receipt_index < 0) {
    /* No effects were accepted. Since receipts never retire during this
     * invocation, rejection remains stable and must not block later replies. */
    enqueue(r, sender, RESULT, WSE_BUSY, w[2], w[3], 0, 0);
    return WSE_OK;
  }
  if (op == TASK) {
    unsigned bytes = (unsigned)((w[0] >> 24) & 255), vacant = PTO_SYNC_SLOTS;
    if (bytes > PTO_SYNC_INLINE_BYTES) status = WSE_INVALID;
    else {
      for (i = 0; i < PTO_SYNC_SLOTS; ++i) {
        if (!r->tasks[i].valid) vacant = i;
        else if (r->tasks[i].sender == sender && r->tasks[i].invocation == w[1] &&
                 r->tasks[i].id == w[4]) break;
      }
      if (i == PTO_SYNC_SLOTS) {
        pto_sync_mail *m;
        if (r->mail_count == PTO_SYNC_MAILBOX || vacant == PTO_SYNC_SLOTS) return WSE_WOULD_BLOCK;
        m = &r->mailbox[(r->mail_begin + r->mail_count) % PTO_SYNC_MAILBOX];
        memset(m, 0, sizeof(*m)); m->info.kind = WSE_MESSAGE_TASK;
        if (!r->platform.source ||
            !r->platform.source(r->platform.context, sender, &m->info.source)) return WSE_DENIED;
        r->tasks[vacant].tag = w[3]; r->tasks[vacant].bytes = bytes;
        memcpy(r->tasks[vacant].payload, w + 6, sizeof(r->tasks[vacant].payload));
        m->info.invocation_id = w[1];
        m->info.application_message_id = w[4]; m->info.tag = w[3];
        m->info.payload_bytes = bytes;
        m->info.transport_message_id.source_endpoint = sender;
        m->info.transport_message_id.source_epoch = p->id.source_epoch;
        m->info.transport_message_id.sequence = p->id.tx_sequence;
        for (i = 0; i < bytes; ++i) m->payload[i] = (unsigned char)(w[6 + i / 8] >> (8 * (i % 8)));
        r->tasks[vacant].valid = 1; r->tasks[vacant].sender = sender;
        r->tasks[vacant].invocation = w[1]; r->tasks[vacant].id = w[4]; ++r->mail_count;
      } else if (r->tasks[i].tag != w[3] || r->tasks[i].bytes != bytes ||
                 memcmp(r->tasks[i].payload, w + 6, sizeof(r->tasks[i].payload)))
        status = WSE_CONFLICT;
    }
  } else if (op == RESOURCE) {
    status = r->platform.resource_return ?
      r->platform.resource_return(r->platform.context, sender, w[4], w[5]) : WSE_DENIED;
    if (status == WSE_PENDING || status == WSE_WOULD_BLOCK) return WSE_WOULD_BLOCK;
  } else if (!o || o->owner != r->endpoint_id) status = WSE_NOT_FOUND;
  else if (op == INIT) {
    if (sender != o->owner) status = WSE_DENIED;
    else if (o->state == WSE_RETIRED && o->initialized) status = WSE_STALE;
    else if (!o->initialized) { o->initialized = 1; o->state = WSE_COLLECTING; }
  } else if (!o->initialized) status = WSE_NOT_FOUND;
  else if (op == QUERY) { if (!participant(o, sender)) status = WSE_DENIED; }
  else if (op == RETIRE) {
    if (sender != o->owner) status = WSE_DENIED;
    else if ((o->state != WSE_FREE && !(o->group && o->state == WSE_READY) &&
              o->state != WSE_ABORTING && o->state != WSE_ENTRY_FAILED) || !quiet(r, o)) status = WSE_BUSY;
    else {
      for (i = 0; i < PTO_SYNC_MEMBERS; ++i)
        if ((o->ready_token[i] && !(o->ready_sent & (UINT32_C(1) << i))) ||
            (o->free_token[i] && !(o->free_sent & (UINT32_C(1) << i)))) break;
      if (i != PTO_SYNC_MEMBERS || o->abort_token) status = WSE_BUSY;
      else o->state = WSE_RETIRED;
    }
  } else if (o->state == WSE_RETIRED) status = WSE_STALE;
  else if (op == ABORT || op == ERROR_REPORT) {
    if ((op == ABORT && sender != o->owner) ||
        (op == ERROR_REPORT && !participant(o, sender))) status = WSE_DENIED;
    else if (op == ABORT && o->abort_token && o->abort_token != w[5]) status = WSE_BUSY;
    else if (op == ERROR_REPORT && (w[5] <= WSE_PENDING || w[5] > WSE_BUFFER_TOO_SMALL)) status = WSE_INVALID;
    else {
      o->state = WSE_ABORTING;
      o->error = op == ABORT ? WSE_ABORTED : (wse_status_t)w[5];
      if (op == ABORT) { o->abort_token = w[5]; o->abort_destination = sender; }
    }
  } else if (op == SUBSCRIBE && w[4] == UINT64_MAX) {
    if (sender != o->owner || o->group) status = WSE_DENIED;
    else {
      for (i = 0; i < PTO_SYNC_MEMBERS; ++i)
        if (!o->free_token[i] || o->free_token[i] == w[5]) break;
      if (i == PTO_SYNC_MEMBERS) status = WSE_BUSY;
      else { o->free_token[i] = w[5]; o->free_destination[i] = sender; }
    }
  } else if (op == DONE || op == ARRIVE) {
    index = member(o->producers, o->count, w[4], sender);
    if (index < 0 || (op == ARRIVE) != o->group) status = WSE_DENIED;
    else if (o->state != WSE_COLLECTING && o->state != WSE_READY) status = WSE_STALE;
    else if (op == ARRIVE && o->ready_token[index] && o->ready_token[index] != w[5]) status = WSE_BUSY;
    else {
      o->done |= UINT32_C(1) << index;
      if (op == ARRIVE) {
        o->ready_token[index] = w[5]; o->ready_destination[index] = sender;
      }
    }
  } else if (op == SUBSCRIBE || op == RELEASE) {
    index = member(o->readers, o->consumers, w[4], sender);
    if (index < 0 || o->group) status = WSE_DENIED;
    else if (op == RELEASE) {
      if (o->state == WSE_COLLECTING) status = WSE_INVALID;
      else o->released |= UINT32_C(1) << index;
    } else if ((o->released & (UINT32_C(1) << index)) ||
               (o->state != WSE_COLLECTING && o->state != WSE_READY)) status = WSE_STALE;
    else if (o->ready_token[index] && o->ready_token[index] != w[5]) status = WSE_BUSY;
    else { o->ready_token[index] = w[5]; o->ready_destination[index] = sender; }
  } else status = WSE_INVALID;
  enqueue(r, sender, RESULT, status, w[2], w[3], 0, 0);
  if (op == QUERY && o && status == WSE_OK) {
    pto_sync_envelope *e = &r->outbox[(r->out_begin + r->out_count - 1) % PTO_SYNC_OUTBOX];
    e->words[6] = o->state; e->words[9] = o->error;
    for (i = 0; i < o->count; ++i) e->words[7] += (o->done >> i) & 1;
    for (i = 0; i < o->consumers; ++i) e->words[8] += (o->released >> i) & 1;
  }
  {
    pto_sync_receipt *receipt = &r->receipts[receipt_index];
    receipt->sender = sender; receipt->token = w[2]; receipt->valid = 1;
    memcpy(receipt->request, w, sizeof(receipt->request));
    memcpy(receipt->response,
      r->outbox[(r->out_begin + r->out_count - 1) % PTO_SYNC_OUTBOX].words,
      sizeof(receipt->response));
  }
  if (o && o->owner == r->endpoint_id) notify(r, o);
  return WSE_OK;
}
static wse_status_t map_transport(int code) {
  switch (code) {
  case PTO_MSGB_OK: case PTO_MSGB_DUPLICATE: return WSE_OK;
  case PTO_MSGB_EMPTY: return WSE_PENDING;
  case PTO_MSGB_BUSY: return WSE_WOULD_BLOCK;
  case PTO_MSGB_EXHAUSTED: return WSE_BUSY;
  case PTO_MSGB_LOCAL_RECOVERY: return WSE_BUSY;
  case PTO_MSGB_BAD_COMMAND: case PTO_MSGB_LOCAL_INVALID: return WSE_INVALID;
  default: return WSE_FAILED;
  }
}
static void packet_for_slot(pto_sync_runtime *r, pto_sync_slot *s,
                              uint64_t w[10]) {
  unsigned i; memset(w, 0, 10 * sizeof(uint64_t));
  w[0] = 1 | ((uint64_t)s->opcode << 8) | ((uint64_t)s->bytes << 24);
  w[1] = r->invocation; w[2] = s->token; w[3] = s->object;
  w[4] = s->actor; w[5] = s->argument;
  for (i = 0; i < s->bytes; ++i) w[6 + i / 8] |= (uint64_t)s->payload[i] << (8 * (i % 8));
  if (s->opcode == ERROR_REPORT) w[6] = s->detail;
}
wse_status_t pto_sync_progress(pto_sync_runtime *r) {
  unsigned i; int code; wse_status_t status = WSE_OK;
  if (!r || r->failed) return WSE_FAILED;
  if (r->progressing) return WSE_BUSY;
  r->progressing = 1;
  /* Platform must resume the exact faulting instruction before allowing more
   * progress. Do not read a stale TXSTAT/ACKSTAT as a new operation result. */
  if (r->endpoint.tx_pending || r->endpoint.ack_pending || r->rx_recovery) { status = WSE_BUSY; goto end; }
  for (i = 0; i < PTO_SYNC_SLOTS; ++i) {
    pto_sync_slot *s = &r->slots[i];
    if (s->live && s->is_event && s->status == WSE_PENDING &&
        (s->event_kind == EVENT_VISIBLE || s->event_kind == EVENT_READS)) {
      wse_status_t polled = r->platform.poll ?
        r->platform.poll(r->platform.context, s->cookie) : WSE_FAILED;
      finish(r, s, polled);
    }
  }
  /* Flush committed replies before dispatching more requests. */
  if (r->out_count) {
    pto_sync_envelope *e = &r->outbox[r->out_begin];
    r->tx_kind = 1;
    code = pto_msgb_send(&r->endpoint, e->destination, e->words, 10, NULL);
    if (code == PTO_MSGB_OK || code == PTO_MSGB_DUPLICATE) {
      r->out_begin = (r->out_begin + 1) % PTO_SYNC_OUTBOX; --r->out_count;
    } else if (code != PTO_MSGB_BUSY && code != PTO_MSGB_LOCAL_RECOVERY) {
      r->failed = 1; status = map_transport(code); goto end;
    }
    if (r->endpoint.tx_pending) { status = WSE_BUSY; goto end; }
    r->tx_kind = 0;
  }
  /* Keep receiving even during software drain: peers still owe replies. */
  code = r->endpoint.rx_pending ? pto_msgb_rx_resume(&r->endpoint) :
                                  pto_msgb_rx_begin(&r->endpoint);
  if (code == PTO_MSGB_OK) {
    if (!r->endpoint.rx_acked) {
      if (!r->rx_dispatched) {
        status = dispatch(r, &r->endpoint.rx);
        if (status == WSE_WOULD_BLOCK) goto end;
        if (status != WSE_OK) { r->failed = 1; goto end; }
        r->rx_dispatched = 1;
      }
      code = pto_msgb_rx_ack(&r->endpoint);
      if (code != PTO_MSGB_OK && code != PTO_MSGB_DUPLICATE) {
        status = map_transport(code); goto end;
      }
    }
    r->rx_releasing = 1;
    code = pto_msgb_rx_release(&r->endpoint);
    if (code == PTO_MSGB_LOCAL_RECOVERY) r->rx_recovery = 1;
    if (code != PTO_MSGB_OK) { status = map_transport(code); goto end; }
    r->rx_dispatched = r->rx_releasing = 0;
  } else if (code != PTO_MSGB_EMPTY && code != PTO_MSGB_BUSY) {
    if (code == PTO_MSGB_LOCAL_RECOVERY) r->rx_recovery = 1;
    status = map_transport(code); goto end;
  }
  for (i = 0; i < PTO_SYNC_OBJECTS; ++i)
    if (r->objects[i].bound && r->objects[i].initialized &&
        r->objects[i].owner == r->endpoint_id) notify(r, &r->objects[i]);
  /* Local pending dependencies defer emission without blocking the caller. */
  if (!r->out_count) for (i = 0; i < PTO_SYNC_SLOTS; ++i) {
    pto_sync_slot *s = &r->slots[i], *dep;
    uint64_t words[10];
    if (!s->live || s->is_event || s->status != WSE_PENDING || s->sent) continue;
    dep = lookup(r, s->dependency);
    if (dep && dep->status == WSE_PENDING) continue;
    if (dep && dep->status != WSE_OK) { finish(r, s, dep->status); continue; }
    packet_for_slot(r, s, words);
    r->tx_kind = 2; r->tx_slot = i;
    code = pto_msgb_send(&r->endpoint, s->destination, words, 10, NULL);
    if (code == PTO_MSGB_OK || code == PTO_MSGB_DUPLICATE) s->sent = 1;
    else if (code != PTO_MSGB_BUSY && code != PTO_MSGB_LOCAL_RECOVERY)
      finish(r, s, map_transport(code));
    if (!r->endpoint.tx_pending) r->tx_kind = 0;
    break;
  }
end:
  if (r->failed) {
    for (i = 0; i < PTO_SYNC_SLOTS; ++i)
      if (r->slots[i].live && !r->slots[i].is_event) finish(r, &r->slots[i], WSE_FAILED);
    for (i = 0; i < PTO_SYNC_SLOTS; ++i)
      if (r->slots[i].live && r->slots[i].is_event) finish(r, &r->slots[i], WSE_FAILED);
  }
  r->progressing = 0; return status;
}
wse_status_t pto_sync_resume_tx(pto_sync_runtime *r) {
  int code;
  if (!r || !r->endpoint.tx_pending || !r->tx_kind) return WSE_INVALID;
  code = pto_msgb_tx_resume(&r->endpoint, NULL);
  if (code == PTO_MSGB_LOCAL_RECOVERY) return WSE_BUSY;
  if (code == PTO_MSGB_OK || code == PTO_MSGB_DUPLICATE) {
    if (r->tx_kind == 1) {
      r->out_begin = (r->out_begin + 1) % PTO_SYNC_OUTBOX; --r->out_count;
    } else r->slots[r->tx_slot].sent = 1;
  } else if (code != PTO_MSGB_BUSY) {
    if (r->tx_kind == 2) finish(r, &r->slots[r->tx_slot], map_transport(code));
    else r->failed = 1;
  }
  r->tx_kind = 0; return map_transport(code);
}
wse_status_t pto_sync_resume_ack(pto_sync_runtime *r) {
  int code;
  if (!r || !r->endpoint.ack_pending) return WSE_INVALID;
  code = pto_msgb_ack_resume(&r->endpoint);
  return map_transport(code);
}
wse_status_t pto_sync_resume_rx(pto_sync_runtime *r) {
  int code;
  if (!r || !r->rx_recovery) return WSE_INVALID;
  code = r->rx_releasing ? pto_msgb_rx_release(&r->endpoint) :
                           pto_msgb_rx_resume(&r->endpoint);
  if (code == PTO_MSGB_LOCAL_RECOVERY) return WSE_BUSY;
  r->rx_recovery = 0;
  if (r->rx_releasing && code == PTO_MSGB_OK)
    r->rx_releasing = r->rx_dispatched = 0;
  return map_transport(code);
}
void pto_sync_begin_drain(pto_sync_runtime *r) { if (r) r->draining = 1; }
int pto_sync_quiescent(const pto_sync_runtime *r) {
  unsigned i;
  if (!r || r->out_count || r->mail_count || r->endpoint.tx_pending ||
      r->endpoint.rx_pending || r->endpoint.ack_pending) return 0;
  for (i = 0; i < PTO_SYNC_SLOTS; ++i)
    if (r->slots[i].live && r->slots[i].status == WSE_PENDING) return 0;
  for (i = 0; i < PTO_SYNC_OBJECTS; ++i) if (r->objects[i].initialized &&
      r->objects[i].owner == r->endpoint_id && r->objects[i].state != WSE_RETIRED)
    return 0;
  return 1;
}
static wse_status_t submit(unsigned op, wse_request_t *request,
                            wse_event_t *event, unsigned kind,
                            pto_sync_object *o, uint32_t destination,
                            uint64_t actor, uint64_t argument,
                            wse_event_t dependency, void *output,
                            const void *payload, unsigned bytes) {
  pto_sync_runtime *r = pto_sync_current_runtime();
  pto_sync_slot *s, *e = NULL, *dep = NULL;
  if (!r || !request || bytes > PTO_SYNC_INLINE_BYTES || (bytes && !payload)) return WSE_INVALID;
  if (r->failed) return WSE_FAILED;
  if (r->draining && op != RELEASE && op != RETIRE && op != ABORT &&
      op != RESOURCE && op != ERROR_REPORT) return WSE_BUSY;
  if (dependency.opaque) {
    dep = lookup(r, dependency.opaque);
    if (!dep || !dep->is_event || !dep->caller_live) return WSE_STALE;
  }
  s = allocate(r, 0); if (!s) return WSE_WOULD_BLOCK;
  if (event) {
    e = allocate(r, 1);
    if (!e) { s->live = 0; return WSE_WOULD_BLOCK; }
    e->event_kind = (uint8_t)kind; e->destination = destination;
    e->object = o ? o->handle : 0; e->refs = 1;
    s->event = e->token;
  }
  s->opcode = (uint8_t)op; s->object = o ? o->handle : 0;
  s->destination = destination; s->actor = actor;
  s->argument = e ? e->token : argument; s->output = output;
  s->bytes = bytes; if (bytes) memcpy(s->payload, payload, bytes);
  if (dep) { ++dep->refs; s->dependency = dep->token; }
  request->opaque = s->token; if (event) event->opaque = e->token;
  return WSE_OK;
}
static wse_status_t object_submit(unsigned op, wse_request_t *req,
                                   wse_event_t *event, unsigned kind,
                                   const wse_sync_key_t *key, uint64_t actor,
                                   wse_event_t dep, void *output) {
  pto_sync_runtime *r = pto_sync_current_runtime();
  pto_sync_object *o = r ? by_key(r, key) : NULL;
  if (!o) return WSE_NOT_FOUND;
  if (op == SUBSCRIBE && actor != UINT64_MAX) {
    int index = member(o->readers, o->consumers, actor, r->endpoint_id);
    if (index >= 0 && (o->relinquished & (UINT32_C(1) << index)))
      return WSE_STALE;
  }
  if ((op == DONE || op == RELEASE || op == RESOURCE) && !dep.opaque) return WSE_INVALID;
  if (dep.opaque && (op == DONE || op == RELEASE || op == ARRIVE)) {
    pto_sync_slot *s = lookup(r, dep.opaque);
    if (!s || !s->is_event || !s->caller_live) return WSE_STALE;
    if ((op == DONE && s->event_kind != EVENT_VISIBLE) ||
        (op == RELEASE && s->event_kind != EVENT_READS) ||
        (s->event_kind != EVENT_VISIBLE && s->event_kind != EVENT_READS) ||
        !r->platform.validate ||
        !r->platform.validate(r->platform.context, s->cookie, key, op == RELEASE)) return WSE_DENIED;
  }
  return submit(op, req, event, kind, o, o->owner, actor, 0, dep, output, NULL, 0);
}
static wse_event_t no_event(void) { wse_event_t e = {0}; return e; }
static int same_data(pto_sync_runtime *r, pto_sync_object *o, const wse_sync_desc_t *d) {
  unsigned i; uint32_t endpoint;
  if (!d || o->group || !region_equal(o->region, d->region) ||
      o->count != d->contribution_count || o->consumers != d->consumer_count ||
      (o->count && !d->contributions) || (o->consumers && !d->consumers)) return 0;
  for (i = 0; i < o->count; ++i)
    if (!resolve(r, d->contributions[i].producer, &endpoint) ||
        o->producers[i].identity != d->contributions[i].contribution_id ||
        o->producers[i].endpoint != endpoint) return 0;
  for (i = 0; i < o->consumers; ++i)
    if (!resolve(r, d->consumers[i].consumer, &endpoint) ||
        o->readers[i].identity != d->consumers[i].consumer_slot ||
        o->readers[i].endpoint != endpoint) return 0;
  return 1;
}
wse_status_t __sync_init_async(wse_request_t *req, const wse_sync_desc_t *d) {
  pto_sync_runtime *r = pto_sync_current_runtime();
  pto_sync_object *o = r && d ? by_key(r, &d->key) : NULL;
  if (!o) return WSE_NOT_FOUND;
  if (!same_data(r, o, d)) return WSE_CONFLICT;
  if (r->endpoint_id != o->owner) return WSE_DENIED;
  return object_submit(INIT, req, NULL, 0, &d->key, 0, no_event(), NULL);
}
wse_status_t __sync_group_init_async(wse_request_t *req, const wse_group_desc_t *d) {
  pto_sync_runtime *r = pto_sync_current_runtime(); unsigned i; uint32_t endpoint;
  pto_sync_object *o = r && d ? by_key(r, &d->key) : NULL;
  if (!o) return WSE_NOT_FOUND;
  if (!o->group || !d->members || o->count != d->member_count) return WSE_CONFLICT;
  for (i = 0; i < o->count; ++i)
    if (!resolve(r, d->members[i].endpoint, &endpoint) ||
        o->producers[i].identity != d->members[i].member_id ||
        o->producers[i].endpoint != endpoint) return WSE_CONFLICT;
  if (r->endpoint_id != o->owner) return WSE_DENIED;
  return object_submit(INIT, req, NULL, 0, &d->key, 0, no_event(), NULL);
}
wse_status_t __sync_query_async(wse_sync_snapshot_t *out, wse_request_t *req,
                                const wse_sync_key_t *key) {
  if (!out) return WSE_INVALID;
  return object_submit(QUERY, req, NULL, 0, key, 0, no_event(), out);
}
wse_status_t __sync_retire_async(wse_request_t *req, const wse_sync_key_t *key) {
  return object_submit(RETIRE, req, NULL, 0, key, 0, no_event(), NULL);
}
wse_status_t __sync_subscribe_async(wse_event_t *ev, wse_request_t *req,
                                    const wse_sync_key_t *key, uint64_t consumer) {
  if (!ev || consumer == UINT64_MAX) return WSE_INVALID;
  return object_submit(SUBSCRIBE, req, ev, EVENT_READY, key, consumer, no_event(), NULL);
}
wse_status_t __sync_watch_free_async(wse_event_t *ev, wse_request_t *req,
                                     const wse_sync_key_t *key) {
  if (!ev) return WSE_INVALID;
  return object_submit(SUBSCRIBE, req, ev, EVENT_FREE, key, UINT64_MAX, no_event(), NULL);
}
wse_status_t __sync_done_release_async(wse_request_t *req, const wse_sync_key_t *key,
                                       uint64_t contribution, wse_event_t visible) {
  return object_submit(DONE, req, NULL, 0, key, contribution, visible, NULL);
}
wse_status_t __sync_release_async(wse_request_t *req, const wse_sync_key_t *key,
                                  uint64_t consumer, wse_event_t reads) {
  wse_status_t status = object_submit(RELEASE, req, NULL, 0, key, consumer, reads, NULL);
  if (status == WSE_OK) {
    pto_sync_runtime *r = pto_sync_current_runtime();
    pto_sync_object *o = by_key(r, key);
    int index = member(o->readers, o->consumers, consumer, r->endpoint_id);
    if (index >= 0) o->relinquished |= UINT32_C(1) << index;
  }
  return status;
}
wse_status_t __sync_group_arrive_async(wse_event_t *ev, wse_request_t *req,
                                       const wse_sync_key_t *key, uint64_t member_id,
                                       const wse_event_t *prior) {
  if (!ev) return WSE_INVALID;
  return object_submit(ARRIVE, req, ev, EVENT_GROUP, key, member_id,
                       prior ? *prior : no_event(), NULL);
}
wse_status_t __sync_abort_async(wse_event_t *ev, wse_request_t *req,
                                const wse_sync_key_t *key, uint32_t reason) {
  if (!ev) return WSE_INVALID;
  return object_submit(ABORT, req, ev, EVENT_DRAIN, key, reason, no_event(), NULL);
}
wse_status_t __sync_error_report_async(wse_request_t *req, const wse_error_report_t *e) {
  if (!e || e->code == WSE_OK || e->code == WSE_PENDING) return WSE_INVALID;
  {
    wse_status_t status = object_submit(ERROR_REPORT, req, NULL, 0, &e->key, e->report_id, no_event(), NULL);
    if (status == WSE_OK) {
      pto_sync_slot *s = lookup(pto_sync_current_runtime(), req->opaque);
      s->argument = e->code; s->detail = e->detail;
    }
    return status;
  }
}
wse_status_t __sync_task_notify_async(wse_request_t *req, const wse_task_msg_t *m) {
  pto_sync_runtime *r = pto_sync_current_runtime(); uint32_t destination;
  wse_status_t result;
  if (!r || !m || !resolve(r, m->destination, &destination)) return WSE_INVALID;
  result = submit(TASK, req, NULL, 0, NULL, destination, m->application_message_id,
                   0, no_event(), NULL, m->payload, m->payload_bytes);
  if (result == WSE_OK) lookup(r, req->opaque)->object = m->task_tag;
  return result;
}
wse_status_t __sync_resource_return_async(wse_request_t *req,
                                          const wse_resource_return_t *resource,
                                          wse_event_t done) {
  pto_sync_runtime *r = pto_sync_current_runtime(); uint32_t destination;
  pto_sync_slot *d;
  if (!r || !resource || !resolve(r, resource->owner, &destination)) return WSE_INVALID;
  d = lookup(r, done.opaque);
  if (!d || !d->is_event || !d->caller_live) return WSE_STALE;
  if (!r->platform.resource_validate ||
      !r->platform.resource_validate(r->platform.context, d->cookie,
                                      resource->resource_id, resource->lease_id)) return WSE_DENIED;
  /* Resource ownership/use coverage is additionally verified by the trusted
   * resource_return callback at its owner. */
  return submit(RESOURCE, req, NULL, 0, NULL, destination, resource->resource_id,
                  resource->lease_id, done, NULL, NULL, 0);
}
static wse_status_t test_token(uint64_t token, int event) {
  pto_sync_runtime *r = pto_sync_current_runtime(); pto_sync_slot *s;
  if (!r) return WSE_INVALID;
  s = lookup(r, token);
  if (!s || s->is_event != event || !s->caller_live) return WSE_STALE;
  return s->status;
}
wse_status_t __sync_request_test(wse_request_t req) { return test_token(req.opaque, 0); }
wse_status_t __sync_event_test_acquire(wse_event_t ev) {
  pto_sync_runtime *r = pto_sync_current_runtime();
  wse_status_t status = test_token(ev.opaque, 1);
  /* Successful platform poll/validate carries the acquire relation. A wire
   * READY is only authorization to begin the declared local data operation. */
  (void)r; return status;
}
static wse_status_t wait_token(uint64_t token, int event) {
  pto_sync_runtime *r = pto_sync_current_runtime(); wse_status_t status;
  if (!r) return WSE_INVALID;
  while ((status = test_token(token, event)) == WSE_PENDING) {
    wse_status_t progress = pto_sync_progress(r);
    if (progress == WSE_FAILED) return progress;
    if (r->endpoint.tx_pending || r->endpoint.ack_pending || r->rx_recovery) return WSE_BUSY;
    if (r->platform.yield) r->platform.yield(r->platform.context);
  }
  return status;
}
wse_status_t __sync_request_wait(wse_request_t req) { return wait_token(req.opaque, 0); }
wse_status_t __sync_event_wait_acquire(wse_event_t ev) { return wait_token(ev.opaque, 1); }
static wse_status_t release_token(uint64_t token, int event) {
  pto_sync_runtime *r = pto_sync_current_runtime(); pto_sync_slot *s;
  if (!r) return WSE_INVALID;
  s = lookup(r, token);
  if (!s || s->is_event != event || !s->caller_live) return WSE_STALE;
  if (s->status == WSE_PENDING) return WSE_BUSY;
  s->caller_live = 0; collect(r, s); return WSE_OK;
}
wse_status_t __sync_request_release(wse_request_t req) { return release_token(req.opaque, 0); }
wse_status_t __sync_event_release(wse_event_t ev) { return release_token(ev.opaque, 1); }
static wse_status_t capture(wse_event_t *event, wse_window_t window,
                             uint64_t offset, uint64_t extent, int reads) {
  pto_sync_runtime *r = pto_sync_current_runtime(); pto_sync_slot *s;
  wse_status_t status; uint64_t cookie = 0;
  if (!r || !event || !window.opaque || offset > UINT64_MAX - extent ||
      !r->platform.capture || !r->platform.poll || !r->platform.validate ||
      !r->platform.drop) return WSE_INVALID;
  if (r->failed || (r->draining && !reads)) return WSE_BUSY;
  s = allocate(r, 1); if (!s) return WSE_WOULD_BLOCK;
  status = r->platform.capture(r->platform.context, reads, window, offset, extent, &cookie);
  if (status != WSE_OK) { s->live = 0; return status; }
  s->cookie = cookie; s->event_kind = reads ? EVENT_READS : EVENT_VISIBLE;
  event->opaque = s->token; return WSE_OK;
}
wse_status_t __sync_l1_publish_async(wse_event_t *ev, wse_window_t w, uint64_t off, uint64_t n) {
  return capture(ev, w, off, n, 0);
}
wse_status_t __sync_l1_reads_complete_async(wse_event_t *ev, wse_window_t w, uint64_t off, uint64_t n) {
  return capture(ev, w, off, n, 1);
}
wse_status_t __sync_message_try_recv(void *payload, wse_message_info_t *info,
                                    uint32_t capacity, uint32_t port) {
  pto_sync_runtime *r = pto_sync_current_runtime(); pto_sync_mail *m;
  if (!r || !info || port || (capacity && !payload)) return WSE_INVALID;
  if (!r->mail_count) return WSE_PENDING;
  m = &r->mailbox[r->mail_begin];
  if (capacity < m->info.payload_bytes) { *info = m->info; return WSE_BUFFER_TOO_SMALL; }
  if (m->info.payload_bytes) memcpy(payload, m->payload, m->info.payload_bytes);
  *info = m->info; r->mail_begin = (r->mail_begin + 1) % PTO_SYNC_MAILBOX;
  --r->mail_count; return WSE_OK;
}
static wse_status_t service_emit(wse_request_t *req, wse_service_t capability,
                                  const wse_notification_t *n, unsigned op) {
  pto_sync_runtime *r = pto_sync_current_runtime(); pto_sync_object *o;
  unsigned i; int free_slot = -1;
  if (!r || !n || !req || !(o = by_key(r, &n->key))) return WSE_INVALID;
  if (o->owner != r->endpoint_id || o->capability != capability.opaque ||
      !o->initialized) return WSE_DENIED;
  if ((op == FREE && (o->group || o->state != WSE_FREE)) ||
      (op == READY && (o->group || o->state != WSE_READY)) ||
      (op == GROUP_READY && (!o->group || o->state != WSE_READY))) return WSE_BUSY;
  /* Only an existing pending notification identity may be emitted. */
  for (i = 0; i < PTO_SYNC_MEMBERS; ++i)
    if ((op != READY || !(o->released & (UINT32_C(1) << i))) &&
        (op == FREE ? o->free_token[i] : o->ready_token[i]) == n->notification_id &&
        n->notification_id) { free_slot = (int)i; break; }
  if (free_slot < 0) return WSE_NOT_FOUND;
  if (r->out_count == PTO_SYNC_OUTBOX) return WSE_WOULD_BLOCK;
  {
    pto_sync_slot *s = allocate(r, 0);
    uint32_t destination = op == FREE ? o->free_destination[free_slot] : o->ready_destination[free_slot];
    if (!s) return WSE_WOULD_BLOCK;
    enqueue(r, destination, op, WSE_OK, n->notification_id, o->handle, 0, s->token);
    if (op == FREE) o->free_sent |= UINT32_C(1) << free_slot;
    else o->ready_sent |= UINT32_C(1) << free_slot;
    s->sent = 1; s->destination = destination; s->object = o->handle;
    req->opaque = s->token;
  }
  return WSE_OK;
}
wse_status_t __sync_ready_async(wse_request_t *req, wse_service_t s, const wse_notification_t *n) {
  return service_emit(req, s, n, READY);
}
wse_status_t __sync_free_async(wse_request_t *req, wse_service_t s, const wse_notification_t *n) {
  return service_emit(req, s, n, FREE);
}
wse_status_t __sync_group_ready_async(wse_request_t *req, wse_service_t s, const wse_notification_t *n) {
  return service_emit(req, s, n, GROUP_READY);
}
