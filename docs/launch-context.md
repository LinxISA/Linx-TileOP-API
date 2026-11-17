# Device block launch context

Include `<common/pto_launch.hpp>` in a Linx64 V5 device kernel dispatched by a
runtime that binds the PTO launch context. The header exposes the one-dimensional
logical block count and index as 32-bit values.

```cpp
#include <common/pto_launch.hpp>

void kernel(uint64_t tile_count)
{
    const uint32_t block_num = pto::GetBlockNum();
    for (uint64_t tile = pto::GetBlockIdx(); tile < tile_count;
         tile += block_num) {
        process_tile(tile);
    }
}
```

`pto::GetBlockNum()` reads the read-only `BLOCKNUM` system register. It returns
the number of logical block instances in the current launch. A valid launch has
a count in the range 1 through `UINT32_MAX`. The XLEN register carrier has zero
in its upper 32 bits.

`pto::GetBlockIdx()` reads the read-only `BLOCKID` system register. It returns
the zero-based logical index assigned to this kernel call. Its value is less
than `GetBlockNum()`.

The index identifies a logical launch instance. It does not identify a physical
PE or hardware thread. A resident worker may execute different indices in
successive launches. Use `get_thread_idx()` only when physical PE identity is
part of the kernel's platform contract.

The runtime binds both registers when it admits the launch and retains them
through the matching completion. Each resident worker has an independent
binding. After completion, the worker returns to its ambient base-register
values. Reset initializes both registers to zero. A legacy launch binds a
single logical block with `BLOCKNUM=1` and `BLOCKID=0`. Code outside a
dispatched kernel must not use their values as a launch identity. The accessors
emit
`SSRGET 0x0050` and `SSRGET 0x0051` in sequential SYS blocks. They provide no
host/device memory fence.
