#include "reference/msgb_ack.h"

#include <assert.h>
#include <stdint.h>

static msgb_transport_message_id_t message_id(uint32_t endpoint, uint32_t epoch,
                                              uint64_t sequence) {
  msgb_transport_message_id_t id = {endpoint, epoch, sequence};
  return id;
}

static void init(msgb_ack_model_t *model, msgb_ack_context_t *a,
                 msgb_ack_context_t *b) {
  assert(msgb_ack_model_init(model, UINT16_C(0x1234)) == MSGB_ACK_OK);
  msgb_ack_context_init(a, UINT64_C(0xa1));
  msgb_ack_context_init(b, UINT64_C(0xb2));
}

static uint64_t claim(msgb_ack_model_t *model, msgb_ack_context_t *context) {
  assert(msgb_ssrset_rxclaim(model, context, 1) == MSGB_ACK_OK);
  return msgb_ssrget_rxtoken(model, context);
}

static void read_all(msgb_ack_model_t *model, msgb_ack_context_t *context) {
  uint32_t index;
  uint64_t value;
  for (index = 0; index < model->snapshot.message.word_count; ++index)
    assert(msgb_ssrget_msgbd(model, context, index, &value) == MSGB_ACK_OK);
}

static void test_fields_and_success_duplicate(void) {
  msgb_ack_model_t model;
  msgb_ack_context_t a, b;
  const uint64_t words[] = {7, 9};
  msgb_transport_message_id_t id = message_id(
      UINT32_C(0x89abcdef), UINT32_C(0x10203040), UINT64_C(0x123456789abcdef0));
  msgb_transport_message_id_t observed;
  uint64_t token;

  init(&model, &a, &b);
  assert(msgb_rx_token_make(0, 1) == 0);
  assert(msgb_rx_token_make(1, 0) == 0);
  assert(msgb_rx_token_make(1, UINT64_C(0x0001000000000001)) == 0);
  assert(msgb_rx_token_make(UINT16_C(0xffff), UINT64_C(0x0000ffffffffffff)) ==
         UINT64_MAX);
  assert(msgb_transport_message_id_valid(message_id(0, 1, 1)));
  assert(!msgb_transport_message_id_valid(message_id(1, 0, 1)));
  assert(!msgb_transport_message_id_valid(message_id(1, 1, 0)));
  assert(msgb_transport_message_id_hi(id) == UINT64_C(0x89abcdef10203040));
  assert(msgb_transport_message_id_lo(id) == UINT64_C(0x123456789abcdef0));
  assert(msgb_ack_enqueue(&model, id, 5, words, 2) == MSGB_ACK_OK);
  token = claim(&model, &a);
  assert(token == UINT64_C(0x1234000000000001));
  assert(msgb_rx_token_epoch(token) == UINT16_C(0x1234));
  assert(msgb_rx_token_claim_sequence(token) == 1);
  assert(msgb_ssrget_rxstate(&model, &a) ==
         (UINT64_C(2) << 12 | UINT64_C(1) << 8 | MSGB_ACK_OK));
  assert(msgb_ssrget_rxmessage_id(&model, &a, &observed) == MSGB_ACK_OK);
  assert(msgb_transport_message_id_equal(id, observed));
  read_all(&model, &a);
  assert(msgb_ssrset_msgback(&model, &a, token) == MSGB_ACK_OK);
  assert(msgb_ssrget_msgbackstat(&a) == UINT64_C(0x101));
  assert(msgb_ssrget_msgbackid(&a) == token);
  assert(
      msgb_ssrget_rxstate(&model, &a) ==
      (UINT64_C(2) << 12 | UINT64_C(1) << 9 | UINT64_C(1) << 8 | MSGB_ACK_OK));
  assert(msgb_ssrset_msgback(&model, &a, token) == MSGB_ACK_DUPLICATE);
  assert(msgb_ssrget_msgbackstat(&a) == UINT64_C(0x102));
}

static void test_partial_wrong_owner_and_mismatch(void) {
  msgb_ack_model_t model;
  msgb_ack_context_t a, b;
  const uint64_t words[] = {1, 2};
  uint64_t token, value;

  init(&model, &a, &b);
  assert(msgb_ack_enqueue(&model, message_id(0, 1, 1), 0, words, 2) ==
         MSGB_ACK_OK);
  token = claim(&model, &a);
  assert(msgb_ssrget_msgbd(&model, &a, 0, &value) == MSGB_ACK_OK);
  assert(msgb_ssrset_msgback(&model, &a, token) == MSGB_ACK_NOT_READ);
  assert(model.queue_head.valid && model.snapshot.valid);
  assert(msgb_ssrset_msgback(&model, &b, token) == MSGB_ACK_NOT_OWNER);
  assert(msgb_ssrget_msgbackstat(&a) == (UINT64_C(1) << 8 | MSGB_ACK_NOT_READ));
  assert(msgb_ssrget_msgbackstat(&b) ==
         (UINT64_C(1) << 8 | MSGB_ACK_NOT_OWNER));
  assert(msgb_ssrset_msgback(&model, &a, token + 1) == MSGB_ACK_TOKEN_MISMATCH);
  assert(model.queue_head.valid && !model.snapshot.acked);
}

static void test_cancel_reclaim_fresh_token(void) {
  msgb_ack_model_t model;
  msgb_ack_context_t a, b;
  const uint64_t word = 4;
  msgb_transport_message_id_t id = message_id(7, 9, 11);
  uint64_t old_token, new_token;

  init(&model, &a, &b);
  assert(msgb_ack_enqueue(&model, id, 0, &word, 1) == MSGB_ACK_OK);
  old_token = claim(&model, &a);
  assert(msgb_ssrset_rxrelease(&model, &a, old_token) == MSGB_ACK_OK);
  new_token = claim(&model, &a);
  assert(new_token != old_token);
  assert(
      msgb_transport_message_id_equal(id, model.snapshot.message.message_id));
  assert(msgb_ssrset_msgback(&model, &a, old_token) == MSGB_ACK_TOKEN_MISMATCH);
  assert(model.queue_head.valid && model.snapshot.token == new_token);
}

static void test_previous_success_duplicate_preserves_new_claim(void) {
  msgb_ack_model_t model;
  msgb_ack_context_t a, b;
  const uint64_t first = 1, second = 2;
  uint64_t old_token, new_token;

  init(&model, &a, &b);
  assert(msgb_ack_enqueue(&model, message_id(1, 1, 1), 0, &first, 1) ==
         MSGB_ACK_OK);
  old_token = claim(&model, &a);
  read_all(&model, &a);
  assert(msgb_ssrset_msgback(&model, &a, old_token) == MSGB_ACK_OK);
  assert(msgb_ssrset_msgback(&model, &a, 0) == MSGB_ACK_BAD_TOKEN);
  assert(a.last_success_token == old_token);
  assert(msgb_ssrget_msgbackid(&a) == 0);
  assert(msgb_ssrget_msgbackstat(&a) ==
         (UINT64_C(1) << 8 | MSGB_ACK_BAD_TOKEN));
  assert(msgb_ssrset_rxrelease(&model, &a, old_token) == MSGB_ACK_OK);
  assert(msgb_ack_enqueue(&model, message_id(1, 1, 2), 0, &second, 1) ==
         MSGB_ACK_OK);
  new_token = claim(&model, &a);
  assert(msgb_ssrset_msgback(&model, &a, old_token) == MSGB_ACK_DUPLICATE);
  assert(model.queue_head.valid && model.snapshot.valid);
  assert(model.snapshot.token == new_token && !model.snapshot.acked);
}

static void test_acked_snapshot_and_new_head(void) {
  msgb_ack_model_t model;
  msgb_ack_context_t a, b;
  const uint64_t first = 31, second = 32;
  uint64_t token, value = 0;

  init(&model, &a, &b);
  assert(msgb_ack_enqueue(&model, message_id(1, 1, 1), 0, &first, 1) ==
         MSGB_ACK_OK);
  token = claim(&model, &a);
  read_all(&model, &a);
  assert(msgb_ssrset_msgback(&model, &a, token) == MSGB_ACK_OK);
  assert(msgb_ack_enqueue(&model, message_id(2, 1, 1), 0, &second, 1) ==
         MSGB_ACK_OK);
  assert(msgb_ssrget_msgbd(&model, &a, 0, &value) == MSGB_ACK_OK);
  assert(value == first);
  assert(msgb_ssrset_rxclaim(&model, &b, 1) == MSGB_ACK_BUSY);
  assert(msgb_ssrset_rxrelease(&model, &a, token + 1) ==
         MSGB_ACK_TOKEN_MISMATCH);
  assert(model.snapshot.valid);
  assert(msgb_ssrset_rxrelease(&model, &a, token) == MSGB_ACK_OK);
  assert(claim(&model, &b) != token);
  assert(model.snapshot.message.words[0] == second);
}

static void test_zero_words_reserved_and_context_banks(void) {
  msgb_ack_model_t model;
  msgb_ack_context_t a, b;
  uint64_t token;

  init(&model, &a, &b);
  assert(msgb_ssrset_rxclaim(&model, &a, 2) == MSGB_ACK_BAD_COMMAND);
  assert(!model.snapshot.valid);
  assert(msgb_ack_enqueue(&model, message_id(0, 1, 1), 0, NULL, 0) ==
         MSGB_ACK_OK);
  token = claim(&model, &a);
  assert(msgb_ssrset_msgback(&model, &a, token) == MSGB_ACK_OK);
  assert(msgb_ssrget_msgbackstat(&b) == 0);
  assert(msgb_ssrget_msgbackid(&b) == 0);
  assert(msgb_ssrset_msgback(&model, &b, token) == MSGB_ACK_NOT_OWNER);
  assert(msgb_ssrget_msgbackstat(&a) == (UINT64_C(1) << 8 | MSGB_ACK_OK));
  assert(msgb_ssrget_msgbackstat(&b) ==
         (UINT64_C(1) << 8 | MSGB_ACK_NOT_OWNER));
  assert(msgb_ssrget_msgbackid(&a) == token);
  assert(msgb_ssrget_msgbackid(&b) == token);
}

static void test_epoch_and_exhaustion(void) {
  msgb_ack_model_t model;
  msgb_ack_model_t max_epoch_model;
  msgb_ack_context_t a, b;
  const uint64_t word = 1;
  uint64_t old_token, drain_token;

  init(&model, &a, &b);
  assert(msgb_ack_enqueue(&model, message_id(0, 1, 1), 0, &word, 1) ==
         MSGB_ACK_OK);
  old_token = claim(&model, &a);
  assert(msgb_ssrset_rxrelease(&model, &a, old_token) == MSGB_ACK_OK);
  assert(msgb_ack_advance_epoch(&model, UINT16_C(0x1235)) == MSGB_ACK_BUSY);
  drain_token = claim(&model, &a);
  read_all(&model, &a);
  assert(msgb_ssrset_msgback(&model, &a, drain_token) == MSGB_ACK_OK);
  assert(msgb_ssrset_rxrelease(&model, &a, drain_token) == MSGB_ACK_OK);
  assert(msgb_ack_advance_epoch(&model, UINT16_C(0x1236)) ==
         MSGB_ACK_BAD_COMMAND);
  assert(msgb_ack_advance_epoch(&model, UINT16_C(0x1235)) == MSGB_ACK_OK);
  assert(msgb_ssrset_msgback(&model, &a, old_token) == MSGB_ACK_STALE_EPOCH);

  assert(msgb_ack_enqueue(&model, message_id(0, 1, 2), 0, &word, 1) ==
         MSGB_ACK_OK);
  model.claim_sequence = MSGB_ACK_CLAIM_SEQUENCE_MASK;
  assert(msgb_ssrset_rxclaim(&model, &a, 1) == MSGB_ACK_EXHAUSTED);
  assert(model.queue_head.valid && !model.snapshot.valid);

  assert(msgb_ack_model_init(&max_epoch_model, UINT16_MAX) == MSGB_ACK_OK);
  assert(msgb_ack_advance_epoch(&max_epoch_model, UINT16_C(1)) ==
         MSGB_ACK_EXHAUSTED);
}

int main(void) {
  test_fields_and_success_duplicate();
  test_partial_wrong_owner_and_mismatch();
  test_cancel_reclaim_fresh_token();
  test_previous_success_duplicate_preserves_new_claim();
  test_acked_snapshot_and_new_head();
  test_zero_words_reserved_and_context_banks();
  test_epoch_and_exhaustion();
  return 0;
}
