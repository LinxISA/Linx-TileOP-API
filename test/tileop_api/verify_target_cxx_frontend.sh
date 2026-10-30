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

for source in RangeSubview.cpp GMov.cpp TileRegionUnary.cpp \
              TileRegionCubeSubview.cpp Issue166RmsNormB128.cpp \
              TileRegionUnarySubviewAssembly.cpp \
              TileRegionTCVTSubviewAssembly.cpp \
              TileRegionTCVTSubview.cpp \
              TileRegionShift.cpp \
              Issue241ReductionPrefixBinary.cpp \
              Issue253RowExpandDualSubview.cpp \
              Issue268CooperativeRowMax.cpp \
              Issue288CubeM32BinaryAssembly.cpp \
              Issue299MixedBinaryAssembly.cpp \
              TOrAssSubview.cpp \
              TileArrayTCVTE8M0.cpp \
              SharedTransposeNonSquare.cpp TCI.cpp TRowExpandMul.cpp; do
  "$TC_DIR/clang++" "${FLAGS[@]}" -fsyntax-only \
    "$ROOT/test/tileop_api/src/$source"
done

for source in TileRegionUnary TileRegionCubeSubview Issue166RmsNormB128; do
  "$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
    "$ROOT/test/tileop_api/src/$source.cpp" -o "$OUT/$source.ll"
  "$TC_DIR/clang++" "${FLAGS[@]}" -c \
    "$ROOT/test/tileop_api/src/$source.cpp" -o "$OUT/$source.o"
done
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/Issue288CubeM32BinaryAssembly.cpp" \
  -o "$OUT/Issue288CubeM32BinaryAssembly.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/Issue288CubeM32BinaryAssembly.cpp" \
  -o "$OUT/Issue288CubeM32BinaryAssembly.o"
python3 - "$OUT/Issue288CubeM32BinaryAssembly.ll" <<'PY_ISSUE288'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
body = next((part for part in re.split(r"(?=^define )", text,
                                       flags=re.MULTILINE)
             if "assemble_binary_cube_m32" in part), None)
if body is None:
    raise SystemExit("Issue288: missing binary CubeM32 assembly fixture")
ops = [line for line in body.splitlines()
       if "asm sideeffect" in line and "BSTART.TEPL" in line]
if len(ops) != 8:
    raise SystemExit(f"Issue288: expected 8 producers, got {len(ops)}")
if any("B.DATR CUBE_M32" not in op for op in ops):
    raise SystemExit("Issue288: every producer must preserve CUBE_M32 layout")
if any("B.SUBVIEW" in op or "TCVT" in op or "TMOV" in op for op in ops):
    raise SystemExit("Issue288: ordinary sources must not be materialized or subviewed")
if any("B.ASSEMBLE" not in op for op in ops):
    raise SystemExit("Issue288: every producer must write an assembly slot")
if "mask=1111, last, ->$0<${7:Z}>" not in ops[0]:
    raise SystemExit("Issue288: INIT producer must allocate the parent")
for op in ops[1:]:
    between = op.split("B.IOT", 1)[1].split("B.ASSEMBLE", 1)[0]
    if between.count("B.IOT") != 1 or "mask=1111, last" not in between:
        raise SystemExit("Issue288: continuation must bind the existing ParentRef last")
# The operand tail is ParentSize, Offset, Opcode, WriterSize, INIT, LAST,
# Layout.  ParentSize is the 1 KiB parent (4); WriterSize is 128 bytes (1).
tails = []
for op in ops:
    match = re.search(
        r"i32 4, i64 ([0-7]), i32 0, i32 1, "
        r"i1 (true|false), i1 (true|false), i32 29\)", op)
    if match is None:
        raise SystemExit("Issue288: parent capacity or writer extent was lost")
    tails.append((int(match.group(1)), match.group(2), match.group(3)))
expected = [(0, "true", "false")]
expected += [(offset, "false", "false") for offset in range(1, 7)]
expected += [(7, "false", "true")]
if tails != expected:
    raise SystemExit(f"Issue288: incorrect offsets or lifecycle {tails}")
PY_ISSUE288
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/Issue299MixedBinaryAssembly.cpp" \
  -o "$OUT/Issue299MixedBinaryAssembly.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/Issue299MixedBinaryAssembly.cpp" \
  -o "$OUT/Issue299MixedBinaryAssembly.o"
python3 - "$OUT/Issue299MixedBinaryAssembly.ll" <<'PY_ISSUE299'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
for name, selector, source_offset in (
        ("assemble_view_tile", 0, 7),
        ("assemble_tile_view", 1, 6)):
    body = next((part for part in functions if name in part), None)
    if body is None:
        raise SystemExit(f"Issue299: missing mixed assembly fixture {name}")
    ops = [line for line in body.splitlines()
           if "asm sideeffect" in line and "BSTART.TEPL" in line]
    if len(ops) != 8:
        raise SystemExit(f"Issue299: {name} expected 8 producers, got {len(ops)}")
    if any("B.DATR CUBE_M32" not in op for op in ops):
        raise SystemExit(f"Issue299: {name} did not preserve CUBE_M32 layout")
    if any(op.count("B.SUBVIEW") != 1 or
           f"B.SUBVIEW {selector}," not in op for op in ops):
        raise SystemExit(f"Issue299: {name} has an incorrect source selector")
    if any("B.ASSEMBLE" not in op for op in ops):
        raise SystemExit(f"Issue299: {name} is missing destination assembly")
    if any(token in op for op in ops for token in ("TADDS", "TCVT", "TMOV")):
        raise SystemExit(f"Issue299: {name} materialized a mixed source")
    if "mask=1111, last, ->$0<${7:Z}>" not in ops[0]:
        raise SystemExit(f"Issue299: {name} INIT did not allocate the parent")
    for op in ops[1:]:
        between = op.split("B.SUBVIEW", 1)[1].split("B.ASSEMBLE", 1)[0]
        if between.count("B.IOT") != 1 or "mask=1111, last" not in between:
            raise SystemExit(
                f"Issue299: {name} continuation did not bind ParentRef last")
    tails = []
    for op in ops:
        match = re.search(
            rf"i32 4, i64 {source_offset}, i64 ([0-7]), i32 0, "
            r"i32 1, i32 1, i1 (true|false), i1 (true|false), i32 29\)",
            op)
        if match is None:
            raise SystemExit(
                f"Issue299: {name} lost parent, subview, or writer capacity")
        tails.append((int(match.group(1)), match.group(2), match.group(3)))
    expected = [(0, "true", "false")]
    expected += [(offset, "false", "false") for offset in range(1, 7)]
    expected += [(7, "false", "true")]
    if tails != expected:
        raise SystemExit(f"Issue299: {name} lifecycle mismatch: {tails}")

all_ops = next((part for part in functions
                if "instantiate_all_mixed_binary" in part), None)
if all_ops is None:
    raise SystemExit("Issue299: missing all-opcode instantiation fixture")
ops = [line for line in all_ops.splitlines()
       if "asm sideeffect" in line and "BSTART.TEPL" in line]
if len(ops) != 24:
    raise SystemExit(f"Issue299: expected 24 mixed opcode instances, got {len(ops)}")
if sum("B.SUBVIEW 0," in op for op in ops) != 12 or \
        sum("B.SUBVIEW 1," in op for op in ops) != 12:
    raise SystemExit("Issue299: all binary operations must support both mixed orders")
PY_ISSUE299
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/RangeAssemble.cpp" -o "$OUT/RangeAssemble.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/RangeAssemble.cpp" -o "$OUT/RangeAssemble.o"
python3 - "$OUT/TileRegionUnary.ll" "$OUT/TileRegionCubeSubview.ll" \
          "$OUT/Issue166RmsNormB128.ll" "$OUT/RangeAssemble.ll" <<'PY'
import re
import sys
from pathlib import Path


def region_operations(path):
    return [line for line in Path(path).read_text(encoding="utf-8").splitlines()
            if "asm sideeffect" in line and "BSTART.TEPL" in line
            and "B.SUBVIEW" in line]


unary = region_operations(sys.argv[1])
if len(unary) != 9:
    raise SystemExit(f"TileRegionUnary: expected 9 region operations, got {len(unary)}")
for operation in unary:
    if "i32 16, i32 32, i32 16, i32 5, i32 5" not in operation:
        raise SystemExit("TileRegionUnary: LB2 does not use the 16-column fragment")

cube = region_operations(sys.argv[2])
if len(cube) != 1:
    raise SystemExit(f"TileRegionCubeSubview: expected 1 region operation, got {len(cube)}")
if "i32 16, i32 32, i32 16, i32 5, i32 5" not in cube[0]:
    raise SystemExit("TileRegionCubeSubview: LB2 does not use the 16-column fragment")
if "i32 16, i32 32, i32 64, i32 5, i32 5" in cube[0]:
    raise SystemExit("TileRegionCubeSubview: LB2 leaked the 64-column parent geometry")

rowsum = [line for line in region_operations(sys.argv[3]) if "i32 64, i64" in line]
expected = (
    "i32 8, i32 32, i32 8, i32 4, i32 4, i32 64, i64 0",
    "i32 8, i32 8, i32 8, i32 2, i32 2, i32 64, i64 0",
)
if len(rowsum) != len(expected):
    raise SystemExit(f"Issue166RmsNormB128: expected 2 TROWSUM regions, got {len(rowsum)}")
for operands in expected:
    if not any(operands in operation for operation in rowsum):
        raise SystemExit(f"Issue166RmsNormB128: missing source geometry {operands}")

range_assemble = Path(sys.argv[4]).read_text(encoding="utf-8")
body = next((part for part in re.split(r"(?=^define )", range_assemble,
                                       flags=re.MULTILINE)
             if "assemble_tadd_session" in part), None)
if body is None:
    raise SystemExit("RangeAssemble: missing TADD assembly-session fixture")
operations = [line for line in body.splitlines()
              if "asm sideeffect" in line and "BSTART.TEPL" in line]
if len(operations) != 3:
    raise SystemExit(f"RangeAssemble: expected 3 TADD operations, got {len(operations)}")
if "B.IOT $5, $6, mask=1111, last, ->$0" not in operations[0]:
    raise SystemExit("RangeAssemble: INIT TADD must allocate its destination")
if "B.ASSEMBLE ${8:c}, ${9:c}" not in operations[0]:
    raise SystemExit("RangeAssemble: INIT TADD is missing B.ASSEMBLE")
for operation in operations[1:]:
    if "B.IOT $4, $5, mask=1111" not in operation or \
       "B.IOT $7, mask=1111, last" not in operation:
        raise SystemExit("RangeAssemble: continuation TADD has invalid binders")
    if "B.ASSEMBLE" not in operation:
        raise SystemExit("RangeAssemble: continuation TADD is missing B.ASSEMBLE")
PY

# Typed factories carry a seventh WriterSizeCode template argument. Verify
# continuation overload selection and real object code for external parents.
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/TileArrayWriterSize.cpp" -o "$OUT/TileArrayWriterSize.ll"
"$TC_DIR/clang++" "${FLAGS[@]}" -c \
  "$ROOT/test/tileop_api/src/TileArrayWriterSize.cpp" -o "$OUT/TileArrayWriterSize.o"
# Shared references still have a separate backend handle-copy gap; verify
# descriptor lowering in IR without claiming Shared object/runtime closure.
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/SharedTloadAssembly.cpp" -o "$OUT/SharedTloadAssembly.ll"
python3 - "$OUT/TileArrayWriterSize.ll" "$OUT/SharedTloadAssembly.ll" <<'PY_ASSEMBLE'
import re
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text()
body = next(part for part in re.split(r"(?=^define )", text, flags=re.MULTILINE)
            if part.startswith("define ") and "assemble_writer_size" in part)
ops = [line for line in body.splitlines() if "asm sideeffect" in line]
if len(ops) != 4 or any("B.ASSEMBLE" not in op for op in ops):
    raise SystemExit("TileArrayWriterSize: an INIT/MIDDLE/LAST descriptor was lost")
for op, offset, last in zip(ops[1:], (4, 8, 12), (0, 0, 1)):
    if "->" in op.split("B.IOT", 1)[1].split("B.ASSEMBLE", 1)[0]:
        raise SystemExit("TileArrayWriterSize: continuation allocated a new parent")
    if f"i32 0, i32 {last}, i32 {offset}, i32 3)" not in op:
        raise SystemExit("TileArrayWriterSize: incorrect continuation extent or state")
text = Path(sys.argv[2]).read_text()
body = next(part for part in re.split(r"(?=^define )", text, flags=re.MULTILINE)
            if part.startswith("define ") and "assemble_shared_continuations" in part)
ops = [line for line in body.splitlines() if "asm sideeffect" in line]
if len(ops) != 2 or any("B.ASSEMBLE" not in op or "B.IOS" not in op for op in ops):
    raise SystemExit("TileArrayWriterSize: Shared continuation lost its descriptor")
for op, offset, last in zip(ops, (4, 12), (0, 1)):
    if "->" in op.split("B.IOS", 1)[1].split("B.ASSEMBLE", 1)[0] or \
       f"i32 0, i32 {last}, i32 {offset}, i32 3)" not in op:
        raise SystemExit("TileArrayWriterSize: Shared continuation rebound its parent")
PY_ASSEMBLE

"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/TRowExpandMul.cpp" -o "$OUT/TRowExpandMul.ll"
python3 - "$OUT/TRowExpandMul.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
for name in ("test_row_vector_src1", "test_col_vector_src1"):
    body = next((part for part in functions
                 if part.startswith("define ") and name in part
                 and "BSTART.TEPL" in part), None)
    if body is None:
        raise SystemExit(f"missing TROWEXPAND fixture {name}")
    # LLVM prints each inline-assembly bundle as one string literal, often on
    # one physical line.  Count TEPL bundles directly; TLOAD is TLSU and must
    # never be used as a subtraction-based proxy for the operation count.
    bundles = re.findall(r"BSTART\.TEPL\s+([0-9]+)", body)
    if len(bundles) != 8:
        raise SystemExit(f"{name}: expected 8 TEPL operations, got {len(bundles)}")
    expected = {"test_row_vector_src1": {"68", "69", "70", "71", "72", "73", "74", "75"},
                "test_col_vector_src1": {"84", "85", "86", "87", "88", "89", "90", "91"}}[name]
    if set(bundles) != expected:
        raise SystemExit(f"{name}: unexpected TEPL operation codes {bundles}")
    operations = [operation for operation in re.split(r"(?=BSTART\.TEPL)", body)
                  if "BSTART.TEPL" in operation]
    if len(operations) != 8:
        raise SystemExit(f"{name}: failed to split all TEPL bundles")
    for operation in operations:
        if "BSTART.TEPL" not in operation:
            continue
        if "B.DATR" not in operation:
            raise SystemExit(f"{name}: every TEPL operation needs B.DATR")
        # Region/SubView inline-asm carries B.SUBVIEW, while the direct jcore
        # path binds the source tile directly.  Validate ordering when present
        # without imposing the Region representation on both implementations.
        if "B.SUBVIEW" in operation and operation.index("B.SUBVIEW") > operation.index("B.DATR"):
            raise SystemExit(f"{name}: B.DATR must follow B.SUBVIEW")
PY

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

# Issue #253: a dual-SubTileView row expansion binds a 2KB matrix fragment and
# a 128B broadcast carrier fragment.  Syntax acceptance is not enough: each
# B.SUBVIEW must carry the size code of its own fragment.
"$TC_DIR/clang++" "${FLAGS[@]}" -S -emit-llvm \
  "$ROOT/test/tileop_api/src/Issue253RowExpandDualSubview.cpp" \
  -o "$OUT/Issue253RowExpandDualSubview.ll"
python3 - "$OUT/Issue253RowExpandDualSubview.ll" <<'PY'
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(encoding="utf-8")
functions = re.split(r"(?=^define )", text, flags=re.MULTILINE)
for name, offset in (("dual_view_slot0", 0), ("dual_view_slot1", 2)):
    body = next((part for part in functions if name in part), None)
    if body is None:
        raise SystemExit(f"missing Issue #253 fixture {name}")
    asm = [line for line in body.splitlines()
           if "asm sideeffect" in line and "BSTART.TEPL" in line]
    if len(asm) != 7:
        raise SystemExit(f"{name}: expected 7 TEPL operations, got {len(asm)}")
    opcodes = set()
    for line in asm:
        sizes = {int(select): int(operand) for select, operand in re.findall(
            r"B\.SUBVIEW ([01]), \$[0-9]+, 0, \$\{([0-9]+):c\}", line)}
        select = re.search(r"EQ, \$\{([0-9]+):c\}", line)
        opcode = re.search(r"BSTART\.TEPL \$\{([0-9]+):c\}", line)
        if set(sizes) != {0, 1} or select is None or opcode is None:
            raise SystemExit(f"{name}: missing source-specific B.SUBVIEW modifier")
        # Input operand N of the template is the N-th call argument; the
        # immediates are the only i32 arguments.
        args = line.rsplit('"(', 1)[1].split(") #", 1)[0].split(", ")
        value = lambda operand: int(args[operand - 1].removeprefix("i32 "))
        if (value(sizes[0]), value(sizes[1])) != (5, 1):
            raise SystemExit(
                f"{name}: B.SUBVIEW sizes {value(sizes[0])}/{value(sizes[1])}, "
                "expected 2KB matrix fragment and 128B broadcast fragment")
        if value(int(select.group(1))) != offset:
            raise SystemExit(f"{name}: unexpected broadcast byte offset")
        opcodes.add(value(int(opcode.group(1))))
    if opcodes != {69, 70, 71, 72, 73, 74, 75}:
        raise SystemExit(f"{name}: unexpected TEPL operation codes {opcodes}")
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
