# Typed PredicateCell adapters (experimental)

## Availability

This is an explicit TileOP API, **not an element-if-only interface**. It does
not require a pragma loop or `TPARTELEMENT`. For scalar-style conditionals in
element loops, use the [Element-if guide](element-if.md); that path constructs
predicates internally. There is currently no public bridge between its implicit
masks and `Predicate<Basis>`, and these adapter calls are not supported loop-body
operations. Sharing ISA predicate concepts does not make the two APIs or their
supported profiles interchangeable.

This is an **unreleased development-branch interface**, not a claim about an
installed SDK. It requires the matching LLVM compiler changes that introduce
`ew_mscatter_add_predicate`; existing MC ExecutionMask support alone is not
sufficient. Keep the new TileOP headers and matching compiler together. Do not
replace headers in an older installed SDK before that compiler dependency is
available.

Include `common/pto_tileop.hpp` when targeting Linx. The experimental
`pto::predicate` API keeps a comparison result distinct from ordinary numeric
Tiles and packed GPR predicates:

```cpp
auto active = pto::predicate::compare_scalar<pto::CmpMode::GE>(values, threshold);
pto::predicate::scatter(destination, byte_offsets, values, active);
pto::predicate::atomic_add(destination, byte_offsets, values, active);
pto::predicate::atomic_add<true>(destination, byte_offsets, values, active);
```

## Complete compile-only example

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Values = VecTileM32<uint32_t, 32, 4>;
using Offsets = VecTileM32<uint64_t, 32, 4>;
using Global = global_tensor<uint32_t, RowMajor<32, 4>>;

void predicate_consumers(Values &values, Offsets &byte_offsets,
                        Global &destination, uint32_t threshold) {
  auto active = predicate::compare_scalar<CmpMode::GE>(values, threshold);
  predicate::scatter(destination, byte_offsets, values, active);
  predicate::atomic_add(destination, byte_offsets, values, active);
  predicate::atomic_add<true>(destination, byte_offsets, values, active);
}
```

The caller must supply initialized Numeric CUBE operands. Offsets are already
byte displacements, not element indices; this API does not scale them by the
value element size. This example demonstrates compilation only, not a complete
kernel initialization sequence or a model execution test.

With the matching compiler, use the usual Linx target options and your SDK
sysroot/resource configuration:

```bash
clang++ --target=linx64v5-unknown-linux-musl -mlxbc -fenable-matrix \
  -O2 -std=c++20 -D__linx -DENABLE_TENSOR_INSTR \
  --sysroot="$LINX_SYSROOT" -I"$TILEOP_ROOT/include" \
  -c predicate_consumers.cpp -o predicate_consumers.o
```

Ensure the compiler resource directory does not contain a stale TileOP include
tree that takes precedence over the selected headers.

## Carrier and consumer contract

The first implementation admits static valid shapes, Local VEC CUBE_M32,
FP32/S32/U32 comparison and value operands, and S64/U64 **byte displacements**.
The value type, valid region and layout must match the comparison basis.
Offsets may have a different physical capacity, but must cover that valid
region. `destination` must have the same element type as `values`.

`scatter` binds the PredicateCell directly to the explicit MSCATTER_MASK
operand. It does not convert it with TCVT or reinterpret it as an ordinary
numeric U8 Tile. This follows the executable
`IndexedTLSUPredicateDescriptorLegal` helper; the instruction-page wording
discrepancy is tracked in PTO-ISA/pto-spec #349.

`atomic_add` requires the compiler builtin `ew_mscatter_add_predicate` and
emits MSCATTER_ADD with a PredicateCell ExecutionMask. `true` inverts the
execution mask; this is not a new operation-owned atomic mask operand.
According to `GM_RED_VALUE`, inactive coordinates do not calculate indexed
addresses, probe memory or enqueue atomic RMWs. Structural legality checks
still apply. Replacing inactive addends with zero is not equivalent.

`Predicate<Basis>` cannot be default-constructed by callers or imported from
an arbitrary U8 Tile. Its carrier is intentionally private. Ordinary TAND is
not a PredicateCell logical operator. Predicate composition, GPR selection,
first-set-bit operations, dynamic valid shapes, M16 and cross-call predicate
ABI/spill support are not provided by this experimental interface. Keep
producer and consumers together in an optimized, inlined region.

Object/assembly compilation checks are not model execution or TopK numerical
validation. No ISA or model legality checks are relaxed by this interface.
