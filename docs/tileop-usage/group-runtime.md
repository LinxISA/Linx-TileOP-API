# Hosted four-PE group runtime

The first hosted SMT4 ABI keeps the process runtime on PE0 and uses a small
worker entry for PE1 through PE3. It is intentionally static-only,
single-shot, and independent of glibc or musl internals.

## Required symbols

Applications define one worker body:

```cpp
extern "C" int __linx_group_worker_main(uint32_t peId, void *context);
```

PE0 calls:

```cpp
int status = linx_group_run(context);
```

The linked compiler runtime provides `linx_group_run()` and the non-returning
`__linx_group_worker_start` entry consumed by gfrun.

## Ownership

- PE0 owns libc startup, TLS, file descriptors, input/output, and process
  exit.
- PE1 through PE3 use independent stacks and do not call libc or issue
  syscalls.
- `THREAD_ID` selects PE0 through PE3; `BLOCKID` remains the core work ID.
- The worker body publishes one completion status per PE and then parks until
  PE0 terminates the process.

## ELF boundary

The first ABI supports static executable and static PIE carriers. A
`PT_INTERP` image is rejected. A static PIE may contain `PT_DYNAMIC` for
self-relocation without becoming a dynamically interpreted carrier.

The executable must retain `__linx_group_worker_start` in its symbol table so
the functional model can assign PE1 through PE3 their initial PC.
