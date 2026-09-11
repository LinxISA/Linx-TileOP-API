#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
LINX_SYSROOT=${LINX_SYSROOT:?set LINX_SYSROOT to the musl libc++ sysroot}
LINX_TARGET=${LINX_TARGET:-linx64v5-unknown-linux-musl}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-target-cxx.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

# The driver searches its builtin tileop-api before ordinary -I directories.
# Use a copied resource tree whose tileop-api subtree is this checkout, so the
# verification cannot accidentally exercise stale installed headers.
RESOURCE_DIR="$OUT/clang-resource"
cp -R "$("$TC_DIR/clang++" -print-resource-dir)" "$RESOURCE_DIR"
rm -rf "$RESOURCE_DIR/include/tileop-api"
cp -R "$ROOT/include" "$RESOURCE_DIR/include/tileop-api"

FLAGS=(
  -resource-dir "$RESOURCE_DIR"
  --target="$LINX_TARGET"
  -mlxbc
  --sysroot="$LINX_SYSROOT"
  -nostdinc++
  -isystem "$LINX_SYSROOT/usr/include/c++/v1"
  -fenable-matrix
  -O2
  -std=c++20
  -D__linx
  -DENABLE_TENSOR_INSTR
)

TRACE="$OUT/resource-include.trace"
if ! "$TC_DIR/clang++" "${FLAGS[@]}" -H -fsyntax-only \
     "$ROOT/test/tileop_api/src/RangeSubview.cpp" \
     >/dev/null 2>"$TRACE" ||
   ! grep -Fq "$RESOURCE_DIR/include/tileop-api/common/pto_tileop.hpp" "$TRACE"; then
  echo "FAIL: compiler did not select checkout tileop-api headers" >&2
  sed -n '1,30p' "$TRACE" >&2
  exit 1
fi

for source in RangeSubview.cpp GMov.cpp TileRegionCubeSubview.cpp \
              TileRegionUnarySubviewAssembly.cpp \
              TileRegionTCVTSubviewAssembly.cpp \
              SharedTransposeNonSquare.cpp; do
  "$TC_DIR/clang++" "${FLAGS[@]}" -fsyntax-only \
    "$ROOT/test/tileop_api/src/$source"
done

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/GMov.cpp" -o "$OUT/GMov.ll"
grep -q 'BSTART.GMOV' "$OUT/GMov.ll"
for mask in 0011 0101 1010 1101 1111 0001; do
  grep -q "mask=$mask" "$OUT/GMov.ll"
done

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/SharedTLoad.cpp" -o "$OUT/SharedTLoad.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/RangeSubview.cpp" -o "$OUT/RangeSubview.ll"
grep -q '<256 x i32> asm sideeffect' "$OUT/SharedTLoad.ll"
grep -q 'i64 asm sideeffect.*=@2Sr' "$OUT/SharedTLoad.ll"
grep -q '<256 x i32> asm sideeffect.*@2Sr' "$OUT/SharedTLoad.ll"
if grep -q 'store i64' "$OUT/SharedTLoad.ll"; then
  echo "FAIL: Shared handle materialized to memory" >&2
  exit 1
fi

python3 - "$OUT/RangeSubview.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
expected = (
    "subview_source_tstore",
    "subview_size12_tstore",
    "subview_factory_zero_tstore",
    "subview_factory_runtime_tstore",
    "subview_factory_zero_offset_tstore",
    "subview_factory_runtime_offset_tstore",
    "subview_factory_explicit_reg_tstore",
)
for name in expected:
    body = next((part for part in functions if name in part), None)
    if body is None:
        raise SystemExit(f"missing range fixture {name}")
    asm = [line for line in body.splitlines() if "asm sideeffect" in line]
    if not any("B.IOT" in line for line in asm):
        raise SystemExit(f"{name}: missing source B.IOT binder")
    if not any("B.SUBVIEW" in line for line in asm):
        raise SystemExit(f"{name}: missing B.SUBVIEW modifier")
PY

echo "Linx target C++ frontend range/GMOV contract: PASS"

# Keep the pre-existing MX carrier/assembly gate after the range/GMOV gate so
# an unrelated MX failure cannot prevent the new contract checks from running.
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/MXScaleVariants.cpp" -o "$OUT/MXScaleVariants.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/MXScaleVariants.cpp" -o "$OUT/MXScaleVariants.o"
"$TC_DIR/llvm-objdump" -d "$OUT/MXScaleVariants.o" > "$OUT/MXScaleVariants.dis"

python3 - "$OUT/MXScaleVariants.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
expected = {
    "carrier_zero_scale": (3, 5, 4),
    "carrier_scale_a": (4, 6, 5),
    "carrier_scale_b": (4, 6, 5),
    "carrier_both_scales": (5, 6, 6),
    "carrier_shared_zero_scale": (3, 5, 4),
    "carrier_shared_scale_a": (4, 6, 5),
    "carrier_shared_scale_b": (4, 6, 5),
    "carrier_shared_both_scales": (5, 7, 6),
    "carrier_gemv_zero_scale": (3, 5, 4),
    "carrier_gemv_scale_a": (4, 6, 5),
    "carrier_gemv_scale_b": (4, 6, 5),
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

echo "Linx target C++ frontend MX/effective-shape contract: PASS"
