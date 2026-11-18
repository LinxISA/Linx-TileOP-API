# Janus scalar conversion

`#include <common/pto_tileop.hpp>` exposes `linx_cvt<RMode, Sat>(dst, src)`.
On non-SIMT Janus the supported type pair is FP32 (`float`) to S32
(`int32_t`). Both `LINX_NOSAT` and `LINX_SAT` are supported. The source is
read before the destination is written.

```cpp
int32_t result;
float input = -2.5f;
linx_cvt<LINX_RNE>(result, input); // -2
linx_cvt<LINX_RNA>(result, input); // -3
linx_cvt<LINX_RHB>(result, input); // -2
auto rounded = linx_cvt_as<LINX_RNE, LINX_NOSAT, int32_t>(input);
auto default_rounded = linx_cvt_as<int32_t>(input);
```

| Mode | FP32 to S32 behavior |
| --- | --- |
| `LINX_RNONE`, `LINX_RNE` | Nearest, ties to even. The API default is fixed RNE, independent of scalar FRM. |
| `LINX_RTZ` | Towards zero. |
| `LINX_RDN` | Towards negative infinity. |
| `LINX_RUP` | Towards positive infinity. |
| `LINX_RNA` | Nearest, ties away from zero. |
| `LINX_RTO` | Exact integers unchanged; truncate inexact inputs, then adjust an even result one unit away from zero to make it odd. |
| `LINX_RHB` | Nearest, ties towards positive infinity. |

The five directly encoded modes use scalar `FCVTN/Z/M/P/A.fs2sw` with
64-bit GPR carriers. RTO and RHB use FCVTZ followed by exact integer
correction of fractional FP32 inputs. No SIMT register or floating cast is
used to approximate these modes.

Finite overflow wraps modulo 2^32 with `LINX_NOSAT`, and clamps to the signed
32-bit endpoints with `LINX_SAT`. NaNs return zero; positive and negative
infinity return `INT32_MAX` and `INT32_MIN` respectively. Both zero encodings
return integer zero. The native scalar conversion supplies sticky numeric
status; integer correction does not clear or otherwise modify that status.
RTO/RHB corrections apply only to fractional values with magnitude below
2^23, which cannot overflow S32.

These result rules follow the common conversion domain at PTO ASL commit
`0179a7d36bcc9f3a96ebdb34663dd481072e983f`, particularly
`asl/scalar/model/fsu/arithmetic.asl`,
`asl/tile/model/numeric/reference-conversion.asl`, and
`asl/tile/model/execution/matrix-quantization.asl`. Native status behavior
depends on the target implementing that scalar conversion contract.

Other normal type pairs, packed conversion, P2P conversion, and packed
address-space overloads produce an API diagnostic on Janus. This includes
`linx_cvt_as`, `linx_cvt_packed`, `linx_cvt_package`, and
`linx_cvt_package_as`. Use the separately documented `TCVT` API for supported
Local Tile conversions; its shape, capacity and type rules still apply.

Legacy `v.cvt` overloads are isolated behind the compiler-defined
`__linx_simt=1` capability on an explicitly selected SIMT target. Do not
define that macro yourself or enable SIMT for Janus. Compilers without the
capability macro select the restricted Janus implementation.

## Verification

```bash
LINX_CXX=/path/to/clang++ LINX_SYSROOT=/path/to/sysroot \
LINX_MODEL_ROOT=/path/to/SuperScalarModel \
PTO_CVT_OUTPUT_DIR=/tmp/scalar-cvt-results \
python3 test/test_scalar_cvt.py -v
```

The test compiles candidate headers into objects at O0/O2/O3, checks native
conversion mnemonics and invalid calls, and compares the native ELF's memory
dump to an independent exact rational oracle. Its single-PE debug launcher
stops at an explicit PC after `main` returns; this proves exported numerical
results, not the hosted kernel completion protocol. The retained directory contains
commands, logs, objects, ELF and expected/observed bytes. Without
`LINX_MODEL_ROOT`, model execution is explicitly skipped. Host checks cover
the integer correction with an undefined-behavior sanitizer and do not
establish native execution or numeric-status correctness.

At SSM commit `911702dbcf29561cedb4781c4cbab091dabb8a7e`, the scalar conversion
dispatcher passes `FRMMode` (RNE=1 through RNA=5) directly to a converter that
expects RNE=0 through RNA=4. This makes RNE behave as RTZ and RNA assert.
The native numerical test fails on that model. The full ELF built with the
local LLVM 0.59 compiler also encounters an earlier `swi.intra` decoding
fault (`encoded carrier has no assigned operation`) before numerical
comparison. Neither the golden values nor model legality are adjusted to
accommodate these failures. Numeric status also needs independent target
verification. Object generation and host correction checks alone do not
establish full functional acceptance.
