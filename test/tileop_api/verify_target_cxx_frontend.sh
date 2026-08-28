#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
LINX_SYSROOT=${LINX_SYSROOT:?set LINX_SYSROOT to the musl libc++ sysroot}
LINX_TARGET=${LINX_TARGET:-linx64-unknown-linux-musl}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-target-cxx.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

FLAGS=(
  --target="$LINX_TARGET"
  -mlxbc
  --sysroot="$LINX_SYSROOT"
  -nostdinc++
  -isystem "$LINX_SYSROOT/include/c++/v1"
  -fenable-matrix
  -O2
  -std=c++20
  -D__linx
  -DENABLE_TENSOR_INSTR
  -I"$ROOT/include"
)

for source in MXScaleVariants.cpp SharedTransposeNonSquare.cpp; do
  "$TC_DIR/clang++" "${FLAGS[@]}" -fsyntax-only \
    "$ROOT/test/tileop_api/src/$source"
done

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/SharedTLoad.cpp" -o "$OUT/SharedTLoad.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/MXScaleVariants.cpp" -o "$OUT/MXScaleVariants.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/MXScaleVariants.cpp" -o "$OUT/MXScaleVariants.o"
"$TC_DIR/llvm-objdump" -d "$OUT/MXScaleVariants.o" > "$OUT/MXScaleVariants.dis"
grep -q '<1024 x i32> asm sideeffect' "$OUT/SharedTLoad.ll"
grep -q 'i64 asm sideeffect.*=\^Sr' "$OUT/SharedTLoad.ll"
grep -q '<1024 x i32> asm sideeffect.*\^Sr' "$OUT/SharedTLoad.ll"
if grep -q 'store i64' "$OUT/SharedTLoad.ll"; then
  echo "FAIL: Shared handle materialized to memory" >&2
  exit 1
fi

python3 - "$OUT/MXScaleVariants.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
expected = {
    "carrier_zero_scale": (3, 4, 4),
    "carrier_scale_a": (4, 5, 5),
    "carrier_scale_b": (4, 5, 5),
    "carrier_both_scales": (5, 6, 6),
    "carrier_shared_zero_scale": (3, 4, 4),
    "carrier_shared_scale_a": (4, 5, 5),
    "carrier_shared_scale_b": (4, 5, 5),
    "carrier_shared_both_scales": (5, 6, 6),
    "carrier_gemv_zero_scale": (3, 4, 4),
    "carrier_gemv_scale_a": (4, 5, 5),
    "carrier_gemv_scale_b": (4, 5, 5),
    "carrier_gemv_both_scales": (5, 6, 6),
}
for name, counts in expected.items():
    body = next((part for part in functions if name in part), None)
    if body is None:
        raise SystemExit(f"missing object-producing function {name}")
    calls = [line for line in body.splitlines()
             if "asm sideeffect" in line and
             ("BSTART.TMATMULMX" in line or "BSTART.TGEMVMX" in line)]
    if len(calls) != 3:
        raise SystemExit(f"{name}: expected 3 MX family calls, got {len(calls)}")
    constraints = [line.split('"', 4)[3] for line in calls]
    actual = tuple(item.count("^Tr") + item.count("^Sr")
                   for item in constraints)
    if actual != counts:
        raise SystemExit(f"{name}: tile constraints {actual}, expected {counts}")

post = next(part for part in functions if "carrier_postprocess_all_sources" in part)
post_calls = [line for line in post.splitlines()
              if "asm sideeffect" in line and "BSTART.TMATMULMX" in line]
if len(post_calls) != 1:
    raise SystemExit("postprocess carrier must emit exactly one MX call")
constraints = post_calls[0].split('"', 4)[3]
if constraints.count("^Tr") + constraints.count("^Sr") != 9:
    raise SystemExit("postprocess MX call must bind 2 outputs + 7 present inputs")
PY

grep -Eq 'BSTART\.TMATMULMX[[:space:]]+FP16' "$OUT/MXScaleVariants.dis"
grep -Eq 'BSTART\.TMATMULMX\.ACC[[:space:]]+E4M3' "$OUT/MXScaleVariants.dis"
grep -Eq 'BSTART\.TGEMVMX\.BIAS[[:space:]]+BF16' "$OUT/MXScaleVariants.dis"
grep -Eq 'B\.IOT[[:space:]]+t#[1-8], t#[1-8], mask=1111' "$OUT/MXScaleVariants.dis"

echo "Linx target C++ frontend MX/effective-shape contract: PASS"
