# Janus packed BF16 and legacy SIMT APIs

Janus is a non-SIMT PTO/Block target. Packed BF16 conversion and arithmetic now
use an integer software backend on non-SIMT translation units. It emits scalar
integer operations instead of legacy `v.*` instructions with `vr` operands.
Verified SIMT translation units retain their existing instruction path. This
resolves the packed conversion/addition path in TileOP #325 without changing
the `__bf16x2` ABI.

## Supported software profile

`PTO_TILEOP_API_HAS_JANUS_PACKED_BF16` reports availability of this software
profile, not hardware acceleration or equivalence to every legacy SIMT
numerical behavior. The helpers are in `<common/pto_packed_bf16.hpp>`; Linx
`<common/pto_tileop.hpp>` includes them.

| API | Non-SIMT behavior |
| --- | --- |
| `linx_cvt_packed(dst, float, float)` and matching `package` aliases | FP32 to two BF16 lanes |
| `pto::bf16x2_from_float<RMode, Sat>(lo, hi)` | Explicit numerical packing |
| `pto::bf16x2_to_float<Lane>(packed)` | Exact BF16 to FP32 widening |
| `blkv_bf16x2_fadd/fsub/fmul/fdiv/fsqrt` and aliases | Per-lane correctly rounded arithmetic |
| `blkv_bf16x2_fmadd/fmsub`, `madd/fmmad/msub` | Fused `a*b+c` / `a*b-c`, one final rounding |
| `blkv_bf16x2_fmax/max` | Maximum; canonical NaN if either input is NaN; +0 wins over -0 |
| `blkv_bf16x2_fexp/exp` | Compile-time rejection |
| Scalar `blkv_bf16_*`, other `linx_cvt*` type pairs and address-space(6) forms | Require verified legacy SIMT |

Supported modes are `RNONE` (defined here as `RNE`), `RNE`, `RTZ`, `RDN`, `RUP`,
`RNA` (nearest, ties away), and `RTO` (truncate then set the low bit on inexact
results). `RHB` and saturation are rejected at compile time.

Finite operations use exact integer intermediates before one BF16 rounding,
including FMA. Subnormals are preserved without FTZ. Infinities and signed
zeros follow the operation and rounding direction. Numerical conversion and
arithmetic canonicalize NaNs to `0x7fc0`; raw widening and bit helpers preserve
the BF16 payload. This backend does not read or update FSU rounding/status
registers or raise floating-point exception flags. It prioritizes correctness
and portability and is not a throughput substitute for Tile APIs.

```cpp
__bf16x2 pair;
linx_cvt_packed(pair, 1.f, 2.f);
auto doubled = blkv_bf16x2_add(pair, pair);
uint32_t packed = pto::bf16x2_to_bits(pair); // 0x40003f80
uint32_t sum = pto::bf16x2_to_bits(doubled); // 0x40804000
float lane0 = pto::bf16x2_to_float<0>(pair); // 1.f
float lane1 = pto::bf16x2_to_float<1>(pair); // 2.f
```

For tensor data, prefer supported BF16 Tiles and Tile-level `TCVT`/`TADD` under
their dtype, shape, layout and rounding contracts. A two-element scalar pack
is not a Tile register.

## Legacy target capability

Updated Clang defines `__linx_simt_contract=1` for Linx targets and
`__linx_simt=1` only when the selected CPU and final target features support
legacy SIMT. Both markers are required for that instruction path. Older
compilers use the packed software subset and conservatively reject remaining
legacy operations. Do not define compiler-owned macros with `-D`. Function
attributes such as `__vec__` do not change translation-unit preprocessor
capabilities or enable SIMT on Janus.

`PTO_TILEOP_API_HAS_LEGACY_SIMT_CONTRACT` reports that headers enforce this
contract. Unsupported calls fail with an API diagnostic before invalid register
constraints reach the assembler. Existing type restrictions remain. The
markers do not establish that all historical conversion-table entries are
implemented by every SIMT compiler/model.

## Packed storage and raw bits

The compiler-provided `__bf16x2.data` may be a `float` carrier, while host
compatibility storage uses `uint32_t`. Assigning `.data` to an integer can
perform numerical conversion. Use `<common/pto_packed_bits.hpp>`:

```cpp
auto pair = pto::bf16x2_pack_bits(0x3f80, 0x4000);
uint32_t word = pto::bf16x2_to_bits(pair);       // 0x40003f80
uint16_t lane0 = pto::bf16x2_lane_bits<0>(pair); // 0x3f80
uint16_t lane1 = pto::bf16x2_lane_bits<1>(pair); // 0x4000
auto copy = pto::bf16x2_from_bits(word);
```

Lane 0 occupies bits [15:0], lane 1 bits [31:16]. These helpers copy encodings,
including NaN payloads, without numerical conversion or ABI changes.
`PTO_TILEOP_API_HAS_BF16X2_BITS` reports their availability.

## Numerical acceptance

`test/test_packed_bf16_numeric.py` compares the backend against an independent
Python exact-rational oracle. Host coverage includes every BF16 sqrt encoding,
all supported rounding modes, randomized arithmetic, and FP32 conversion
boundaries around every BF16 bin. The target fixture compiles public Janus
wrappers, executes runtime memory inputs on a compatible model, and compares
dumped raw words, lane order, widened values and a trailing canary. It uses the
model's documented `--stop_pc` observer at a separate function reached only
after the kernel has stored all outputs. This establishes numerical execution;
it does not claim
architectural kernel End, runtime lifecycle or multi-PE completion acceptance.

Set `LINX_CXX` and `LINX_SYSROOT` for target compilation and `LINX_MODEL_ROOT`
for execution. Compiler and model memory-instruction encodings must match.
An incompatible model is a failure; instruction legality is not weakened.
Execution is explicitly skipped when no model is configured; object compilation
alone does not establish numerical correctness. Optional `LINX_BUILTINS`
supplies compiler-rt for an in-tree Clang. Set `LINX_BF16_ARTIFACT_DIR` to retain
the executable, raw golden/output, model log and machine-readable identities
and comparison status.

The local issue #325 acceptance run used Clang `a080d17698b809d8535d42a65df9ca8c457baa70`
and the unchanged model `2f474f926a7f8a3d487609f0d9b2bfd44aa3bef8` on Janus.
All 3,164 packed cases and 12,661 output words matched the oracle, including
`0x40003f80`, `0x40804000` and both widened lanes. The newer in-tree compiler
`00c457adbca5b114365a735f52ff0ee13d8bca6a` passed API object/IR checks;
its scoped Scalar memory envelopes are not supported by this model revision,
so its executable is not covered by that numerical result.
