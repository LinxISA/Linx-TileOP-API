// The same two kernels are used for direct boot and resident HSCB dispatch.
// Both typed objects cross PEs exclusively through the mailbox.
#include <common/pto_message.hpp>

struct Request {
    uint8_t tag;
    uint8_t key[8];
};
struct Reply {
    uint32_t tag;
    uint32_t generation;
    uint8_t body[72];
};
static_assert(sizeof(Request) == 9 && sizeof(Reply) == 80, "wire sizes");

struct Results {
    uint8_t head[4];
    uint8_t sent_request[9];
    uint8_t received_request[9];
    uint8_t middle[4];
    uint8_t sent_reply[80];
    uint8_t received_reply[80];
    uint8_t end[4];
    uint8_t ready[2];
    uint8_t endpoint[2];
    uint8_t done[2];
};
static_assert(sizeof(Results) == 196, "dump layout");
extern "C" {
volatile Results msgb_results = {
    {0xb5, 0x1d, 0xe2, 0x78}, {}, {}, {0xde, 0xad, 0xbe, 0xef},
    {}, {}, {0x55, 0xaa, 0xc3, 0x3c}, {}, {}, {}
};
}

#ifndef MSGB_RESIDENT_FIXTURE
static void enable_bank() {
    const uint64_t domain = 1;
    const uint64_t configuration = (UINT64_C(1) << 32) | (1u << 16) | 1u;
    // Reset selects ACR0. Bind the invocation's ACR0 bank to domain and epochs.
    asm volatile("C.BSTART.SYS FALL\n SSRSET %0, 0x84b\n C.BSTART.STD FALL"
                 :: "r"(domain) : "memory");
    asm volatile("C.BSTART.SYS FALL\n SSRSET %0, 0x84a\n C.BSTART.STD FALL"
                 :: "r"(configuration) : "memory");
}

#endif

template <class T> static void capture(volatile uint8_t *slot, const T &value) {
    auto bytes = reinterpret_cast<const uint8_t *>(&value);
    for (unsigned i = 0; i < sizeof(T); ++i)
        slot[i] = bytes[i];
}

template <class T> static linx_msgb_result receive(T &object) {
    linx_msgb_result status;
    do {
        status = pto::PTORecvMessage(object);
    } while (status == LINX_MSGB_EMPTY);
    return status;
}

template <class T> static linx_msgb_result send(uint32_t endpoint, const T &object) {
    linx_msgb_result status;
    do {
        // During resident dispatch the peer's LAUNCH can still occupy its bank.
        status = pto::PTOSendMessage(endpoint, object);
    } while (status == LINX_MSGB_BUSY);
    return status;
}

static linx_msgb_result requester() {
    const Request request{0xd3, {0x00, 0xff, 0x71, 0x80, 0x19, 0xaa, 0xc4, 0x35}};
    capture(msgb_results.sent_request, request);
    auto status = send(16, request);
    if (status != LINX_MSGB_OK)
        return status;
    Reply reply;
    status = receive(reply);
    if (status == LINX_MSGB_OK)
        capture(msgb_results.received_reply, reply);
    return status;
}

static linx_msgb_result responder() {
    Request request;
    auto status = receive(request);
    if (status != LINX_MSGB_OK)
        return status;
    capture(msgb_results.received_request, request);
    Reply reply;
    reply.tag = 0xbadc0000u | (static_cast<uint32_t>(request.tag) << 8) | request.key[7];
    reply.generation = 0x10203040u;
    for (unsigned i = 0; i < sizeof(reply.body); ++i)
        reply.body[i] = static_cast<uint8_t>((37 * i + request.key[i & 7]) ^ request.tag);
    capture(msgb_results.sent_reply, reply);
    return send(0, reply);
}

static void record_entry(unsigned pe) {
    msgb_results.endpoint[pe] = static_cast<uint8_t>(linx_msgb_io_read(LINX_MSGB_ENDPOINT));
    msgb_results.ready[pe] = static_cast<uint8_t>(0x31 + pe);
}

static uint8_t record_done(unsigned pe, linx_msgb_result status) {
    const auto value = static_cast<uint8_t>(status == LINX_MSGB_OK ? 0xa0 + pe : status);
    msgb_results.done[pe] = value;
    return value;
}

#ifdef MSGB_RESIDENT_FIXTURE
extern "C" uint64_t msgb_requester(uint64_t argument) {
    record_entry(0);
    return argument ^ record_done(0, requester());
}

extern "C" uint64_t msgb_responder(uint64_t argument) {
    record_entry(1);
    return argument ^ record_done(1, responder());
}
#else
extern "C" [[noreturn]] void _start() {
    uint64_t pe;
    asm volatile("C.BSTART.SYS FALL\n SSRGET 0x26, ->%0\n C.BSTART.STD FALL"
                 : "=r"(pe) :: "memory");
    if (pe < 2) {
        enable_bank();
        record_entry(pe);
        while (!msgb_results.ready[1 - pe]) {}
        auto status = pe == 0 ? requester() : responder();
        record_done(pe, status);
        if (pe == 0) {
            while (!msgb_results.done[1]) {}
            *reinterpret_cast<volatile uint32_t *>(UINT64_C(0x10009000)) =
                msgb_results.done[0] == 0xa0 && msgb_results.done[1] == 0xa1 ? 0x5555 : 0x13333;
        }
    }
    for (;;) { asm volatile("" ::: "memory"); }
}
#endif
