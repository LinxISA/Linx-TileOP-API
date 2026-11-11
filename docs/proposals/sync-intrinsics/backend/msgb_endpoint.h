#ifndef PTO_MSGB_ENDPOINT_H
#define PTO_MSGB_ENDPOINT_H
#include "msgb_registers.h"
#include "../reference/msgb_ack.h"
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Serialized per bank by the endpoint executor, including interrupt/reentrant
 * users. No global lock is held across progress on another endpoint. */
typedef enum {
  PTO_MSGB_IO_OK, PTO_MSGB_IO_REJECTED, PTO_MSGB_IO_RECOVERY
} pto_msgb_io_status;
typedef struct {
  void *context;
  pto_msgb_io_status (*read)(void *, uint32_t, uint64_t *);
  pto_msgb_io_status (*write)(void *, uint32_t, uint64_t);
  pto_msgb_io_status (*send)(void *, uint64_t);
} pto_msgb_io;
typedef struct {
  uint64_t control, token;
  msgb_transport_message_id_t id;
  uint64_t words[PTO_MSGB_WORDS];
  uint32_t count;
} pto_msgb_packet;
typedef struct {
  pto_msgb_io io;
  /* A RECOVERY return means effect unknown, not rejected. Do not call send or
   * claim again. The platform must resume the exact instruction occurrence,
   * then call the appropriate resume function with exclusive bank ownership. */
  int tx_pending, rx_pending, ack_pending, rx_acked;
  uint64_t tx_control, tx_words[PTO_MSGB_WORDS];
  uint32_t tx_stage_next;
  int tx_issued;
  pto_msgb_packet rx;
} pto_msgb_endpoint;
enum {
  PTO_MSGB_LOCAL_TOO_SMALL = 0x100, PTO_MSGB_LOCAL_RECOVERY,
  PTO_MSGB_LOCAL_INVALID
};
static inline int pto_msgb_io_valid(pto_msgb_io io) {
  return io.read && io.write && io.send;
}
static inline int pto_msgb_result(uint64_t stat) {
  if (!(stat & PTO_MSGB_VALID) || (stat & ~UINT64_C(0x1ff)) ||
      (stat & 255) > PTO_MSGB_IO_ERROR || !(stat & 255))
    return PTO_MSGB_LOCAL_RECOVERY;
  return (int)(stat & 255);
}
static inline void pto_msgb_endpoint_init(pto_msgb_endpoint *ep, pto_msgb_io io) {
  memset(ep, 0, sizeof(*ep)); ep->io = io;
}
static inline int pto_msgb_tx_resume(pto_msgb_endpoint *ep,
                                    msgb_transport_message_id_t *id) {
  uint64_t stat, hi, lo;
  int result;
  if (!ep || !ep->tx_pending) return PTO_MSGB_LOCAL_INVALID;
  while (ep->tx_stage_next < PTO_MSGB_WORDS) {
    uint32_t index = ep->tx_stage_next++;
    pto_msgb_io_status io = ep->io.write(ep->io.context,
        PTO_MSGBD1 + index, ep->tx_words[index]);
    if (io == PTO_MSGB_IO_RECOVERY) return PTO_MSGB_LOCAL_RECOVERY;
    if (io != PTO_MSGB_IO_OK) {
      ep->tx_pending = 0; return PTO_MSGB_IO_ERROR;
    }
  }
  if (!ep->tx_issued) {
    pto_msgb_io_status io;
    ep->tx_issued = 1;
    io = ep->io.send(ep->io.context, ep->tx_control);
    if (io == PTO_MSGB_IO_RECOVERY) return PTO_MSGB_LOCAL_RECOVERY;
    if (io != PTO_MSGB_IO_OK) {
      ep->tx_pending = 0; return PTO_MSGB_IO_ERROR;
    }
  }
  if (ep->io.read(ep->io.context, PTO_MSGBTXSTAT, &stat) != PTO_MSGB_IO_OK)
    return PTO_MSGB_LOCAL_RECOVERY;
  result = pto_msgb_result(stat);
  if (result == PTO_MSGB_LOCAL_RECOVERY) return result;
  if (result == PTO_MSGB_OK || result == PTO_MSGB_DUPLICATE) {
    if (ep->io.read(ep->io.context, PTO_MSGBTXID_HI, &hi) != PTO_MSGB_IO_OK ||
        ep->io.read(ep->io.context, PTO_MSGBTXID_LO, &lo) != PTO_MSGB_IO_OK)
      return PTO_MSGB_LOCAL_RECOVERY;
    if (!(uint32_t)hi || !lo) return PTO_MSGB_LOCAL_RECOVERY;
    if (id) { id->source_endpoint = (uint32_t)(hi >> 32);
      id->source_epoch = (uint32_t)hi; id->tx_sequence = lo; }
  }
  ep->tx_pending = 0;
  return result;
}
static inline int pto_msgb_send(pto_msgb_endpoint *ep, uint32_t destination,
                               const uint64_t *words, uint32_t count,
                               msgb_transport_message_id_t *id) {
  uint32_t i; uint64_t control;
  if (!ep || !pto_msgb_io_valid(ep->io) || (count && !words) ||
      !pto_msgb_control_make(destination, count, &control))
    return PTO_MSGB_LOCAL_INVALID;
  if (ep->tx_pending) return PTO_MSGB_LOCAL_RECOVERY;
  for (i = 0; i < PTO_MSGB_WORDS; ++i)
    ep->tx_words[i] = i < count ? words[i] : 0;
  ep->tx_control = control; ep->tx_stage_next = 0;
  ep->tx_issued = 0; ep->tx_pending = 1;
  return pto_msgb_tx_resume(ep, id);
}
static inline int pto_msgb_rx_release(pto_msgb_endpoint *ep) {
  uint64_t state;
  if (!ep || !ep->rx_pending || !ep->rx.token || ep->ack_pending)
    return PTO_MSGB_LOCAL_INVALID;
  if (ep->io.write(ep->io.context, PTO_MSGBRXRELEASE, ep->rx.token) != PTO_MSGB_IO_OK ||
      ep->io.read(ep->io.context, PTO_MSGBRXSTATE, &state) != PTO_MSGB_IO_OK)
    return PTO_MSGB_LOCAL_RECOVERY;
  /* A repeated exact release after an interrupted successful release is safe.
   * No other bank user may acquire a new claim while rx_pending is set. */
  if ((state & 255) != PTO_MSGB_OK && (state & 255) != PTO_MSGB_NO_CLAIM)
    return (int)(state & 255);
  if (state & PTO_MSGB_OWNED) return PTO_MSGB_LOCAL_RECOVERY;
  ep->rx_pending = ep->rx_acked = 0;
  return PTO_MSGB_OK;
}
/* Also resumes a claim whose metadata/data read was interrupted. */
static inline int pto_msgb_rx_resume(pto_msgb_endpoint *ep) {
  uint64_t state, hi, lo; uint32_t i;
  pto_msgb_packet p;
  if (!ep || !ep->rx_pending) return PTO_MSGB_LOCAL_INVALID;
  if (ep->io.read(ep->io.context, PTO_MSGBRXSTATE, &state) != PTO_MSGB_IO_OK)
    return PTO_MSGB_LOCAL_RECOVERY;
  if (!(state & PTO_MSGB_OWNED)) {
    int result = (int)(state & 255);
    if (result == PTO_MSGB_EMPTY || result == PTO_MSGB_BUSY ||
        result == PTO_MSGB_EXHAUSTED || result == PTO_MSGB_BAD_COMMAND) {
      ep->rx_pending = 0; return result;
    }
    return PTO_MSGB_LOCAL_RECOVERY;
  }
  memset(&p, 0, sizeof(p)); p.count = (uint32_t)((state >> 12) & 15);
  if (ep->io.read(ep->io.context, PTO_MSGBRXTOKEN, &p.token) != PTO_MSGB_IO_OK)
    return PTO_MSGB_LOCAL_RECOVERY;
  if (!msgb_rx_token_valid(p.token) || (ep->rx.token && ep->rx.token != p.token))
    return PTO_MSGB_LOCAL_RECOVERY;
  ep->rx.token = p.token;
  if (p.count > PTO_MSGB_WORDS) return PTO_MSGB_BAD_COMMAND;
  if (ep->io.read(ep->io.context, PTO_MSGBRXMSGID_HI, &hi) != PTO_MSGB_IO_OK ||
      ep->io.read(ep->io.context, PTO_MSGBRXMSGID_LO, &lo) != PTO_MSGB_IO_OK ||
      ep->io.read(ep->io.context, PTO_MSGBRXCONTROL, &p.control) != PTO_MSGB_IO_OK)
    return PTO_MSGB_LOCAL_RECOVERY;
  p.id.source_endpoint = (uint32_t)(hi >> 32);
  p.id.source_epoch = (uint32_t)hi; p.id.tx_sequence = lo;
  if (!msgb_transport_message_id_valid(p.id) || !pto_msgb_control_valid(p.control) ||
      ((p.control >> 32) & 15) != p.count) return PTO_MSGB_BAD_COMMAND;
  for (i = 0; i < p.count; ++i)
    if (ep->io.read(ep->io.context, PTO_MSGBD1 + i, &p.words[i]) != PTO_MSGB_IO_OK)
      return PTO_MSGB_LOCAL_RECOVERY;
  ep->rx = p; ep->rx_acked = (state & PTO_MSGB_ACKED) != 0;
  return PTO_MSGB_OK;
}
static inline int pto_msgb_rx_begin(pto_msgb_endpoint *ep) {
  pto_msgb_io_status io;
  if (!ep || !pto_msgb_io_valid(ep->io)) return PTO_MSGB_LOCAL_INVALID;
  if (ep->rx_pending) return PTO_MSGB_LOCAL_RECOVERY;
  memset(&ep->rx, 0, sizeof(ep->rx)); ep->rx_pending = 1;
  io = ep->io.write(ep->io.context, PTO_MSGBRXCLAIM, 1);
  if (io == PTO_MSGB_IO_RECOVERY) return PTO_MSGB_LOCAL_RECOVERY;
  if (io != PTO_MSGB_IO_OK) { ep->rx_pending = 0; return PTO_MSGB_IO_ERROR; }
  return pto_msgb_rx_resume(ep);
}
static inline int pto_msgb_ack_resume(pto_msgb_endpoint *ep) {
  uint64_t stat, echo; int result;
  if (!ep || !ep->ack_pending) return PTO_MSGB_LOCAL_INVALID;
  if (ep->io.read(ep->io.context, PTO_MSGBACKSTAT, &stat) != PTO_MSGB_IO_OK ||
      ep->io.read(ep->io.context, PTO_MSGBACKID, &echo) != PTO_MSGB_IO_OK)
    return PTO_MSGB_LOCAL_RECOVERY;
  result = pto_msgb_result(stat);
  if (echo != ep->rx.token || result == PTO_MSGB_LOCAL_RECOVERY)
    return PTO_MSGB_LOCAL_RECOVERY;
  ep->ack_pending = 0;
  if (result == PTO_MSGB_OK || result == PTO_MSGB_DUPLICATE) ep->rx_acked = 1;
  return result;
}
static inline int pto_msgb_rx_ack(pto_msgb_endpoint *ep) {
  pto_msgb_io_status io;
  if (!ep || !ep->rx_pending || !ep->rx.token) return PTO_MSGB_LOCAL_INVALID;
  if (ep->ack_pending) return PTO_MSGB_LOCAL_RECOVERY;
  ep->ack_pending = 1;
  io = ep->io.write(ep->io.context, PTO_MSGBACK, ep->rx.token);
  if (io == PTO_MSGB_IO_RECOVERY) return PTO_MSGB_LOCAL_RECOVERY;
  if (io != PTO_MSGB_IO_OK) { ep->ack_pending = 0; return PTO_MSGB_IO_ERROR; }
  return pto_msgb_ack_resume(ep);
}
/* Raw application mailbox. Too-small leaves both output and queue unchanged;
 * required is the only output changed. Pending recovery retains a private copy. */
static inline int pto_msgb_try_recv(pto_msgb_endpoint *ep, pto_msgb_packet *out,
                                   uint32_t capacity, uint32_t *required) {
  int result;
  if (!ep || !out) return PTO_MSGB_LOCAL_INVALID;
  if (ep->rx_pending) return PTO_MSGB_LOCAL_RECOVERY;
  result = pto_msgb_rx_begin(ep);
  if (result != PTO_MSGB_OK) return result;
  if (required) *required = ep->rx.count;
  if (capacity < ep->rx.count) {
    result = pto_msgb_rx_release(ep);
    return result == PTO_MSGB_OK ? PTO_MSGB_LOCAL_TOO_SMALL : result;
  }
  result = pto_msgb_rx_ack(ep);
  if (result != PTO_MSGB_OK && result != PTO_MSGB_DUPLICATE) return result;
  *out = ep->rx;
  return pto_msgb_rx_release(ep);
}
#ifdef __cplusplus
}
#endif
#endif
