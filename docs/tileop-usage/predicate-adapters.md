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

The spec #406 composition/view APIs additionally require matching LLVM
PredicateCell result-identity propagation. The earlier adapter compiler alone
does not establish that contract, even if a simple straight-line case compiles.

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

The adapters admit static valid shapes, Local VEC CUBE_M16/M32,
FP32/S32/U32 comparison and value operands, and S64/U64 **byte displacements**.
Valid region and layout must match the predicate, but its comparison basis
does not constrain the FP32/S32/U32 payload type. Indexed memory remains
CUBE_M32-only. M16 supports comparison, logical composition, selection,
typed subviews and concatenation, not indexed memory.
Offsets may have a different physical capacity, but must cover that valid
region. `destination` must have the same element type as `values`.

`scatter` binds the PredicateCell directly to the explicit MSCATTER_MASK
operand. It does not convert it with TCVT or reinterpret it as an ordinary
numeric U8 Tile. This follows the executable
`IndexedTLSUPredicateDescriptorLegal` helper and the contracts in spec #406.

`atomic_add` requires the compiler builtin `ew_mscatter_add_predicate` and
emits MSCATTER_ADD with a PredicateCell ExecutionMask. `true` inverts the
execution mask; this is not a new operation-owned atomic mask operand.
According to `GM_RED_VALUE`, inactive coordinates do not calculate indexed
addresses, probe memory or enqueue atomic RMWs. Structural legality checks
still apply. Replacing inactive addends with zero is not equivalent.

`Predicate<Basis>` cannot be default-constructed by callers or imported from
an arbitrary U8 Tile. Its carrier is intentionally private. GPR selection,
first-set-bit operations, dynamic valid shapes and cross-call predicate
ABI/spill support are not provided by this experimental interface. Keep
producer and consumers together in an optimized, inlined region.

## Logical composition and selection

```cpp
auto lower = predicate::compare_scalar<CmpMode::GE>(values, low);
auto upper = predicate::compare_scalar<CmpMode::LT>(values, high);
auto active = predicate::logical_and(lower, upper);
predicate::select(result, active, true_values, false_values);
predicate::select_scalar(result, active, true_values, 0u);
predicate::atomic_add(destination, byte_offsets, values, active);
```

`logical_and`, `logical_or` and `logical_xor` use U8 TAND/TOR/TXOR.
Both inputs must have identical valid and physical shapes and layouts, but
their original comparison bases may differ. The result is a private
PredicateCell with **U8 basis**, not a Numeric U8 Tile. Numeric inputs are not
implicitly admitted. `logical_numeric<6/7/8>(predicate, numeric)` explicitly
combines a predicate with same-shape S8/U8 Numeric data and returns a **Numeric
U8 Tile**, which cannot be passed as a typed predicate consumer argument.

`select` selects between Numeric true/false Tiles; `select_scalar` selects
between a Numeric true Tile and a raw scalar false value. The predicate basis
may differ from the payload type. Valid shapes/layouts must still match.

## Typed views and closed assembly

`subview<OffsetCells, Columns>(predicate)` materializes a typed TMOV with
B.SUBVIEW. Offsets are in **128-byte CELL units**, not elements or bytes.
Columns must cover complete U8 CELLs (4 columns for M32, 8 for M16), fit the
source valid region, and use at most 64KB view capacity. Basis is preserved.

`concatenate(left, right)` emits INIT/LAST B.ASSEMBLE writers and returns only
the closed parent. Both inputs must be U8-basis logical results with identical
full physical rows, complete equal-size valid column regions and layout;
writer capacity is at most 64KB. The output joins their columns. For example,
an M32 32x8 logical result can be split into `subview<0, 4>` and
`subview<1, 4>`, then concatenated. Arbitrary OPEN-parent mutation is not exposed.

These calls do not request an ExecutionMask on the logical writer or expose
MERGE. ZERO/MERGE, canonical-byte definedness and Q-generation pinning across
wait/trap recovery remain model/runtime requirements; object tests do not
prove those semantics or a complete TopK kernel.

Object/assembly compilation checks are not model execution or TopK numerical
validation. No ISA or model legality checks are relaxed by this interface.
