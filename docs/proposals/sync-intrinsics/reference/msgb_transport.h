#ifndef LINX_TILEOP_MSGB_TRANSPORT_H
#define LINX_TILEOP_MSGB_TRANSPORT_H

/*
 * Portable reference contract for the proposed MSGB transport.
 *
 * The SSRSET ACK register profile is specified in msgb-ssr-ack.md and modeled
 * separately in msgb_ack.h. This adapter assigns no numeric SSR addresses,
 * MSGBCR codec or compiler lowering. The ten staging words reflect MSGBD1..10.
 */

#include <stddef.h>
#include <stdint.h>

#include "msgb_ack.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MSGB_TRANSPORT_WORDS 10u

typedef enum {
  MSGB_TRANSPORT_OK = 0,
  MSGB_TRANSPORT_EMPTY,
  MSGB_TRANSPORT_BUSY,
  MSGB_TRANSPORT_TOO_LARGE,
  MSGB_TRANSPORT_TOO_SMALL,
  MSGB_TRANSPORT_INVALID,
  MSGB_TRANSPORT_IO_ERROR
} msgb_transport_status_t;

typedef struct {
  void *context;

  /*
   * Claim one PE-private (or otherwise exclusive) MSGBD staging bank. Every
   * non-OK result guarantees that no claim was acquired and no queue changed.
   */
  msgb_transport_status_t (*tx_claim)(void *context, uint64_t *claim);

  /* Model SSRSET to MSGBD[index + 1]. index is always 0..9. */
  msgb_transport_status_t (*tx_write_msgbd)(void *context, uint64_t claim,
                                            uint32_t index, uint64_t value);

  /*
   * Model BSE as the sole atomic send-commit point. control is opaque and is
   * passed to the target backend for MSGBCR codec validation. word_count is the
   * logical payload length and lets the backend verify that codec without this
   * reference assigning any field bits. On OK, accepted reports whether the
   * complete staged message was enqueued. Any other result, or OK with accepted
   * == 0, guarantees that no message was enqueued. An accepted commit returns
   * the stable 128-bit Transport Message ID assigned at this point; it must be
   * valid according to msgb_transport_message_id_valid().
   */
  msgb_transport_status_t (*tx_bse_try_commit)(void *context, uint64_t claim,
                                               uint64_t control,
                                               uint32_t word_count,
                                               int *accepted,
                                               msgb_transport_message_id_t *id);

  /* Release or cancel the claim. This must not enqueue a staged message. */
  void (*tx_release)(void *context, uint64_t claim);

  /*
   * Claim a stable snapshot of the receive head. A successful claim excludes
   * another receiver from claiming the same head until rx_release. The backend
   * returns a fresh claim-specific RX token, the stable 128-bit Transport
   * Message ID, and payload length. Reclaiming the same message allocates a
   * different RX token while preserving that Transport Message ID. EMPTY and
   * BUSY have no side effect. Every other non-OK result has the same
   * no-claim/no-mutation guarantee. word_count must be 0..10. control is
   * opaque; only the target codec/backend may reject target-specific values.
   */
  msgb_transport_status_t (*rx_claim)(void *context, uint64_t *claim,
                                      uint64_t *rx_token,
                                      msgb_transport_message_id_t *id,
                                      uint64_t *control, uint32_t *word_count);

  /* Model SSRGET from one word of the claimed, stable head snapshot. */
  msgb_transport_status_t (*rx_read_msgbd)(void *context, uint64_t claim,
                                           uint32_t index, uint64_t *value);

  /*
   * Bind to SSRSET MSGBACK with rx_token, then read ACKSTAT/ACKID. OK means
   * this exact claim's consumption succeeded (or is an idempotent replay),
   * never a second pop. Every failure guarantees no dequeue by this attempt.
   */
  msgb_transport_status_t (*rx_ack_pop)(void *context, uint64_t claim,
                                        uint64_t rx_token);

  /* Drop only the local snapshot claim. This must not dequeue the message. */
  void (*rx_release)(void *context, uint64_t claim);
} msgb_transport_ops_t;

static inline int msgb_transport_ops_valid(const msgb_transport_ops_t *ops) {
  return ops != NULL && ops->tx_claim != NULL && ops->tx_write_msgbd != NULL &&
         ops->tx_bse_try_commit != NULL && ops->tx_release != NULL &&
         ops->rx_claim != NULL && ops->rx_read_msgbd != NULL &&
         ops->rx_ack_pop != NULL && ops->rx_release != NULL;
}

/*
 * Stage MSGBD1..10 with SSRSET, then invoke BSE once as the atomic commit.
 * Every staging slot is initialized, so a short message cannot expose stale
 * tail data. control is opaque and this layer assigns no illegal value. The
 * optional Transport Message ID output is written only after accepted success.
 */
static inline msgb_transport_status_t
msgb_transport_send(const msgb_transport_ops_t *ops, uint64_t control,
                    const uint64_t *words, uint32_t word_count,
                    msgb_transport_message_id_t *transport_id) {
  uint64_t claim = 0;
  msgb_transport_message_id_t accepted_id;
  msgb_transport_status_t status;
  uint32_t index;
  int accepted = 0;

  if (!msgb_transport_ops_valid(ops) || word_count > MSGB_TRANSPORT_WORDS ||
      (word_count != 0 && words == NULL)) {
    return word_count > MSGB_TRANSPORT_WORDS ? MSGB_TRANSPORT_TOO_LARGE
                                             : MSGB_TRANSPORT_INVALID;
  }

  status = ops->tx_claim(ops->context, &claim);
  if (status != MSGB_TRANSPORT_OK)
    return status;

  for (index = 0; index < MSGB_TRANSPORT_WORDS; ++index) {
    const uint64_t value = index < word_count ? words[index] : UINT64_C(0);
    status = ops->tx_write_msgbd(ops->context, claim, index, value);
    if (status != MSGB_TRANSPORT_OK) {
      ops->tx_release(ops->context, claim);
      return status;
    }
  }

  status = ops->tx_bse_try_commit(ops->context, claim, control, word_count,
                                  &accepted, &accepted_id);
  ops->tx_release(ops->context, claim);
  if (status != MSGB_TRANSPORT_OK)
    return status;
  if (!accepted)
    return MSGB_TRANSPORT_BUSY;
  if (transport_id != NULL)
    *transport_id = accepted_id;
  return MSGB_TRANSPORT_OK;
}

/*
 * Read a stable receive-head snapshot and explicitly ACK/POP it. On success,
 * outputs describe the dequeued message. transport_id is the stable 128-bit
 * message identity, while rx_token is the claim-specific MSGBACK operand. On
 * TOO_SMALL, only *word_count is
 * updated with the required capacity; the message remains queued. On every
 * other failure all outputs are unchanged and the message remains queued.
 *
 * output_words may be NULL only when output_capacity is zero. Pointer validity
 * and non-overlap with the scalar outputs are caller preconditions.
 */
static inline msgb_transport_status_t msgb_transport_try_recv(
    const msgb_transport_ops_t *ops, uint64_t *control, uint64_t *rx_token,
    msgb_transport_message_id_t *transport_id, uint64_t *output_words,
    uint32_t output_capacity, uint32_t *word_count) {
  uint64_t claim = 0;
  uint64_t snapshot_token = 0;
  uint64_t snapshot_control = 0;
  msgb_transport_message_id_t snapshot_id;
  uint64_t temporary[MSGB_TRANSPORT_WORDS];
  uint32_t snapshot_count = 0;
  uint32_t index;
  msgb_transport_status_t status;

  if (!msgb_transport_ops_valid(ops) || control == NULL || rx_token == NULL ||
      word_count == NULL || (output_capacity != 0 && output_words == NULL)) {
    return MSGB_TRANSPORT_INVALID;
  }

  status = ops->rx_claim(ops->context, &claim, &snapshot_token, &snapshot_id,
                         &snapshot_control, &snapshot_count);
  if (status != MSGB_TRANSPORT_OK)
    return status;

  if (snapshot_count > MSGB_TRANSPORT_WORDS) {
    ops->rx_release(ops->context, claim);
    return MSGB_TRANSPORT_IO_ERROR;
  }
  if (!msgb_transport_message_id_valid(snapshot_id)) {
    ops->rx_release(ops->context, claim);
    return MSGB_TRANSPORT_IO_ERROR;
  }

  if (output_capacity < snapshot_count) {
    ops->rx_release(ops->context, claim);
    *word_count = snapshot_count;
    return MSGB_TRANSPORT_TOO_SMALL;
  }

  for (index = 0; index < snapshot_count; ++index) {
    status = ops->rx_read_msgbd(ops->context, claim, index, &temporary[index]);
    if (status != MSGB_TRANSPORT_OK) {
      ops->rx_release(ops->context, claim);
      return status;
    }
  }

  status = ops->rx_ack_pop(ops->context, claim, snapshot_token);
  if (status != MSGB_TRANSPORT_OK) {
    ops->rx_release(ops->context, claim);
    return status;
  }

  for (index = 0; index < snapshot_count; ++index)
    output_words[index] = temporary[index];
  *control = snapshot_control;
  *rx_token = snapshot_token;
  if (transport_id != NULL)
    *transport_id = snapshot_id;
  *word_count = snapshot_count;
  ops->rx_release(ops->context, claim);
  return MSGB_TRANSPORT_OK;
}

#ifdef __cplusplus
}
#endif

#endif
