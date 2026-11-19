#include "reference/msgb_transport.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint64_t stage_words[MSGB_TRANSPORT_WORDS];
  uint64_t words[MSGB_TRANSPORT_WORDS];
  uint64_t control;
  msgb_transport_message_id_t transport_id;
  uint32_t word_count;
  int queued;
  int tx_claimed;
  int rx_claimed;
  uint64_t next_rx_claim;
  uint64_t rx_token;
  int fail_tx_claim;
  int fail_rx_claim;
  int fail_write_index;
  int fail_read_index;
  int fail_pop;
  int reject_commit;
  int commit_calls;
  int write_calls;
  int release_calls;
  int pop_calls;
  uint32_t source_endpoint;
  uint32_t source_epoch;
  uint64_t next_tx_sequence;
} fake_msgb_t;

static msgb_transport_status_t fake_tx_claim(void *opaque, uint64_t *claim) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  if (fake->fail_tx_claim)
    return MSGB_TRANSPORT_IO_ERROR;
  if (fake->tx_claimed)
    return MSGB_TRANSPORT_BUSY;
  fake->tx_claimed = 1;
  *claim = UINT64_C(11);
  return MSGB_TRANSPORT_OK;
}

static msgb_transport_status_t fake_tx_write(void *opaque, uint64_t claim,
                                             uint32_t index, uint64_t value) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  assert(claim == UINT64_C(11));
  assert(fake->tx_claimed);
  assert(index < MSGB_TRANSPORT_WORDS);
  ++fake->write_calls;
  if ((int)index == fake->fail_write_index)
    return MSGB_TRANSPORT_IO_ERROR;
  fake->stage_words[index] = value;
  return MSGB_TRANSPORT_OK;
}

static msgb_transport_status_t
fake_bse_commit(void *opaque, uint64_t claim, uint64_t control,
                uint32_t word_count, int *accepted,
                msgb_transport_message_id_t *id) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  uint32_t index;
  assert(claim == UINT64_C(11));
  assert(fake->tx_claimed);
  assert(fake->write_calls == (int)MSGB_TRANSPORT_WORDS);
  ++fake->commit_calls;
  if (fake->reject_commit || fake->queued) {
    *accepted = 0;
    return MSGB_TRANSPORT_OK;
  }
  fake->control = control;
  fake->transport_id.source_endpoint = fake->source_endpoint;
  fake->transport_id.source_epoch = fake->source_epoch;
  fake->transport_id.tx_sequence = ++fake->next_tx_sequence;
  *id = fake->transport_id;
  fake->word_count = word_count;
  for (index = 0; index < MSGB_TRANSPORT_WORDS; ++index)
    fake->words[index] = fake->stage_words[index];
  fake->queued = 1;
  *accepted = 1;
  return MSGB_TRANSPORT_OK;
}

static void fake_tx_release(void *opaque, uint64_t claim) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  assert(claim == UINT64_C(11));
  assert(fake->tx_claimed);
  fake->tx_claimed = 0;
  ++fake->release_calls;
}

static msgb_transport_status_t fake_rx_claim(void *opaque, uint64_t *claim,
                                             uint64_t *rx_token,
                                             msgb_transport_message_id_t *id,
                                             uint64_t *control,
                                             uint32_t *word_count) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  if (fake->fail_rx_claim)
    return MSGB_TRANSPORT_IO_ERROR;
  if (!fake->queued)
    return MSGB_TRANSPORT_EMPTY;
  if (fake->rx_claimed)
    return MSGB_TRANSPORT_BUSY;
  fake->rx_claimed = 1;
  *claim = ++fake->next_rx_claim;
  fake->rx_token = (UINT64_C(1) << 48) | *claim;
  *rx_token = fake->rx_token;
  *id = fake->transport_id;
  *control = fake->control;
  *word_count = fake->word_count;
  return MSGB_TRANSPORT_OK;
}

static msgb_transport_status_t fake_rx_read(void *opaque, uint64_t claim,
                                            uint32_t index, uint64_t *value) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  assert(claim == fake->next_rx_claim);
  assert(fake->rx_claimed && fake->queued);
  if ((int)index == fake->fail_read_index)
    return MSGB_TRANSPORT_IO_ERROR;
  *value = fake->words[index];
  return MSGB_TRANSPORT_OK;
}

static msgb_transport_status_t fake_rx_pop(void *opaque, uint64_t claim,
                                           uint64_t rx_token) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  assert(claim == fake->next_rx_claim);
  assert(fake->rx_claimed && fake->queued);
  ++fake->pop_calls;
  if (fake->fail_pop)
    return MSGB_TRANSPORT_IO_ERROR;
  if (rx_token != fake->rx_token)
    return MSGB_TRANSPORT_IO_ERROR;
  fake->queued = 0;
  return MSGB_TRANSPORT_OK;
}

static void fake_rx_release(void *opaque, uint64_t claim) {
  fake_msgb_t *fake = (fake_msgb_t *)opaque;
  assert(claim == fake->next_rx_claim);
  assert(fake->rx_claimed);
  fake->rx_claimed = 0;
  ++fake->release_calls;
}

static msgb_transport_ops_t fake_ops(fake_msgb_t *fake) {
  msgb_transport_ops_t ops;
  memset(&ops, 0, sizeof(ops));
  ops.context = fake;
  ops.tx_claim = fake_tx_claim;
  ops.tx_write_msgbd = fake_tx_write;
  ops.tx_bse_try_commit = fake_bse_commit;
  ops.tx_release = fake_tx_release;
  ops.rx_claim = fake_rx_claim;
  ops.rx_read_msgbd = fake_rx_read;
  ops.rx_ack_pop = fake_rx_pop;
  ops.rx_release = fake_rx_release;
  return ops;
}

static fake_msgb_t empty_fake(void) {
  fake_msgb_t fake;
  memset(&fake, 0, sizeof(fake));
  fake.source_endpoint = UINT32_C(0x10203040);
  fake.source_epoch = UINT32_C(7);
  fake.fail_write_index = -1;
  fake.fail_read_index = -1;
  return fake;
}

static void seed_message(fake_msgb_t *fake, uint32_t words);

static void test_send_zero_fills_and_commits_once(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  const uint64_t payload[] = {UINT64_C(3), UINT64_C(5), UINT64_C(8)};
  uint32_t index;

  assert(msgb_transport_send(&ops, UINT64_C(99), payload, 3, NULL) ==
         MSGB_TRANSPORT_OK);
  assert(fake.commit_calls == 1);
  assert(fake.write_calls == (int)MSGB_TRANSPORT_WORDS);
  assert(fake.stage_words[0] == 3 && fake.stage_words[1] == 5 &&
         fake.stage_words[2] == 8);
  for (index = 3; index < MSGB_TRANSPORT_WORDS; ++index)
    assert(fake.stage_words[index] == 0 && fake.words[index] == 0);
  assert(fake.queued && !fake.tx_claimed);
}

static void test_send_reject_and_retry(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  const uint64_t payload[] = {UINT64_C(13)};
  msgb_transport_message_id_t id = {9, 9, 9};

  fake.reject_commit = 1;
  assert(msgb_transport_send(&ops, UINT64_C(2), payload, 1, &id) ==
         MSGB_TRANSPORT_BUSY);
  assert(!fake.queued && fake.commit_calls == 1 && !fake.tx_claimed);
  assert(id.source_endpoint == 9 && id.source_epoch == 9 &&
         id.tx_sequence == 9 && fake.next_tx_sequence == 0);
  fake.reject_commit = 0;
  fake.write_calls = 0;
  assert(msgb_transport_send(&ops, UINT64_C(2), payload, 1, &id) ==
         MSGB_TRANSPORT_OK);
  assert(fake.queued && fake.commit_calls == 2 && fake.next_tx_sequence == 1);
  assert(msgb_transport_message_id_equal(id, fake.transport_id));
}

static void test_full_queue_does_not_replace_head(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  const uint64_t payload[] = {UINT64_C(55)};

  seed_message(&fake, 1);
  assert(msgb_transport_send(&ops, UINT64_C(3), payload, 1, NULL) ==
         MSGB_TRANSPORT_BUSY);
  assert(fake.queued && fake.control == 77 &&
         fake.transport_id.tx_sequence == 41);
  assert(fake.word_count == 1 && fake.words[0] == 100);
  assert(fake.commit_calls == 1 && !fake.tx_claimed);
}

static void test_send_fault_never_commits(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  const uint64_t payload[] = {UINT64_C(21)};

  fake.fail_write_index = 4;
  assert(msgb_transport_send(&ops, UINT64_C(2), payload, 1, NULL) ==
         MSGB_TRANSPORT_IO_ERROR);
  assert(fake.commit_calls == 0 && !fake.queued && !fake.tx_claimed);
}

static void test_claim_failures_do_not_deadlock_retry(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  const uint64_t payload[] = {UINT64_C(34)};
  uint64_t control = UINT64_C(7), id = UINT64_C(8), output = UINT64_C(9);
  msgb_transport_message_id_t transport_id = {7, 8, 9};
  uint32_t count = 10;

  fake.fail_tx_claim = 1;
  assert(msgb_transport_send(&ops, UINT64_C(2), payload, 1, NULL) ==
         MSGB_TRANSPORT_IO_ERROR);
  assert(!fake.tx_claimed && !fake.queued && fake.write_calls == 0 &&
         fake.commit_calls == 0);
  fake.fail_tx_claim = 0;
  assert(msgb_transport_send(&ops, UINT64_C(2), payload, 1, NULL) ==
         MSGB_TRANSPORT_OK);
  assert(fake.queued && !fake.tx_claimed);

  fake.fail_rx_claim = 1;
  assert(msgb_transport_try_recv(&ops, &control, &id, &transport_id, &output, 1,
                                 &count) == MSGB_TRANSPORT_IO_ERROR);
  assert(fake.queued && !fake.rx_claimed);
  assert(control == 7 && id == 8 && output == 9 && count == 10);
  assert(transport_id.source_endpoint == 7 && transport_id.source_epoch == 8 &&
         transport_id.tx_sequence == 9);
  fake.fail_rx_claim = 0;
  assert(msgb_transport_try_recv(&ops, &control, &id, &transport_id, &output, 1,
                                 &count) == MSGB_TRANSPORT_OK);
  assert(!fake.queued && control == 2 && output == 34 && count == 1);
}

static void seed_message(fake_msgb_t *fake, uint32_t words) {
  uint32_t index;
  fake->control = UINT64_C(77);
  fake->transport_id.source_endpoint = UINT32_C(0xabc);
  fake->transport_id.source_epoch = UINT32_C(2);
  fake->transport_id.tx_sequence = UINT64_C(41);
  fake->word_count = words;
  fake->queued = 1;
  for (index = 0; index < words; ++index)
    fake->words[index] = UINT64_C(100) + index;
}

static void test_receive_claim_exclusion(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  uint64_t claim, token, first_token, control;
  msgb_transport_message_id_t first_id, second_id;
  uint32_t count;

  seed_message(&fake, 2);
  assert(ops.rx_claim(ops.context, &claim, &token, &first_id, &control,
                      &count) == MSGB_TRANSPORT_OK);
  first_token = token;
  assert(ops.rx_claim(ops.context, &claim, &token, &second_id, &control,
                      &count) == MSGB_TRANSPORT_BUSY);
  assert(fake.queued);
  ops.rx_release(ops.context, claim);
  assert(fake.queued && !fake.rx_claimed);
  assert(ops.rx_claim(ops.context, &claim, &token, &second_id, &control,
                      &count) == MSGB_TRANSPORT_OK);
  assert(token != first_token);
  assert(msgb_transport_message_id_valid(first_id));
  assert(msgb_transport_message_id_equal(first_id, second_id));
  ops.rx_release(ops.context, claim);
}

static void test_receive_too_small_and_failures_preserve(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  uint64_t control = UINT64_C(1), id = UINT64_C(2);
  msgb_transport_message_id_t transport_id = {7, 8, 9};
  uint64_t output[3] = {UINT64_C(7), UINT64_C(8), UINT64_C(9)};
  uint32_t count = 999;

  seed_message(&fake, 3);
  assert(msgb_transport_try_recv(&ops, &control, &id, &transport_id, output, 2,
                                 &count) == MSGB_TRANSPORT_TOO_SMALL);
  assert(count == 3 && control == 1 && id == 2 && output[0] == 7);
  assert(transport_id.source_endpoint == 7 && transport_id.source_epoch == 8 &&
         transport_id.tx_sequence == 9);
  assert(fake.queued && fake.pop_calls == 0);

  count = 999;
  fake.fail_read_index = 1;
  assert(msgb_transport_try_recv(&ops, &control, &id, &transport_id, output, 3,
                                 &count) == MSGB_TRANSPORT_IO_ERROR);
  assert(count == 999 && control == 1 && id == 2 && output[0] == 7);
  assert(fake.queued && fake.pop_calls == 0);

  fake.fail_read_index = -1;
  fake.fail_pop = 1;
  assert(msgb_transport_try_recv(&ops, &control, &id, &transport_id, output, 3,
                                 &count) == MSGB_TRANSPORT_IO_ERROR);
  assert(count == 999 && control == 1 && id == 2 && output[0] == 7);
  assert(fake.queued && fake.pop_calls == 1);
}

static void test_receive_success_dequeues_once(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  uint64_t control = 0, id = 0, output[3] = {0, 0, 0};
  uint32_t count = 0;

  seed_message(&fake, 3);
  assert(msgb_transport_try_recv(&ops, &control, &id, NULL, output, 3,
                                 &count) == MSGB_TRANSPORT_OK);
  assert(control == 77 && id == ((UINT64_C(1) << 48) | 1) && count == 3);
  assert(output[0] == 100 && output[1] == 101 && output[2] == 102);
  assert(!fake.queued && fake.pop_calls == 1 && !fake.rx_claimed);
  assert(msgb_transport_try_recv(&ops, &control, &id, NULL, output, 3,
                                 &count) == MSGB_TRANSPORT_EMPTY);
  assert(fake.pop_calls == 1);
}

static void test_short_send_receive_roundtrip(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  const uint64_t input[3] = {UINT64_C(201), UINT64_C(202), UINT64_C(203)};
  uint64_t output[MSGB_TRANSPORT_WORDS];
  uint64_t control = 0, id = 0;
  msgb_transport_message_id_t sent_id = {0, 0, 0}, received_id = {0, 0, 0};
  uint32_t count = 0, index;

  for (index = 0; index < MSGB_TRANSPORT_WORDS; ++index)
    output[index] = UINT64_C(999);
  assert(msgb_transport_send(&ops, UINT64_C(55), input, 3, &sent_id) ==
         MSGB_TRANSPORT_OK);
  assert(fake.word_count == 3);
  for (index = 3; index < MSGB_TRANSPORT_WORDS; ++index)
    assert(fake.words[index] == 0);
  assert(msgb_transport_try_recv(&ops, &control, &id, &received_id, output,
                                 MSGB_TRANSPORT_WORDS,
                                 &count) == MSGB_TRANSPORT_OK);
  assert(control == 55 && count == 3);
  assert(sent_id.source_endpoint == UINT32_C(0x10203040));
  assert(sent_id.source_epoch == 7 && sent_id.tx_sequence == 1);
  assert(msgb_transport_message_id_equal(sent_id, received_id));
  assert(output[0] == 201 && output[1] == 202 && output[2] == 203);
  for (index = 3; index < MSGB_TRANSPORT_WORDS; ++index)
    assert(output[index] == 999);
}

static void test_zero_word_send_receive(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  uint64_t control = 0, id = 0;
  uint32_t count = 99;

  assert(msgb_transport_send(&ops, UINT64_C(0), NULL, 0, NULL) ==
         MSGB_TRANSPORT_OK);
  assert(fake.word_count == 0);
  assert(msgb_transport_try_recv(&ops, &control, &id, NULL, NULL, 0, &count) ==
         MSGB_TRANSPORT_OK);
  assert(control == 0 && count == 0 && !fake.queued);
}

static void test_invalid_and_bounds(void) {
  fake_msgb_t fake = empty_fake();
  msgb_transport_ops_t ops = fake_ops(&fake);
  uint64_t words[MSGB_TRANSPORT_WORDS] = {0};
  uint64_t control = 0, id = 0;
  msgb_transport_message_id_t transport_id = {3, 4, 5};
  uint32_t count = 0;
  msgb_transport_ops_t missing_callback = ops;

  assert(msgb_transport_send(&ops, 1, NULL, 1, NULL) == MSGB_TRANSPORT_INVALID);
  assert(msgb_transport_send(&ops, 1, words, MSGB_TRANSPORT_WORDS + 1, NULL) ==
         MSGB_TRANSPORT_TOO_LARGE);
  assert(msgb_transport_send(&ops, 1, words, MSGB_TRANSPORT_WORDS, NULL) ==
         MSGB_TRANSPORT_OK);
  fake.queued = 0;
  assert(msgb_transport_try_recv(&ops, &control, &id, NULL, NULL, 1, &count) ==
         MSGB_TRANSPORT_INVALID);
  assert(msgb_transport_try_recv(&ops, NULL, &id, NULL, words,
                                 MSGB_TRANSPORT_WORDS,
                                 &count) == MSGB_TRANSPORT_INVALID);
  missing_callback.tx_bse_try_commit = NULL;
  assert(msgb_transport_send(&missing_callback, 1, words, 1, NULL) ==
         MSGB_TRANSPORT_INVALID);
  assert(msgb_transport_send(NULL, 1, words, 1, NULL) ==
         MSGB_TRANSPORT_INVALID);

  seed_message(&fake, 1);
  fake.transport_id.source_epoch = 0;
  assert(msgb_transport_try_recv(&ops, &control, &id, &transport_id, words,
                                 MSGB_TRANSPORT_WORDS,
                                 &count) == MSGB_TRANSPORT_IO_ERROR);
  assert(fake.queued && !fake.rx_claimed);
  assert(transport_id.source_endpoint == 3 && transport_id.source_epoch == 4 &&
         transport_id.tx_sequence == 5);
}

int main(void) {
  test_send_zero_fills_and_commits_once();
  test_send_reject_and_retry();
  test_full_queue_does_not_replace_head();
  test_send_fault_never_commits();
  test_claim_failures_do_not_deadlock_retry();
  test_receive_claim_exclusion();
  test_receive_too_small_and_failures_preserve();
  test_receive_success_dequeues_once();
  test_short_send_receive_roundtrip();
  test_zero_word_send_receive();
  test_invalid_and_bounds();
  return 0;
}
