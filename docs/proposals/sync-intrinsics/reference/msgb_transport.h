#ifndef LINX_TILEOP_MSGB_TRANSPORT_H
#define LINX_TILEOP_MSGB_TRANSPORT_H

/*
 * Portable reference contract for the proposed MSGB transport.
 *
 * This file does not assign MSGBCR fields, receive-status SSRs, ACK/POP SSRs,
 * or compiler lowering. A target backend supplies those details through the
 * callbacks below. The ten staging words reflect the documented MSGBD1..10
 * register window.
 */

#include <stddef.h>
#include <stdint.h>

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
   * == 0, guarantees that no message was enqueued.
   */
  msgb_transport_status_t (*tx_bse_try_commit)(void *context, uint64_t claim,
                                               uint64_t control,
                                               uint32_t word_count,
                                               int *accepted);

  /* Release or cancel the claim. This must not enqueue a staged message. */
  void (*tx_release)(void *context, uint64_t claim);

  /*
   * Claim a stable snapshot of the receive head. A successful claim excludes
   * another receiver from claiming the same head until rx_release. The backend
   * returns its immutable identity and payload length. EMPTY and BUSY have no
   * side effect. Every other non-OK result has the same no-claim/no-mutation
   * guarantee. word_count must be 0..10. control is opaque; only the target
   * codec/backend may reject target-specific values.
   */
  msgb_transport_status_t (*rx_claim)(void *context, uint64_t *claim,
                                      uint64_t *message_id, uint64_t *control,
                                      uint32_t *word_count);

  /* Model SSRGET from one word of the claimed, stable head snapshot. */
  msgb_transport_status_t (*rx_read_msgbd)(void *context, uint64_t claim,
                                           uint32_t index, uint64_t *value);

  /*
   * Atomically ACK and pop exactly message_id under claim. OK means it was
   * dequeued exactly once. Every failure guarantees that it remains queued.
   */
  msgb_transport_status_t (*rx_ack_pop)(void *context, uint64_t claim,
                                        uint64_t message_id);

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
 * tail data. control is opaque and this layer assigns no illegal value.
 */
static inline msgb_transport_status_t
msgb_transport_send(const msgb_transport_ops_t *ops, uint64_t control,
                    const uint64_t *words, uint32_t word_count) {
  uint64_t claim = 0;
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
                                  &accepted);
  ops->tx_release(ops->context, claim);
  if (status != MSGB_TRANSPORT_OK)
    return status;
  return accepted ? MSGB_TRANSPORT_OK : MSGB_TRANSPORT_BUSY;
}

/*
 * Read a stable receive-head snapshot and explicitly ACK/POP it. On success,
 * outputs describe the dequeued message. On TOO_SMALL, only *word_count is
 * updated with the required capacity; the message remains queued. On every
 * other failure all outputs are unchanged and the message remains queued.
 *
 * output_words may be NULL only when output_capacity is zero. Pointer validity
 * and non-overlap with the scalar outputs are caller preconditions.
 */
static inline msgb_transport_status_t
msgb_transport_try_recv(const msgb_transport_ops_t *ops, uint64_t *control,
                        uint64_t *message_id, uint64_t *output_words,
                        uint32_t output_capacity, uint32_t *word_count) {
  uint64_t claim = 0;
  uint64_t snapshot_id = 0;
  uint64_t snapshot_control = 0;
  uint64_t temporary[MSGB_TRANSPORT_WORDS];
  uint32_t snapshot_count = 0;
  uint32_t index;
  msgb_transport_status_t status;

  if (!msgb_transport_ops_valid(ops) || control == NULL || message_id == NULL ||
      word_count == NULL || (output_capacity != 0 && output_words == NULL)) {
    return MSGB_TRANSPORT_INVALID;
  }

  status = ops->rx_claim(ops->context, &claim, &snapshot_id, &snapshot_control,
                         &snapshot_count);
  if (status != MSGB_TRANSPORT_OK)
    return status;

  if (snapshot_count > MSGB_TRANSPORT_WORDS) {
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

  status = ops->rx_ack_pop(ops->context, claim, snapshot_id);
  if (status != MSGB_TRANSPORT_OK) {
    ops->rx_release(ops->context, claim);
    return status;
  }

  for (index = 0; index < snapshot_count; ++index)
    output_words[index] = temporary[index];
  *control = snapshot_control;
  *message_id = snapshot_id;
  *word_count = snapshot_count;
  ops->rx_release(ops->context, claim);
  return MSGB_TRANSPORT_OK;
}

#ifdef __cplusplus
}
#endif

#endif
