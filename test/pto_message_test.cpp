#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

// One receiver with a single queue slot. The scripted failures let the tests
// observe the queue, snapshot and caller's output on either side of ACK.
namespace {
constexpr uint32_t endpoint = 0xa1234567u;
struct Mailbox {
    std::array<uint64_t, 10> staging{}, payload{};
    uint64_t control = 0, token = UINT64_C(1) << 48, echo = 0;
    unsigned tx = 0, rx = 0, ack = 0, ack_attempts = 0, releases = 0;
    unsigned ack_error = 0, release_error = 0;
    bool queued = false, owned = false, consumed = false;
} bank;
}

static inline void linx_msgb_io_stage(unsigned index, uint64_t value) {
    bank.staging.at(index) = value;
}
static inline void linx_msgb_io_send(uint64_t control) {
    assert(static_cast<uint32_t>(control) == endpoint && (control >> 32) <= 10);
    if (bank.queued) {
        bank.tx = 4; // BUSY
        return;
    }
    bank.payload = bank.staging;
    bank.control = control;
    bank.queued = true;
    bank.tx = 1;
}
static inline uint64_t linx_msgb_io_load(unsigned index) {
    assert(bank.owned && index < (bank.control >> 32));
    return bank.payload.at(index);
}
static inline uint64_t linx_msgb_io_read(unsigned reg) {
    switch (reg) {
    case 0x83c: return bank.rx | (bank.owned ? 0x100u : 0u) |
                       (bank.consumed ? 0x200u : 0u) |
                       (bank.owned ? (bank.control >> 32) << 12 : 0u);
    case 0x83d: assert(bank.owned); return bank.token;
    case 0x83e: case 0x83f: assert(bank.owned); return 1;
    case 0x840: assert(bank.owned); return bank.control;
    case 0x843: return 0x100 | bank.ack;
    case 0x844: return bank.echo;
    case 0x845: case 0x846: return 1;
    case 0x847: return 0x100 | bank.tx;
    case 0x848: return endpoint;
    default: assert(false); return 0;
    }
}
static inline void linx_msgb_io_write(unsigned reg, uint64_t value) {
    switch (reg) {
    case 0x83b:
        assert(value == 1);
        if (bank.owned) bank.rx = 4;
        else if (!bank.queued) bank.rx = 3;
        else {
            bank.owned = true;
            ++bank.token;
            bank.rx = 1;
        }
        break;
    case 0x842:
        assert(bank.owned && value == bank.token);
        ++bank.ack_attempts;
        bank.echo = value;
        bank.ack = bank.ack_error;
        if (bank.ack == 0) {
            bank.queued = false;
            bank.consumed = true;
            bank.ack = 1;
        }
        break;
    case 0x841:
        assert(bank.owned && value == bank.token);
        ++bank.releases;
        bank.rx = bank.release_error;
        if (bank.rx == 0) {
            bank.owned = false;
            bank.consumed = false;
            bank.rx = 1;
        }
        break;
    default: assert(false);
    }
}

#define LINX_MSGB_NATIVE_H
#include <common/pto_message.hpp>

template<size_t N> struct Packet { unsigned char bytes[N]; };

template<size_t N> void roundtrip() {
    bank = Mailbox{};
    bank.staging.fill(UINT64_MAX);
    Packet<N> in{}, out{};
    for (size_t i = 0; i < N; ++i)
        in.bytes[i] = static_cast<unsigned char>(0xaa ^ i);
    assert(pto::PTOSendMessage(endpoint, in) == LINX_MSGB_OK);
    assert((bank.control >> 32) == (N + 7) / 8);
    for (size_t i = 0; i < ((N + 7) / 8) * 8; ++i) {
        unsigned char wire = static_cast<unsigned char>(bank.payload.at(i / 8) >> (8 * (i % 8)));
        assert(wire == (i < N ? in.bytes[i] : 0));
    }
    assert(pto::PTORecvMessage(out) == LINX_MSGB_OK);
    for (size_t i = 0; i < N; ++i) assert(out.bytes[i] == in.bytes[i]);
    assert(bank.ack_attempts == 1 && bank.releases == 1 && !bank.queued && !bank.owned);
}

void mismatch_and_retry() {
    bank = Mailbox{};
    Packet<9> input{{7}}, correct{};
    Packet<1> wrong{{42}};
    assert(pto::PTOSendMessage(endpoint, input) == LINX_MSGB_OK);
    assert(pto::PTOSendMessage(endpoint, wrong) == LINX_MSGB_BUSY);
    assert(pto::PTORecvMessage(wrong) == LINX_MSGB_BAD_COMMAND);
    assert(wrong.bytes[0] == 42 && bank.ack_attempts == 0 && bank.releases == 1);
    assert(bank.queued && !bank.owned);
    assert(pto::PTORecvMessage(correct) == LINX_MSGB_OK && correct.bytes[0] == 7);
    assert(pto::PTORecvMessage(wrong) == LINX_MSGB_EMPTY && wrong.bytes[0] == 42);
}

void recover_faults() {
    for (bool fail_ack : {false, true}) {
        bank = Mailbox{};
        Packet<1> input{{7}}, output{{42}};
        assert(pto::PTOSendMessage(endpoint, input) == LINX_MSGB_OK);
        if (fail_ack) bank.ack_error = 10; // NOT_READ
        else bank.release_error = 7;     // TOKEN_MISMATCH
        assert(pto::PTORecvMessage(output) ==
               (fail_ack ? LINX_MSGB_NOT_READ : LINX_MSGB_TOKEN_MISMATCH));
        assert(output.bytes[0] == 42 && bank.owned);
        assert(bank.queued == fail_ack && bank.releases == (fail_ack ? 0u : 1u));

        // Another typed receive must not steal the outstanding snapshot.
        assert(pto::PTORecvMessage(output) == LINX_MSGB_BUSY && output.bytes[0] == 42);
        linx_msgb_message saved{};
        assert(linx_msgb_read_claim(&saved) == LINX_MSGB_OK);
        assert(static_cast<bool>(saved.acked) == !fail_ack && saved.words[0] == 7);
        bank.ack_error = bank.release_error = 0;
        if (fail_ack) assert(linx_msgb_ack(saved.claim) == LINX_MSGB_OK);
        assert(linx_msgb_release(saved.claim) == LINX_MSGB_OK);
        assert(!bank.owned && !bank.queued);
    }
}

#if defined(BAD_NONTRIVIAL)
struct Rejected { ~Rejected() {} int value; };
#elif defined(BAD_OVERSIZE)
using Rejected = Packet<81>;
#elif defined(BAD_SCALAR)
using Rejected = uint32_t;
#elif defined(BAD_CONST_RECV)
using Rejected = const Packet<1>;
#elif defined(BAD_VOLATILE_RECV)
using Rejected = volatile Packet<1>;
#endif

int main() {
#if defined(BAD_CONST_RECV) || defined(BAD_VOLATILE_RECV)
    Rejected out{};
    (void)pto::PTORecvMessage(out);
#elif defined(BAD_NONTRIVIAL) || defined(BAD_OVERSIZE) || defined(BAD_SCALAR)
    Rejected rejected{};
# if defined(TEST_RECV)
    (void)pto::PTORecvMessage(rejected);
# else
    (void)pto::PTOSendMessage(endpoint, rejected);
# endif
#else
    roundtrip<1>(); roundtrip<9>(); roundtrip<80>();
    mismatch_and_retry();
    recover_faults();
#endif
}
