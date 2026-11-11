#ifndef LINX_TILEOP_MSGB_ACK_H
#define LINX_TILEOP_MSGB_ACK_H

/*
 * Bounded executable reference for the proposed SSRSET MSGBACK contract.
 *
 * This model intentionally has one receive queue slot and one stable snapshot.
 * It assigns no physical SSR numbers, privilege encoding, wire format, queue
 * depth, or HSCB retry policy. Named functions below model the corresponding
 * SSRSET/SSRGET operations and their architectural state transitions.
 * Calls are serialized architectural transitions, not a host-thread-safe queue.
 * Context objects represent unique trusted bindings and must not be
 * reinitialized or moved between endpoints while their claims/replay records
 * are live.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSGB_ACK_WORDS 10u
#define MSGB_ACK_CLAIM_SEQUENCE_MASK UINT64_C(0x0000ffffffffffff)
#define MSGB_ACK_EPOCH_SHIFT 48u

typedef enum {
  MSGB_ACK_NONE = 0x00,
  MSGB_ACK_OK = 0x01,
  MSGB_ACK_DUPLICATE = 0x02,
  MSGB_ACK_EMPTY = 0x03,
  MSGB_ACK_BUSY = 0x04,
  MSGB_ACK_NO_CLAIM = 0x05,
  MSGB_ACK_NOT_OWNER = 0x06,
  MSGB_ACK_TOKEN_MISMATCH = 0x07,
  MSGB_ACK_STALE_EPOCH = 0x08,
  MSGB_ACK_BAD_TOKEN = 0x09,
  MSGB_ACK_NOT_READ = 0x0a,
  MSGB_ACK_EXHAUSTED = 0x0b,
  MSGB_ACK_BAD_COMMAND = 0x0c,
  MSGB_ACK_IO_ERROR = 0x0d
} msgb_ack_result_t;

typedef struct {
  uint32_t source_endpoint;
  uint32_t source_epoch;
  uint64_t tx_sequence;
} msgb_transport_message_id_t;

typedef struct {
  /* Unique trusted {domain, endpoint, PE, context-incarnation} binding. */
  uint64_t owner_identity;
  uint8_t ack_result;
  uint8_t ack_valid;
  uint64_t ack_id;
  uint8_t rx_result;
  uint64_t last_success_token;
} msgb_ack_context_t;

typedef struct {
  uint8_t valid;
  msgb_transport_message_id_t message_id;
  uint64_t control;
  uint64_t words[MSGB_ACK_WORDS];
  uint32_t word_count;
} msgb_ack_message_t;

typedef struct {
  uint8_t valid;
  uint8_t acked;
  uint64_t owner_identity;
  uint64_t token;
  uint16_t read_mask;
  msgb_ack_message_t message;
} msgb_ack_snapshot_t;

typedef struct {
  uint16_t rx_epoch;
  uint64_t claim_sequence;
  msgb_ack_message_t queue_head;
  msgb_ack_snapshot_t snapshot;
} msgb_ack_model_t;

static inline int
msgb_transport_message_id_valid(msgb_transport_message_id_t id) {
  return id.source_epoch != 0 && id.tx_sequence != 0;
}

static inline uint64_t
msgb_transport_message_id_hi(msgb_transport_message_id_t id) {
  return ((uint64_t)id.source_endpoint << 32) | id.source_epoch;
}

static inline uint64_t
msgb_transport_message_id_lo(msgb_transport_message_id_t id) {
  return id.tx_sequence;
}

static inline int
msgb_transport_message_id_equal(msgb_transport_message_id_t left,
                                msgb_transport_message_id_t right) {
  return left.source_endpoint == right.source_endpoint &&
         left.source_epoch == right.source_epoch &&
         left.tx_sequence == right.tx_sequence;
}

static inline uint64_t msgb_rx_token_make(uint16_t epoch,
                                          uint64_t claim_sequence) {
  if (epoch == 0 || claim_sequence == 0 ||
      claim_sequence > MSGB_ACK_CLAIM_SEQUENCE_MASK)
    return UINT64_C(0);
  return ((uint64_t)epoch << MSGB_ACK_EPOCH_SHIFT) | claim_sequence;
}

static inline uint16_t msgb_rx_token_epoch(uint64_t token) {
  return (uint16_t)(token >> MSGB_ACK_EPOCH_SHIFT);
}

static inline uint64_t msgb_rx_token_claim_sequence(uint64_t token) {
  return token & MSGB_ACK_CLAIM_SEQUENCE_MASK;
}

static inline int msgb_rx_token_valid(uint64_t token) {
  return msgb_rx_token_epoch(token) != 0 &&
         msgb_rx_token_claim_sequence(token) != 0;
}

static inline void msgb_ack_context_init(msgb_ack_context_t *context,
                                         uint64_t owner_identity) {
  memset(context, 0, sizeof(*context));
  context->owner_identity = owner_identity;
}

/* Initialization is only for a new, externally isolated namespace. Warm reset
 * uses advance_epoch and must not clear live state or restart a used epoch.
 */
static inline msgb_ack_result_t msgb_ack_model_init(msgb_ack_model_t *model,
                                                    uint16_t rx_epoch) {
  if (model == NULL || rx_epoch == 0)
    return MSGB_ACK_BAD_TOKEN;
  memset(model, 0, sizeof(*model));
  model->rx_epoch = rx_epoch;
  return MSGB_ACK_OK;
}

/* Testable queue ingress boundary. It models a decoded, deduplicated message.
 */
static inline msgb_ack_result_t
msgb_ack_enqueue(msgb_ack_model_t *model, msgb_transport_message_id_t id,
                 uint64_t control, const uint64_t *words, uint32_t word_count) {
  uint32_t index;
  if (model == NULL || !msgb_transport_message_id_valid(id) ||
      word_count > MSGB_ACK_WORDS || (word_count != 0 && words == NULL))
    return MSGB_ACK_BAD_COMMAND;
  if (model->queue_head.valid)
    return MSGB_ACK_BUSY;
  memset(&model->queue_head, 0, sizeof(model->queue_head));
  model->queue_head.valid = 1;
  model->queue_head.message_id = id;
  model->queue_head.control = control;
  model->queue_head.word_count = word_count;
  for (index = 0; index < word_count; ++index)
    model->queue_head.words[index] = words[index];
  return MSGB_ACK_OK;
}

/* Model SSRSET MSGBRXCLAIM. Only bit 0 is assigned. */
static inline msgb_ack_result_t msgb_ssrset_rxclaim(msgb_ack_model_t *model,
                                                    msgb_ack_context_t *context,
                                                    uint64_t command) {
  uint64_t sequence;
  if (model == NULL || context == NULL)
    return MSGB_ACK_BAD_COMMAND;
  if ((command & ~UINT64_C(1)) != 0) {
    context->rx_result = MSGB_ACK_BAD_COMMAND;
    return MSGB_ACK_BAD_COMMAND;
  }
  if (command == 0)
    return MSGB_ACK_NONE;
  if (model->snapshot.valid) {
    context->rx_result = MSGB_ACK_BUSY;
    return MSGB_ACK_BUSY;
  }
  if (!model->queue_head.valid) {
    context->rx_result = MSGB_ACK_EMPTY;
    return MSGB_ACK_EMPTY;
  }
  if (model->claim_sequence == MSGB_ACK_CLAIM_SEQUENCE_MASK) {
    context->rx_result = MSGB_ACK_EXHAUSTED;
    return MSGB_ACK_EXHAUSTED;
  }
  sequence = model->claim_sequence + 1;
  if (sequence == 0 || sequence > MSGB_ACK_CLAIM_SEQUENCE_MASK) {
    context->rx_result = MSGB_ACK_EXHAUSTED;
    return MSGB_ACK_EXHAUSTED;
  }
  model->claim_sequence = sequence;
  memset(&model->snapshot, 0, sizeof(model->snapshot));
  model->snapshot.valid = 1;
  model->snapshot.owner_identity = context->owner_identity;
  model->snapshot.token = msgb_rx_token_make(model->rx_epoch, sequence);
  model->snapshot.message = model->queue_head;
  context->rx_result = MSGB_ACK_OK;
  return MSGB_ACK_OK;
}

/* Model SSRGET MSGBRXSTATE. */
static inline uint64_t msgb_ssrget_rxstate(const msgb_ack_model_t *model,
                                           const msgb_ack_context_t *context) {
  uint64_t state = context->rx_result;
  if (model->snapshot.valid &&
      model->snapshot.owner_identity == context->owner_identity) {
    state |= UINT64_C(1) << 8;
    if (model->snapshot.acked)
      state |= UINT64_C(1) << 9;
    state |= (uint64_t)model->snapshot.message.word_count << 12;
  }
  return state;
}

static inline uint64_t msgb_ssrget_rxtoken(const msgb_ack_model_t *model,
                                           const msgb_ack_context_t *context) {
  return model->snapshot.valid &&
                 model->snapshot.owner_identity == context->owner_identity
             ? model->snapshot.token
             : UINT64_C(0);
}

static inline msgb_ack_result_t
msgb_ssrget_rxmessage_id(const msgb_ack_model_t *model,
                         const msgb_ack_context_t *context,
                         msgb_transport_message_id_t *id) {
  if (!model->snapshot.valid)
    return MSGB_ACK_NO_CLAIM;
  if (model->snapshot.owner_identity != context->owner_identity)
    return MSGB_ACK_NOT_OWNER;
  *id = model->snapshot.message.message_id;
  return MSGB_ACK_OK;
}

/* Model SSRGET MSGBD[index + 1]; ACKED snapshots remain readable. */
static inline msgb_ack_result_t msgb_ssrget_msgbd(msgb_ack_model_t *model,
                                                  msgb_ack_context_t *context,
                                                  uint32_t index,
                                                  uint64_t *value) {
  if (model == NULL || context == NULL || value == NULL)
    return MSGB_ACK_BAD_COMMAND;
  if (!model->snapshot.valid)
    return MSGB_ACK_NO_CLAIM;
  if (model->snapshot.owner_identity != context->owner_identity)
    return MSGB_ACK_NOT_OWNER;
  if (index >= model->snapshot.message.word_count)
    return MSGB_ACK_BAD_COMMAND;
  *value = model->snapshot.message.words[index];
  model->snapshot.read_mask |= (uint16_t)(UINT16_C(1) << index);
  return MSGB_ACK_OK;
}

static inline uint16_t msgb_ack_required_read_mask(uint32_t word_count) {
  return word_count == 0 ? UINT16_C(0)
                         : (uint16_t)((UINT16_C(1) << word_count) - 1);
}

/* Model SSRSET MSGBACK <- token. */
static inline msgb_ack_result_t msgb_ssrset_msgback(msgb_ack_model_t *model,
                                                    msgb_ack_context_t *context,
                                                    uint64_t token) {
  msgb_ack_result_t result;
  if (model == NULL || context == NULL)
    return MSGB_ACK_BAD_COMMAND;
  if (!msgb_rx_token_valid(token))
    result = MSGB_ACK_BAD_TOKEN;
  else if (msgb_rx_token_epoch(token) != model->rx_epoch)
    result = MSGB_ACK_STALE_EPOCH;
  else if (context->last_success_token == token)
    result = MSGB_ACK_DUPLICATE;
  else if (!model->snapshot.valid)
    result = MSGB_ACK_NO_CLAIM;
  else if (model->snapshot.owner_identity != context->owner_identity)
    result = MSGB_ACK_NOT_OWNER;
  else if (model->snapshot.token != token)
    result = MSGB_ACK_TOKEN_MISMATCH;
  else if (model->snapshot.read_mask !=
           msgb_ack_required_read_mask(model->snapshot.message.word_count))
    result = MSGB_ACK_NOT_READ;
  else if (!model->queue_head.valid ||
           !msgb_transport_message_id_equal(model->queue_head.message_id,
                                            model->snapshot.message.message_id))
    result = MSGB_ACK_IO_ERROR;
  else {
    model->queue_head.valid = 0;
    model->snapshot.acked = 1;
    context->last_success_token = token;
    result = MSGB_ACK_OK;
  }
  context->ack_result = (uint8_t)result;
  context->ack_valid = 1;
  context->ack_id = token;
  return result;
}

/* Model SSRGET MSGBACKSTAT and MSGBACKID. */
static inline uint64_t
msgb_ssrget_msgbackstat(const msgb_ack_context_t *context) {
  return (uint64_t)context->ack_result |
         ((uint64_t)(context->ack_valid != 0) << 8);
}

static inline uint64_t
msgb_ssrget_msgbackid(const msgb_ack_context_t *context) {
  return context->ack_id;
}

/* Model SSRSET MSGBRXRELEASE <- token. */
static inline msgb_ack_result_t
msgb_ssrset_rxrelease(msgb_ack_model_t *model, msgb_ack_context_t *context,
                      uint64_t token) {
  msgb_ack_result_t result;
  if (model == NULL || context == NULL)
    return MSGB_ACK_BAD_COMMAND;
  if (!msgb_rx_token_valid(token))
    result = MSGB_ACK_BAD_TOKEN;
  else if (msgb_rx_token_epoch(token) != model->rx_epoch)
    result = MSGB_ACK_STALE_EPOCH;
  else if (!model->snapshot.valid)
    result = MSGB_ACK_NO_CLAIM;
  else if (model->snapshot.owner_identity != context->owner_identity)
    result = MSGB_ACK_NOT_OWNER;
  else if (model->snapshot.token != token)
    result = MSGB_ACK_TOKEN_MISMATCH;
  else {
    memset(&model->snapshot, 0, sizeof(model->snapshot));
    result = MSGB_ACK_OK;
  }
  context->rx_result = (uint8_t)result;
  return result;
}

/* Controlled epoch advance: only the immediate nonzero next epoch is legal. */
static inline msgb_ack_result_t msgb_ack_advance_epoch(msgb_ack_model_t *model,
                                                       uint16_t next_epoch) {
  if (model == NULL || next_epoch == 0)
    return MSGB_ACK_BAD_TOKEN;
  if (model->queue_head.valid || model->snapshot.valid)
    return MSGB_ACK_BUSY;
  if (model->rx_epoch == UINT16_MAX)
    return MSGB_ACK_EXHAUSTED;
  if (next_epoch != (uint16_t)(model->rx_epoch + 1))
    return MSGB_ACK_BAD_COMMAND;
  model->rx_epoch = next_epoch;
  model->claim_sequence = 0;
  return MSGB_ACK_OK;
}

#ifdef __cplusplus
}
#endif

#endif
