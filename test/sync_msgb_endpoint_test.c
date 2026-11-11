#include "backend/msgb_linx_backend.h"
#include <assert.h>
#include <string.h>
typedef struct {
  msgb_ack_model_t rx;
  msgb_ack_context_t owner;
  uint64_t staging[10], stat, hi, lo;
  unsigned sends, fail_ack_read, uncertain_send, uncertain_stage;
} fake;
static pto_msgb_io_status rd(void *opaque, uint32_t a, uint64_t *v) {
  fake *f = (fake *)opaque;
  if (a == PTO_MSGBACKSTAT && f->fail_ack_read) {
    f->fail_ack_read = 0; return PTO_MSGB_IO_RECOVERY;
  }
  switch (a) {
  case PTO_MSGBTXSTAT: *v = f->stat; break;
  case PTO_MSGBTXID_HI: *v = f->hi; break;
  case PTO_MSGBTXID_LO: *v = f->lo; break;
  case PTO_MSGBRXSTATE: *v = msgb_ssrget_rxstate(&f->rx, &f->owner); break;
  case PTO_MSGBRXTOKEN: *v = msgb_ssrget_rxtoken(&f->rx, &f->owner); break;
  case PTO_MSGBRXMSGID_HI: *v = msgb_transport_message_id_hi(f->rx.snapshot.message.message_id); break;
  case PTO_MSGBRXMSGID_LO: *v = msgb_transport_message_id_lo(f->rx.snapshot.message.message_id); break;
  case PTO_MSGBRXCONTROL: *v = f->rx.snapshot.message.control; break;
  case PTO_MSGBACKSTAT: *v = msgb_ssrget_msgbackstat(&f->owner); break;
  case PTO_MSGBACKID: *v = msgb_ssrget_msgbackid(&f->owner); break;
  default:
    if (a < PTO_MSGBD1 || a > PTO_MSGBD10 ||
        msgb_ssrget_msgbd(&f->rx, &f->owner, a - PTO_MSGBD1, v) != MSGB_ACK_OK)
      return PTO_MSGB_IO_REJECTED;
  }
  return PTO_MSGB_IO_OK;
}
static pto_msgb_io_status wr(void *opaque, uint32_t a, uint64_t v) {
  fake *f = (fake *)opaque;
  if (a >= PTO_MSGBD1 && a <= PTO_MSGBD10) f->staging[a - PTO_MSGBD1] = v;
  else if (a == PTO_MSGBRXCLAIM) (void)msgb_ssrset_rxclaim(&f->rx, &f->owner, v);
  else if (a == PTO_MSGBACK) (void)msgb_ssrset_msgback(&f->rx, &f->owner, v);
  else if (a == PTO_MSGBRXRELEASE) (void)msgb_ssrset_rxrelease(&f->rx, &f->owner, v);
  else return PTO_MSGB_IO_REJECTED;
  if (a == PTO_MSGBD1 && f->uncertain_stage) {
    f->uncertain_stage = 0; return PTO_MSGB_IO_RECOVERY;
  }
  return PTO_MSGB_IO_OK;
}
static pto_msgb_io_status send(void *opaque, uint64_t control) {
  fake *f = (fake *)opaque;
  msgb_transport_message_id_t id = {7, 1, f->lo + 1};
  ++f->sends;
  if (f->rx.queue_head.valid) { f->stat = 0x104; return PTO_MSGB_IO_OK; }
  assert(msgb_ack_enqueue(&f->rx, id, control, f->staging,
                           (uint32_t)((control >> 32) & 15)) == MSGB_ACK_OK);
  f->hi = msgb_transport_message_id_hi(id); f->lo = id.tx_sequence;
  f->stat = 0x101;
  return f->uncertain_send ? PTO_MSGB_IO_RECOVERY : PTO_MSGB_IO_OK;
}
int main(void) {
  fake f; pto_msgb_endpoint ep; pto_msgb_packet packet, unchanged;
  msgb_transport_message_id_t id = {0, 0, 0};
  uint64_t data[10], control = 55; uint32_t required = 0; unsigned i;
  pto_msgb_io io;
  memset(&f, 0, sizeof(f)); memset(&packet, 0xa5, sizeof(packet)); unchanged = packet;
  assert(msgb_ack_model_init(&f.rx, 1) == MSGB_ACK_OK);
  msgb_ack_context_init(&f.owner, 9);
  io.context = &f; io.read = rd; io.write = wr; io.send = send;
  pto_msgb_endpoint_init(&ep, io);
  assert(!pto_msgb_control_make(0, 11, &control) && control == 55);
  assert(!pto_msgb_control_valid(UINT64_C(1) << 36));
  assert(!pto_msgb_config_make(0, 1, &control));
  for (i = 0; i < 10; ++i) data[i] = i + 20;
  f.uncertain_send = 1;
  assert(pto_msgb_send(&ep, 7, data, 10, &id) == PTO_MSGB_LOCAL_RECOVERY);
  assert(id.tx_sequence == 0);
  assert(pto_msgb_send(&ep, 7, data, 10, &id) == PTO_MSGB_LOCAL_RECOVERY);
  assert(f.sends == 1);
  assert(pto_msgb_tx_resume(&ep, &id) == PTO_MSGB_OK && id.tx_sequence == 1);
  assert(pto_msgb_try_recv(&ep, &packet, 9, &required) == PTO_MSGB_LOCAL_TOO_SMALL);
  assert(required == 10 && !memcmp(&packet, &unchanged, sizeof(packet)));
  assert(f.rx.queue_head.valid && !f.rx.snapshot.valid);
  assert(pto_msgb_rx_begin(&ep) == PTO_MSGB_OK);
  f.fail_ack_read = 1;
  assert(pto_msgb_rx_ack(&ep) == PTO_MSGB_LOCAL_RECOVERY);
  assert(!f.rx.queue_head.valid && f.rx.snapshot.acked);
  assert(pto_msgb_rx_begin(&ep) == PTO_MSGB_LOCAL_RECOVERY);
  assert(pto_msgb_ack_resume(&ep) == PTO_MSGB_OK);
  assert(pto_msgb_rx_resume(&ep) == PTO_MSGB_OK && ep.rx_acked);
  for (i = 0; i < 10; ++i) assert(ep.rx.words[i] == data[i]);
  assert(pto_msgb_rx_release(&ep) == PTO_MSGB_OK);
  assert(pto_msgb_rx_begin(&ep) == PTO_MSGB_EMPTY);
  f.uncertain_send = 0;
  assert(pto_msgb_send(&ep, 7, data, 0, &id) == PTO_MSGB_OK);
  assert(pto_msgb_try_recv(&ep, &packet, 0, &required) == PTO_MSGB_OK);
  assert(packet.count == 0 && packet.id.tx_sequence == 2);
  assert(pto_msgb_send(&ep, 7, data, 1, &id) == PTO_MSGB_OK);
  assert(pto_msgb_send(&ep, 7, data, 1, &id) == PTO_MSGB_BUSY);
  assert(f.lo == 3);
  assert(pto_msgb_try_recv(&ep, &packet, 1, &required) == PTO_MSGB_OK);
  assert(packet.words[0] == 20);
  for (i = 1; i < 10; ++i) assert(f.staging[i] == 0);
  f.uncertain_stage = 1;
  i = f.sends;
  assert(pto_msgb_send(&ep, 7, data, 1, &id) == PTO_MSGB_LOCAL_RECOVERY);
  assert(f.sends == i && ep.tx_pending && !ep.tx_issued);
  assert(pto_msgb_send(&ep, 7, data, 1, &id) == PTO_MSGB_LOCAL_RECOVERY);
  assert(pto_msgb_tx_resume(&ep, &id) == PTO_MSGB_OK);
  assert(f.sends == i + 1);
  assert(pto_msgb_try_recv(&ep, &packet, 1, &required) == PTO_MSGB_OK);
  return 0;
}
