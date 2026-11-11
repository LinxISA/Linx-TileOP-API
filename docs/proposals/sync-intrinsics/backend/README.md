# PTO MSGB backend and bounded runtime

This opt-in development implementation pins PTO PR #426 at
`d537afa23ea0783b55f994ac52e2154f44b60b0c` in
`contracts/pto-msgb-sources.json`. It does not change the active/release Tile
catalog or install any proposal header. ASL remains the normative owner.

## Compiler and endpoint

The native scalar compiler exposes:

```c
unsigned long __builtin_linx_msgb_read(unsigned long address);
void __builtin_linx_msgb_write(unsigned long address, unsigned long value);
void __builtin_linx_msgb_send(unsigned long control);
```

Addresses are compile-time constants in 0x0830..0x084b. The backend uses only
legal read/write directions; the ISA rejects forbidden accesses. All three
intrinsics are convergent, nonduplicable, chained conservative memory effects.
Each instruction occupies a legal SYS Block. BSE receives control directly;
MSGBCR is read-only last-accepted control. This is not a payload data fence.

`msgb_endpoint.h` uses injected SSR/BSE operations for host execution and
fault injection; `msgb_linx_backend.h` supplies the target implementation when
the three builtins are present. Context-local serialization is required across
staging, BSE and result reads, and across RX claim through ACK/release. The
launcher must prevent interrupt/reentrant users from sharing an active bank.
No TX claim/release hardware register is invented.

A manager configures the target ACR on each PE after kernel launch, using
MSGBCFGSEL, immutable MSGBDOMAIN and MSGBCFG. Epochs come from the manager;
they must not be reset to one on each invocation. Configure only a quiescent
bank. Same-invocation disable/re-enable cannot clear replay receipts.

`PTO_MSGB_IO_RECOVERY` means an instruction's effect is unknown. It is not a
rejected send or ACK. The platform must finish recovery of that exact
instruction occurrence before calling `pto_msgb_tx_resume` or
`pto_msgb_ack_resume`. These functions observe retained results; they do not
reissue an already issued BSE. An interrupted staging write also retains the
private TX words and excludes new sends; resume continues staging only after
the platform completes the original write. Ordinary new sends need a new dynamic SYS occurrence. RX recovery
keeps the exact token and snapshot, including ACKED payload. The runtime blocks
progress after an uncertain claim/read/release until platform recovery followed
by `pto_sync_resume_rx`; TX and ACK use their corresponding resume functions. Do not initialize
the endpoint again during trap recovery.

The original callback reference in `reference/msgb_transport.h` remains a
simple definite-outcome algorithm. Do not connect a backend with uncertain
commit results to it; use the recovery-aware endpoint adapter instead.

## Runtime integration

Compile `runtime/sync_runtime.c` and implement
`pto_sync_current_runtime()` in the launcher using a PE/context-local binding.
Initialize one serialized executor per bank with a unique token namespace and
nonzero invocation. Initialize only a new executor, never over live claims,
wire requests, aliases or recovery state.

The manager copies immutable object/group bindings to all participants with
`pto_sync_bind_data/group`. Their wire handles select the exact SyncKey,
region, owner and fixed contribution/consumer/member sets. INIT validates the
local declaration against that binding. Binding after initialization or
reusing a handle for a new generation is not supported. A new invocation needs
fresh endpoint/runtime lifetime management, not overwriting old records.

The private v1 codec always uses ten words. Word zero contains version,
opcode, status and inline length; subsequent words carry invocation, request
identity, object handle, actor and event identity/argument. The final four
words carry up to 32 inline TASK bytes or QUERY results. Multiword fields are
logical numeric words; target transport owns their byte representation. Raw C
structs, member pointers and remote source addresses are never sent. Bulk data
and dynamic remote descriptor fetching are not implemented. Task port zero is
the supported mailbox; deployment resolve/source callbacks map logical
endpoints to trusted transport endpoint IDs in both directions.

The runtime implements request/event tokens, all declared submission APIs,
owner state transitions, RESULT replies, sticky DATA_READY/BUFFER_FREE/
GROUP_READY, task mailbox deduplication, and platform-owned resource returns.
Owner handlers validate trusted source membership. Request receipts retain
exact request/result identities until executor retirement. TASK deduplication
also retains sender/invocation/application-message ID after application POP.
Full owner receipt storage rejects new commands with BUSY without blocking
later RESULT messages. Wire event tombstones retain their slots until executor
retirement so service retries can complete after caller release. TASK IDs retain
the original tag and payload and reject conflicting reuse. Other temporary
capacity exhaustion returns backpressure; identities are never silently evicted.

Platform callbacks are required for data visibility and resource safety:

- `capture/poll/validate/drop`: capture a closed set of matching older data
  engine operations, retain its lifetime and validate region/scope coverage.
- `acquire`: establish the exact READY region's access-path visibility before
  completing a data event. Pending acquisition retains the RX message.
- `quiescent`: prove no current or future authorized accesses remain before
  FREE, retirement or abort completion. For ABORT this includes revocation of
  granted read permissions, not merely a DMA queue becoming empty.
- `resource_validate/resource_return`: validate use completion and idempotently
  return the trusted sender's complete resource lease.

Missing callbacks fail closed. There is no default successful data-engine
completion. A pure GROUP rendezvous has no payload acquire effect.
Request/event waits drive local progress and invoke the platform yield hook;
the launcher must also progress peer executors while a PE waits. BWE/BWI/BWT
are unchanged and no event-token sleep ABI is assumed.

Call `pto_sync_begin_drain` to stop new work while continuing progress for
accepted work. RELEASE, read-completion capture, resource return, ABORT, error
reporting and RETIRE remain available to discharge existing obligations. Drain services and mailbox obligations before the ISA
End/Abort closes both-way admission; after the ISA drain starts, only existing
RX reads, ACK and RELEASE remain possible. Do not depend on sending a new
service reply after ISA admission closes. `pto_sync_quiescent` is a software
check, not authority to bypass the architecture's own lifecycle checks.

The reference ISA has one queued slot per bank and sixteen replay receipts per
invocation. Receipts are not reclaimed by RX ACK or epoch advance. Long-running
traffic can therefore reach permanent BUSY in that bounded ASL instance;
progress reports backpressure. Wait helpers have no automatic timeout;
the launcher must apply a bounded retry/diagnostic policy for permanent BUSY. A scalable target needs a
separately specified safe receipt-retirement policy. The host runtime tests use
a backend with a larger send history and prove service behavior, not the
sixteen-receipt ASL capacity or hardware progress.

## Validation

`make check-sync-proposal` compiles C11/C++17 declarations and executes the ACK,
definite-outcome transport, recovery-aware endpoint and multi-endpoint service
tests. `tools/check_msgb_contract.py --spec-repo PATH` additionally checks pinned
Git source hashes when that commit is available. Target compiler tests check
O2/O3 preservation, SYS envelopes and object encodings. Set `LINX_CC` to the
patched compiler and `LINX_SYSROOT` to its sysroot when running
`make check-sync-proposal` to also compile backend/runtime objects at O2/O3. None of these replaces
ASL-Model integration, device execution, HSCB reliability or physical ordering
validation.
