#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
LINX_SYSROOT=${LINX_SYSROOT:?set LINX_SYSROOT to the musl libc++ sysroot}
LINX_TARGET=${LINX_TARGET:-linx64-unknown-linux-musl}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-target-cxx.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

# -mlxbc injects the TileOP headers installed in Clang's resource directory
# ahead of ordinary -I paths.  Build a temporary resource overlay that keeps
# the toolchain's builtin headers but replaces only tileop-api with this
# checkout, otherwise this gate silently validates the packaged API instead of
# the proposed source tree.
CLANG_RESOURCE_DIR=$("$TC_DIR/clang++" -print-resource-dir)
CLANG_RESOURCE_OVERLAY="$OUT/clang-resource"
mkdir -p "$CLANG_RESOURCE_OVERLAY/include"
for resource_path in "$CLANG_RESOURCE_DIR"/include/*; do
  resource_name=${resource_path##*/}
  if [[ "$resource_name" != tileop-api ]]; then
    ln -s "$resource_path" "$CLANG_RESOURCE_OVERLAY/include/$resource_name"
  fi
done
ln -s "$ROOT/include" "$CLANG_RESOURCE_OVERLAY/include/tileop-api"

FLAGS=(
  --target="$LINX_TARGET"
  -mlxbc
  -resource-dir="$CLANG_RESOURCE_OVERLAY"
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

for source in MXScaleVariants.cpp SharedTransposeNonSquare.cpp TSELCanonical.cpp; do
  "$TC_DIR/clang++" "${FLAGS[@]}" -fsyntax-only \
    "$ROOT/test/tileop_api/src/$source"
done

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/SharedTLoad.cpp" -o "$OUT/SharedTLoad.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/MXScaleVariants.cpp" -o "$OUT/MXScaleVariants.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/MXScaleVariants.cpp" -o "$OUT/MXScaleVariants.o"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/TSELCanonical.cpp" -o "$OUT/TSELCanonical.o"
"$TC_DIR/llvm-objdump" -d "$OUT/MXScaleVariants.o" > "$OUT/MXScaleVariants.dis"
"$TC_DIR/llvm-objdump" -d "$OUT/TSELCanonical.o" > "$OUT/TSELCanonical.dis"
grep -q '<1024 x i32> asm sideeffect' "$OUT/SharedTLoad.ll"
grep -q 'i64 asm sideeffect.*=@2Sr' "$OUT/SharedTLoad.ll"
grep -q '<1024 x i32> asm sideeffect.*@2Sr' "$OUT/SharedTLoad.ll"
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
    "carrier_shared_both_scales": (5, 7, 6),
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
             ("BSTART.CUBE TMATMULMX" in line or "BSTART.CUBE TGEMVMX" in line)]
    if len(calls) != 3:
        raise SystemExit(f"{name}: expected 3 MX family calls, got {len(calls)}")
    constraints = [line.split('"', 4)[3] for line in calls]
    actual = tuple(item.count("@2Tr") + item.count("@2Sr")
                   for item in constraints)
    if actual != counts:
        raise SystemExit(f"{name}: tile constraints {actual}, expected {counts}")

post = next(part for part in functions if "carrier_postprocess_all_sources" in part)
post_calls = [line for line in post.splitlines()
              if "asm sideeffect" in line and "BSTART.CUBE TMATMULMX" in line]
if len(post_calls) != 1:
    raise SystemExit("postprocess carrier must emit exactly one MX call")
constraints = post_calls[0].split('"', 4)[3]
if constraints.count("@2Tr") + constraints.count("@2Sr") != 9:
    raise SystemExit("postprocess MX call must bind 2 outputs + 7 present inputs")
PY

grep -Eiq 'BSTART\.CUBE[[:space:]]+TMATMULMX,[[:space:]]+FP16' "$OUT/MXScaleVariants.dis"
grep -Eiq 'BSTART\.CUBE[[:space:]]+TMATMULMX\.ACC,[[:space:]]+E4M3' "$OUT/MXScaleVariants.dis"
grep -Eiq 'BSTART\.CUBE[[:space:]]+TGEMVMX\.BIAS,[[:space:]]+BF16' "$OUT/MXScaleVariants.dis"
grep -Eq 'B\.IOT[[:space:]]+t#[1-8], t#[1-8], mask=1111' "$OUT/MXScaleVariants.dis"

grep -Eq 'B\.IOT[[:space:]]+[tumn]#[1-8], [tumn]#[1-8], mask=1111[[:space:]]*$' \
  "$OUT/TSELCanonical.dis"
grep -Eq 'B\.IOT[[:space:]]+[tumn]#[1-8], mask=1111, last,.*->[tumn]<[0-9]+(B|KB)>' \
  "$OUT/TSELCanonical.dis"
if grep -Eq 'B\.IOT[[:space:]]+[tumn]#[1-8], [tumn]#[1-8], mask=1111, last,.*->' \
    "$OUT/TSELCanonical.dis"; then
  echo "FAIL: TSEL collapsed to the forbidden single-B.IOT in-place form" >&2
  sed -n '1,120p' "$OUT/TSELCanonical.dis" >&2
  exit 1
fi

echo "Linx target C++ frontend MX/effective-shape contract: PASS"
