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
              TileRegionTCVTSubview.cpp \
              TileRegionShift.cpp \
              Issue241ReductionPrefixBinary.cpp \
              TOrAssSubview.cpp \
              TileArrayTCVTE8M0.cpp \
              SharedTransposeNonSquare.cpp; do
  "$TC_DIR/clang++" "${FLAGS[@]}" -fsyntax-only \
    "$ROOT/test/tileop_api/src/$source"
done

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/Issue241ReductionPrefixBinary.cpp" \
  -o "$OUT/Issue241ReductionPrefixBinary.ll"
python3 - "$OUT/Issue241ReductionPrefixBinary.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
for name in ("prefix_prefix_arithmetic", "prefix_prefix_bitwise"):
    body = next((part for part in functions if name in part), None)
    if body is None:
        raise SystemExit(f"missing Issue #241 fixture {name}")
    asm = [line for line in body.splitlines()
           if "asm sideeffect" in line and "BSTART.TEPL" in line]
    expected = 6 if name == "prefix_prefix_arithmetic" else 4
    if len(asm) != expected:
        raise SystemExit(f"{name}: expected {expected} TEPL operations, got {len(asm)}")
    for line in asm:
        if line.count("B.SUBVIEW") != 2:
            raise SystemExit(f"{name}: every operation must have two B.SUBVIEW modifiers")
        if "B.SUBVIEW 0" not in line or "B.SUBVIEW 1" not in line:
            raise SystemExit(f"{name}: missing source-specific B.SUBVIEW modifier")
        if "TCVT" in line or "TMOV" in line:
            raise SystemExit(f"{name}: unexpected prefix materialization")
PY

# Issue #172: the role view must let one published Shared handle participate in
# both operand slots, including the Shared transpose path, without introducing
# a second load or a Shared move/publication.  Inspect LLVM rather than only
# checking that the source compiles, because the contract is specifically
# zero-instruction and non-owning.
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/SharedRoleView.cpp" -o "$OUT/SharedRoleView.ll"
grep -q 'BSTART.CUBE TMATMUL' "$OUT/SharedRoleView.ll"
if grep -Eq 'BSTART\.(TLOAD|TMOV)' "$OUT/SharedRoleView.ll"; then
  echo "FAIL: SharedRoleView emitted a load or Shared move" >&2
  exit 1
fi
if [[ $(grep -c 'BSTART.CUBE TMATMUL' "$OUT/SharedRoleView.ll") -ne 2 ]]; then
  echo "FAIL: SharedRoleView did not emit exactly two TMATMUL operations" >&2
  exit 1
fi

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

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/RangeAssemble.cpp" -o "$OUT/RangeAssemble.ll"
python3 - "$OUT/RangeAssemble.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
body = next((part for part in re.split(r"(?=^define )", text, flags=re.MULTILINE)
             if "assemble_tadd_subviews" in part), None)
if body is None:
    raise SystemExit("missing TADD_ASS subview fixture")
if body.count("B.SUBVIEW") != 2:
    raise SystemExit("assemble_tadd_subviews: expected two source B.SUBVIEW modifiers")
if body.count("B.ASSEMBLE") != 1:
    raise SystemExit("assemble_tadd_subviews: expected one destination B.ASSEMBLE modifier")
if not re.search(r"B\\.IOT.*B\\.SUBVIEW.*B\\.SUBVIEW.*B\\.IOT.*B\\.ASSEMBLE", body):
    raise SystemExit("assemble_tadd_subviews: invalid source/destination modifier order")
PY
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

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/TOrAssSubview.cpp" -o "$OUT/TOrAssSubview.ll"
python3 - "$OUT/TOrAssSubview.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
for name in ("tor_ass_subview", "tor_ass_explicit_range"):
    body = next((part for part in functions if name in part), None)
    if body is None:
        raise SystemExit(f"missing TOR_ASS subview fixture {name}")
    if body.count("B.SUBVIEW") != 2:
        raise SystemExit(f"{name}: expected two source B.SUBVIEW modifiers")
    if body.count("B.ASSEMBLE") != 1:
        raise SystemExit(f"{name}: expected one destination B.ASSEMBLE modifier")
    if not re.search(r"B\.IOT.*B\.SUBVIEW.*B\.SUBVIEW.*B\.IOT.*B\.ASSEMBLE",
                     body):
        raise SystemExit(f"{name}: invalid source/destination modifier order")
PY

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/TileRegionTCVTSubview.cpp" \
  -o "$OUT/TileRegionTCVTSubview.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/TileRegionShift.cpp" -o "$OUT/TileRegionShift.ll"
python3 - "$OUT/TileRegionTCVTSubview.ll" "$OUT/TileRegionShift.ll" <<'PY'
import re
import sys
from pathlib import Path

tcvt = Path(sys.argv[1]).read_text(encoding="utf-8")
body = next(part for part in re.split(r"(?=^define )", tcvt, flags=re.MULTILINE)
            if "convert_subview" in part)
if "BSTART.TEPL 27" not in body or body.count("B.SUBVIEW") != 1:
    raise SystemExit("convert_subview: missing TCVT opcode or source B.SUBVIEW")
if "B.ASSEMBLE" in body or "B.DIM zero, $2, ->lb2" in body:
    raise SystemExit("convert_subview: ordinary CUBE TCVT emitted assembly/LB2")

shift = Path(sys.argv[2]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", shift, flags=re.MULTILINE)
ordinary = next(part for part in functions if "shift_subview" in part and
                "assemble_shift_subviews" not in part)
for opcode in (9, 10):
    if f"BSTART.TEPL {opcode}" not in ordinary:
        raise SystemExit(f"shift_subview: missing shift opcode {opcode}")
if ordinary.count("B.SUBVIEW") != 2 or "B.ASSEMBLE" in ordinary:
    raise SystemExit("shift_subview: invalid ordinary source modifiers")

assembled = next(part for part in functions if "assemble_shift_subviews" in part)
for opcode in (9, 10):
    if f"BSTART.TEPL {opcode}" not in assembled:
        raise SystemExit(f"assemble_shift_subviews: missing shift opcode {opcode}")
if assembled.count("B.SUBVIEW") != 8 or assembled.count("B.ASSEMBLE") != 4:
    raise SystemExit("assemble_shift_subviews: invalid modifier counts")
for line in (line for line in assembled.splitlines() if "asm sideeffect" in line):
    if "BSTART.TEPL" in line and not re.search(
            r"B\.IOT.*B\.SUBVIEW.*B\.SUBVIEW.*B\.IOT.*B\.ASSEMBLE", line):
        raise SystemExit("assemble_shift_subviews: invalid modifier order")
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
