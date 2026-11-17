// Each disjoint pair exchanges typed objects through its two MSGB endpoints.
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

struct Route {
    uint8_t pair;
    uint8_t peer;
    bool responder;
};
#if MSGB_WORKERS == 8
// In this single-core gfrun profile, tids 0/4 and 3/7 are siblings.
constexpr Route routes[] = {
    {0, 4, false}, {1, 2, false}, {1, 1, true}, {2, 7, false},
    {0, 0, true}, {3, 6, false}, {3, 5, true}, {2, 3, true}
};
#elif MSGB_WORKERS == 2
constexpr Route routes[] = {{0, 1, false}, {0, 0, true}};
#else
#error "supported fixture sizes: 2 or 8 workers"
#endif

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
constexpr Results initial = {
    {0xb5, 0x1d, 0xe2, 0x78}, {}, {}, {0xde, 0xad, 0xbe, 0xef},
    {}, {}, {0x55, 0xaa, 0xc3, 0x3c}, {}, {}, {}
};
extern "C" {
volatile Results msgb_results[MSGB_WORKERS / 2] = {
    initial
#if MSGB_WORKERS == 8
    , initial, initial, initial
#endif
};
}

#ifndef MSGB_RESIDENT_FIXTURE
static void enable_bank() {
    const uint64_t domain = 1;
    const uint64_t configuration = (UINT64_C(1) << 32) | (1u << 16) | 1u;
    // Reset selects ACR0. Bind this hardware thread's bank to the test domain.
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
        status = pto::PTOSendMessage(endpoint, object);
    } while (status == LINX_MSGB_BUSY);
    return status;
}

static linx_msgb_result requester(const Route &route) {
    Request request{static_cast<uint8_t>(0xd3 ^ (0x23 * route.pair)),
                    {0x00, 0xff, 0x71, 0x80, 0x19, 0xaa, 0xc4, 0x35}};
    request.key[7] += route.pair;
    auto &output = msgb_results[route.pair];
    capture(output.sent_request, request);
    auto status = send(16 * route.peer, request);
    if (status != LINX_MSGB_OK)
        return status;
    Reply reply;
    status = receive(reply);
    if (status == LINX_MSGB_OK)
        capture(output.received_reply, reply);
    return status;
}

static linx_msgb_result responder(const Route &route) {
    Request request;
    auto status = receive(request);
    if (status != LINX_MSGB_OK)
        return status;
    auto &output = msgb_results[route.pair];
    capture(output.received_request, request);
    Reply reply;
    reply.tag = 0xbadc0000u | (static_cast<uint32_t>(request.tag) << 8) | request.key[7];
    reply.generation = 0x10203040u;
    for (unsigned i = 0; i < sizeof(reply.body); ++i)
        reply.body[i] = static_cast<uint8_t>((37 * i + request.key[i & 7]) ^ request.tag);
    capture(output.sent_reply, reply);
    return send(16 * route.peer, reply);
}

static unsigned worker_id() {
    const uint64_t endpoint = linx_msgb_io_read(LINX_MSGB_ENDPOINT);
    if (endpoint % 16 != 0 || endpoint / 16 >= MSGB_WORKERS)
        __builtin_trap();
    const unsigned tid = static_cast<unsigned>(endpoint / 16);
    // This gfrun platform exposes the physical PE in THREAD_ID. Siblings
    // have distinct mailbox IDs even though this register agrees.
    uint64_t pe;
    asm volatile("C.BSTART.SYS FALL\n SSRGET 0x26, ->%0\n C.BSTART.STD FALL"
                 : "=r"(pe) :: "memory");
    if (pe != tid % 4)
        __builtin_trap();
    return tid;
}

static uint8_t exchange(unsigned tid) {
    const Route &route = routes[tid];
    auto &output = msgb_results[route.pair];
    const unsigned side = route.responder ? 1 : 0;
    output.endpoint[side] = static_cast<uint8_t>(linx_msgb_io_read(LINX_MSGB_ENDPOINT));
    output.ready[side] = static_cast<uint8_t>(0x31 + tid);
#ifndef MSGB_RESIDENT_FIXTURE
    while (!output.ready[1 - side]) {}
#endif
    const auto status = route.responder ? responder(route) : requester(route);
    const auto value = static_cast<uint8_t>(status == LINX_MSGB_OK ? 0xa0 + tid : status);
    output.done[side] = value;
    return value;
}

#ifdef MSGB_RESIDENT_FIXTURE
extern "C" uint64_t msgb_requester(uint64_t argument) {
    const unsigned tid = worker_id();
    if (routes[tid].responder)
        __builtin_trap();
    return argument ^ exchange(tid);
}

extern "C" uint64_t msgb_responder(uint64_t argument) {
    const unsigned tid = worker_id();
    if (!routes[tid].responder)
        __builtin_trap();
    return argument ^ exchange(tid);
}
#else
extern "C" [[noreturn]] void _start() {
    enable_bank();
    const unsigned tid = worker_id();
    exchange(tid);
    if (tid == 0) {
        bool passed = true;
        for (unsigned i = 0; i < MSGB_WORKERS; ++i) {
            const auto &done = msgb_results[routes[i].pair].done[routes[i].responder ? 1 : 0];
            while (!done) {}
            passed &= done == 0xa0 + i;
        }
        *reinterpret_cast<volatile uint32_t *>(UINT64_C(0x10009000)) = passed ? 0x5555 : 0x13333;
    }
    for (;;) { asm volatile("" ::: "memory"); }
}
#endif
