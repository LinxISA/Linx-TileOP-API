# Scalar GM shareability

PTO-ISA issue #416 adds explicit Scalar `.private`, `.intra`, and `.inter`
forms. Use a compiler implementing these forms and the `__builtin_linx_scalar_*`
builtins. Unsupported target compilers report an error when this API is used.

```cpp
#include "common/pto_memory.hpp"
using pto::MemoryScope;
auto counts = pto::scalar_view<MemoryScope::IntraCore>(scratch.num);
counts.store(0, 0);
// Issue Tile/IndexedGenerated work and perform the required completion/sync.
auto n = counts.load(0);
pto::scoped_memset<MemoryScope::IntraCore>(output, 0xff, output_bytes);
```

`GlobalTensor::scalar_access<Scope>()` and `global_tensor::scalar_access<Scope>()` provides the same view. Scalar accesses
that interact with Tile/IndexedGenerated requests must select IntraCore and
execute in the same Core. InterCore is for matching Scalar communication across
Cores of one PTO device; it cannot replace IntraCore for Tile communication.

These interfaces explicitly select scope for initialization, reads after Tile
writes, and scalar atomics, including stack scratch exposed to Tile operations. `GlobalTensor::data()` and `ScalarMemoryView::raw_data()`
return ordinary pointers for existing Tile binders; direct scalar dereferences
of them do not inherit the view's scope. Ordinary dereferences instead follow
the compiler's automatic policy below. There is no persistent scope switch.

`scoped_memcpy<DestinationScope, SourceScope>` keeps its two scopes separate.
The byte-wise helpers accept unaligned buffers and do not fall back to ordinary
libc memory functions. `scalar_load/store` require natural alignment for their
access width. The scope APIs do not add synchronization or change atomic order.

`scalar_atomic<Scope, AtomicOperation, MemoryOrder>` returns the previous value
for RMW/CAS, the loaded value for LR, or the architectural SC status (zero on
success). CAS arguments are expected value followed by desired value. LR/SC
must use matching scope and satisfy the existing reservation rules. Atomic ADD
and logical/min/max operations require 32/64-bit integers; exchange/CAS/LR/SC
also support 8/16-bit integers. Signed min/max interpret the memory width as a
signed integer; unsigned min/max use unsigned interpretation.

Host simulation supports ordinary load/store and byte helpers as ordinary CPU
accesses. It does not validate PTO domains or emulate reservation monitors.
End-to-end model tests must additionally inspect request scope and results.

The native Scalar compiler automatically selects IntraCore for ordinary
accesses whose objects cannot be proved private. This includes parameters even
with `restrict/noalias`, globals, integer addresses, and stack objects passed to
Tile, calls, or inline assembly. Only nonescaping stack objects without Tile
memory uses retain Private accesses. This is a conservative object analysis;
it can increase instruction size and reduce optimization. Ordinary initialization,
memset/memcpy/memmove, scalar reads and supported atomics need no API annotation.
The transformation starts before generic optimization, with a second sweep
before code generation. Tile carriers and marked element loops keep their
fixed-IntraCore Tile/IndexedGenerated lowering.

`PTO_SCALAR_SHAREABILITY("intra")` before a function declaration sets an explicit
whole-function default (also `private`/`inter`). Explicit builtins retain their
selected scope. The function stays out of line; memory callees need a matching
contract. Explicit defaults reject inline assembly. A Private default is the
programmer's assertion that such accesses need no Tile sharing.

Automatic mode accepts native callees with visible definitions. External memory
callees require an `intra` contract on their declaration and a separately compiled
implementation that honors it. Indirect memory calls and unsupported generic
memory intrinsics are diagnosed. Aggregate/scalable accesses, non-GM address
spaces, and unaligned or wider-than-64-bit shared atomics also require a separate
implementation and are diagnosed. No unknown-scope runtime fallback is emitted.
Scope selection supplies neither completion nor synchronization.

To run the target compile and object checks against a rebuilt compiler:

```sh
LINX_CLANG=/path/to/build/bin/clang++ LINX_SYSROOT=/path/to/sysroot \
  python3 test/test_scalar_shareability.py
```

These checks cover O0/O2/O3 and mixed Scalar/TLOAD/TSTORE compilation. They do
not execute a PTO model or establish runtime synchronization correctness.
