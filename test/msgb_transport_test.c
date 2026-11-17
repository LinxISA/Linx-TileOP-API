#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The scripted replies and register numbers are independent of the header. */
enum {
    DATA1 = 0x831, CLAIM = 0x83b, STATE = 0x83c, TOKEN = 0x83d,
    RX_HI = 0x83e, RX_LO = 0x83f, CONTROL = 0x840, RELEASE = 0x841,
    ACK = 0x842, ACK_STAT = 0x843, ACK_ID = 0x844, TX_HI = 0x845,
    TX_LO = 0x846, TX_STAT = 0x847, ENDPOINT = 0x848, BSE = 0
};
static uint64_t reply[ENDPOINT + 1];
struct access_record { char kind; unsigned reg; uint64_t value; };
static struct access_record trace[64];
static size_t used;

static void record(char kind, unsigned reg, uint64_t value) {
    assert(used < sizeof(trace) / sizeof(trace[0]));
    trace[used].kind = kind;
    trace[used].reg = reg;
    trace[used++].value = value;
}
static inline uint64_t linx_msgb_io_read(unsigned reg) {
    assert(reg <= ENDPOINT);
    record('r', reg, reply[reg]);
    return reply[reg];
}
static inline void linx_msgb_io_write(unsigned reg, uint64_t value) {
    record('w', reg, value);
}
static inline void linx_msgb_io_send(uint64_t control) {
    record('b', BSE, control);
}
static inline void linx_msgb_io_stage(unsigned index, uint64_t value) {
    assert(index < 10);
    linx_msgb_io_write(DATA1 + index, value);
}
static inline uint64_t linx_msgb_io_load(unsigned index) {
    assert(index < 10);
    return linx_msgb_io_read(DATA1 + index);
}

#include <common/detail/linx_msgb_transport.h>

static size_t seen(char kind, unsigned reg) {
    size_t n = 0;
    for (size_t i = 0; i < used; ++i)
        if (trace[i].kind == kind && trace[i].reg == reg) ++n;
    return n;
}
static size_t position(char kind, unsigned reg) {
    for (size_t i = 0; i < used; ++i)
        if (trace[i].kind == kind && trace[i].reg == reg) return i;
    assert(0 && "missing MSGB access");
    return used;
}
static void reset_bank(void) {
    memset(reply, 0, sizeof(reply));
    used = 0;
}

static void test_send(void) {
    const uint32_t destination = UINT32_C(0xf1234567);
    uint64_t words[10];
    linx_msgb_id id;
    for (unsigned i = 0; i < 10; ++i) words[i] = UINT64_MAX - i;
    reset_bank();
    id.hi = 7; id.lo = 8;
    assert(linx_msgb_try_send(destination, words, 11, &id) == LINX_MSGB_BAD_COMMAND);
    assert(linx_msgb_try_send(destination, NULL, 1, &id) == LINX_MSGB_BAD_COMMAND);
    assert(linx_msgb_try_send(destination, words, 1, NULL) == LINX_MSGB_BAD_COMMAND);
    assert(used == 0 && id.hi == 7 && id.lo == 8);

    const size_t widths[] = {0, 1, 10};
    for (size_t k = 0; k < 3; ++k) {
        const size_t n = widths[k];
        reset_bank();
        reply[TX_STAT] = 0x101; /* valid OK */
        reply[TX_HI] = UINT64_C(0xaffe000011112222);
        reply[TX_LO] = UINT64_C(0x9876543210abcdef);
        assert(linx_msgb_try_send(destination, n ? words : NULL, n, &id) == LINX_MSGB_OK);
        assert(id.hi == reply[TX_HI] && id.lo == reply[TX_LO]);
        const size_t submit = position('b', BSE);
        assert(seen('b', BSE) == 1 && trace[submit].value == (((uint64_t)n << 32) | destination));
        assert(seen('w', CLAIM) == (size_t)(n == 0));
        if (!n) {
            size_t preflight = position('w', CLAIM);
            assert(preflight < submit && trace[preflight].value == 0);
        }
        for (unsigned i = 0; i < 10; ++i) {
            assert(seen('w', DATA1 + i) == (size_t)(i < n));
            if (i < n) {
                size_t stage = position('w', DATA1 + i);
                assert(stage < submit && trace[stage].value == words[i]);
            }
        }
        assert(submit < position('r', TX_STAT));
        assert(position('r', TX_STAT) < position('r', TX_HI));
        assert(position('r', TX_STAT) < position('r', TX_LO));
    }

    id.hi = 7; id.lo = 8;
    reply[TX_STAT] = 0x104; /* BUSY leaves a previous TXID readable */
    used = 0;
    assert(linx_msgb_try_send(destination, NULL, 0, &id) == LINX_MSGB_BUSY);
    assert(id.hi == 7 && id.lo == 8 && seen('b', BSE) == 1);
    assert(!seen('r', TX_HI) && !seen('r', TX_LO));
    reply[TX_STAT] = 1; /* OK with VALID clear */
    assert(linx_msgb_try_send(destination, NULL, 0, &id) == LINX_MSGB_IO_ERROR);
    assert(id.hi == 7 && id.lo == 8);
    reply[TX_STAT] = 0x102; /* hardware replay of one admitted occurrence */
    used = 0;
    assert(linx_msgb_try_send(destination, NULL, 0, &id) == LINX_MSGB_DUPLICATE);
    assert(id.hi == reply[TX_HI] && id.lo == reply[TX_LO] && seen('b', BSE) == 1);
}

static void test_claim(void) {
    linx_msgb_message msg;
    unsigned char before[sizeof(msg)];
    memset(&msg, 0xa5, sizeof(msg));
    memcpy(before, &msg, sizeof(msg));
    reset_bank();
    assert(linx_msgb_try_claim(NULL) == LINX_MSGB_BAD_COMMAND && !used);
    reply[STATE] = 0x1104; /* BUSY plus the previous owned one-word claim */
    assert(linx_msgb_try_claim(&msg) == LINX_MSGB_BUSY);
    assert(seen('w', CLAIM) == 1 && trace[position('w', CLAIM)].value == 1);
    assert(!seen('r', TOKEN) && !seen('r', DATA1));
    assert(memcmp(before, &msg, sizeof(msg)) == 0);
    reply[STATE] = 3; /* EMPTY */
    assert(linx_msgb_try_claim(&msg) == LINX_MSGB_EMPTY);
    assert(memcmp(before, &msg, sizeof(msg)) == 0);
    assert(linx_msgb_read_claim(&msg) == LINX_MSGB_NO_CLAIM);
    used = 0;
    reply[STATE] = 11; /* queue remains present, claim counter exhausted */
    assert(linx_msgb_try_claim(&msg) == LINX_MSGB_EXHAUSTED);
    assert(used == 2 && !seen('w', RELEASE));
    assert(memcmp(before, &msg, sizeof(msg)) == 0);
    used = 0;
    reply[STATE] = 0x101 | (UINT64_C(11) << 12);
    reply[CONTROL] = UINT64_C(11) << 32;
    assert(linx_msgb_try_claim(&msg) == LINX_MSGB_IO_ERROR);
    assert(memcmp(before, &msg, sizeof(msg)) == 0 && !seen('r', DATA1));
    reply[STATE] = 0x1101;
    reply[CONTROL] = UINT64_C(2) << 32;
    assert(linx_msgb_read_claim(&msg) == LINX_MSGB_IO_ERROR);
    assert(memcmp(before, &msg, sizeof(msg)) == 0 && !seen('r', DATA1));

    const size_t widths[] = {0, 1, 10};
    for (size_t k = 0; k < 3; ++k) {
        const size_t n = widths[k];
        reset_bank();
        reply[ENDPOINT] = UINT32_C(0x98765432);
        reply[STATE] = ((uint64_t)n << 12) | 0x101;
        reply[CONTROL] = ((uint64_t)n << 32) | reply[ENDPOINT];
        reply[TOKEN] = UINT64_C(0xff01000000112233);
        reply[RX_HI] = UINT64_C(0xcdefabcd12345678);
        reply[RX_LO] = UINT64_MAX;
        for (unsigned i = 0; i < n; ++i) reply[DATA1 + i] = UINT64_MAX - i;
        assert(linx_msgb_try_claim(&msg) == LINX_MSGB_OK);
        assert(msg.count == n && !msg.acked && msg.destination == reply[ENDPOINT]);
        assert(msg.claim.token == reply[TOKEN] && msg.claim.endpoint == reply[ENDPOINT]);
        assert(msg.id.hi == reply[RX_HI] && msg.id.lo == reply[RX_LO]);
        assert(position('w', CLAIM) < position('r', STATE));
        for (unsigned i = 0; i < 10; ++i) {
            assert(seen('r', DATA1 + i) == (size_t)(i < n));
            if (i < n) {
                assert(position('r', STATE) < position('r', DATA1 + i));
                assert(msg.words[i] == UINT64_MAX - i);
            }
        }
        assert(!seen('w', ACK) && !seen('w', RELEASE));

        used = 0;
        reply[STATE] = ((uint64_t)n << 12) | 0x304; /* still owned/acked after BUSY */
        assert(linx_msgb_read_claim(&msg) == LINX_MSGB_OK);
        assert(msg.acked && msg.claim.token == reply[TOKEN] && msg.count == n);
        assert(!seen('w', CLAIM) && !seen('w', ACK) && !seen('w', RELEASE));
    }
}

static void test_ack_release(void) {
    linx_msgb_claim claim;
    claim.endpoint = 42;
    claim.token = UINT64_C(0xabcdeeff01234567);
    reset_bank();
    reply[ENDPOINT] = 41;
    assert(linx_msgb_ack(claim) == LINX_MSGB_NOT_OWNER && !seen('w', ACK));
    assert(linx_msgb_release(claim) == LINX_MSGB_NOT_OWNER && !seen('w', RELEASE));

    reply[ENDPOINT] = claim.endpoint;
    reply[ACK_ID] = claim.token;
    reply[ACK_STAT] = 0x101;
    used = 0;
    assert(linx_msgb_ack(claim) == LINX_MSGB_OK);
    size_t ack = position('w', ACK);
    assert(trace[ack].value == claim.token && ack < position('r', ACK_STAT));
    assert(ack < position('r', ACK_ID) && !seen('w', RELEASE));
    reply[ACK_STAT] = 0x102;
    assert(linx_msgb_ack(claim) == LINX_MSGB_DUPLICATE);
    reply[ACK_ID] = claim.token ^ (UINT64_C(1) << 40);
    assert(linx_msgb_ack(claim) == LINX_MSGB_IO_ERROR);
    reply[ACK_ID] = claim.token;
    reply[ACK_STAT] = 1;
    assert(linx_msgb_ack(claim) == LINX_MSGB_IO_ERROR);
    reply[ACK_STAT] = 0x10a;
    assert(linx_msgb_ack(claim) == LINX_MSGB_NOT_READ);
    assert(!seen('w', RELEASE));
    used = 0;
    reply[STATE] = 1;
    assert(linx_msgb_release(claim) == LINX_MSGB_OK);
    size_t release = position('w', RELEASE);
    assert(trace[release].value == claim.token && release < position('r', STATE));
    assert(!seen('r', ACK_STAT) && !seen('w', ACK));
    reply[STATE] = 5;
    assert(linx_msgb_release(claim) == LINX_MSGB_NO_CLAIM);
}

int main(void) {
    test_send();
    test_claim();
    test_ack_release();
    return 0;
}
