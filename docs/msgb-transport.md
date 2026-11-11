# Scalar message-buffer transport

Include `<common/linx_msgb.h>` in a Linx64 V5 C or C++ kernel whose runtime
has enabled the MSGB profile for the current Core, PE, ACR and invocation.
This opt-in header follows draft [PTO-SPEC #426 at d537afa](https://github.com/PTO-ISA/pto-spec/pull/426/commits/d537afa23ea0783b55f994ac52e2154f44b60b0c).
ACR0 owns bank selection, domain assignment and enablement. The runtime must
serialize callers, including callbacks and interrupt handlers using the same
bank, across staging and status reads and for the lifetime of a receive claim.
There is one set of staging, snapshot and diagnostic registers per bank.
For a zero-word send, the adapter writes CLAIM=0 before BSE. This checks the
enabled-bank precondition without acquiring or changing a receive snapshot.
On #426 a disabled bank faults at that write, before the legacy BSE can run.

## Send and receive a C++ struct

Include `<common/pto_message.hpp>`. Both endpoints agree on one trivially
copyable struct type. The payload can occupy up to 80 bytes, including
alignment padding. The following request occupies two mailbox words.

```cpp
#include <common/pto_message.hpp>

struct WorkRequest {
    uint64_t job;
    uint32_t first;
    uint32_t count;
};

linx_msgb_result submit(uint32_t worker)
{
    WorkRequest request{42, 128, 32};
    return pto::PTOSendMessage(worker, request);
}

linx_msgb_result poll(WorkRequest &request)
{
    return pto::PTORecvMessage(request);
}
```

Send makes one admission attempt. Receive makes one claim attempt, reads the
snapshot, checks its word count, acknowledges the exact claim, and releases
it before updating the caller's object. EMPTY or BUSY leaves the object as it
was. If the word count differs, receive releases the claim without ACK and
returns BAD_COMMAND on successful release. The matching reader can then claim
the same queue head. If ACK or RELEASE fails, the operation returns that
status. Use the register-level recovery API below for a retained snapshot.

The wire format is the object's native byte representation rounded up to a
whole number of 64-bit words. Send zero-fills the unused **trailing word
bytes**. Internal struct padding is part of the copied representation. Use
fixed-width fields and explicit initialized reserved bytes for shared wire
layouts. Both endpoints must agree on layout, byte order and valid field
values. MSGB carries only the word count, so different types that occupy the
same number of words cannot be distinguished. Use a shared tag field when
multiplexing message kinds. Addresses, references and process-local handles
require an application-defined meaning at the receiver. The receive argument
must be a writable complete object of the agreed type, with writable payload
fields.

## Word-level operations and recovery

| Operation | Observable result |
| --- | --- |
| `linx_msgb_try_send(dst, words, count, &id)` | Stages 0–10 words and issues BSE once. On OK or DUPLICATE, returns the admitted 128-bit transport ID. Other results leave `id` untouched. A null `words` pointer is permitted for count zero. |
| `linx_msgb_try_claim(&message)` | Writes CLAIM=1. On OK copies the identity, destination, bank-scoped claim handle and every valid word of the stable snapshot. EMPTY or BUSY leaves the output untouched. Only `words[0..count)` contain received data. |
| `linx_msgb_read_claim(&message)` | Recovers the currently owned snapshot in the same bank and invocation, including an ACKED snapshot. Requires exclusive ownership of that bank. |
| `linx_msgb_ack(message.claim)` | Checks the endpoint, writes the full claim token and reads the ACK result and token echo. A successful ACK consumes the queued message and retains its snapshot. |
| `linx_msgb_release(message.claim)` | Releases the exact snapshot. Before ACK, the message stays queued and the next claim receives a fresh token. After ACK, this frees the retained snapshot. |

For example, a receiver can copy a message for local processing and recover
a snapshot retained across an interruption. The runtime must determine whether
application work already ran before replaying handlers.

```c
#include <common/linx_msgb.h>

linx_msgb_result receive_one(linx_msgb_message *message)
{
    linx_msgb_result r = linx_msgb_try_claim(message);
    if (r != LINX_MSGB_OK)
        return r;
    r = linx_msgb_ack(message->claim);
    if (r != LINX_MSGB_OK && r != LINX_MSGB_DUPLICATE)
        return r;
    return linx_msgb_release(message->claim);
}
```

The sender supplies a 32-bit endpoint. BSE takes the endpoint in bits 31–0
and the word count in bits 35–32. Each word contains 64 opaque bits. The
transport assigns `id.hi` (source endpoint and epoch) and `id.lo` (transmit
sequence). Receiver claim tokens have a separate epoch and sequence and must
stay with their original bank and invocation.

Each call makes one attempt. An older fault-capable GM operation may stall
the underlying instruction. BUSY can mean backpressure or exhausted replay
receipt capacity. EXHAUSTED is surfaced directly. Reissuing a C send after
success creates a new dynamic BSE occurrence. Transport admission,
receiver consumption and service completion are distinct events. Publishing
or acquiring unrelated Tile or GM data requires its own ordering protocol.
The service-level scalar interfaces are proposed separately in
[Linx-TileOP-API #310](https://github.com/LinxISA/Linx-TileOP-API/pull/310).
