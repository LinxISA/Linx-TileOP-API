#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
TC_DIR=${TC_DIR:?set TC_DIR to the LLVM PR #108 bin directory}
CXX=${CXX:-$TC_DIR/clang++}
OBJDUMP=${OBJDUMP:-$TC_DIR/llvm-objdump}
OBJ=${1:-}
OUT=

cleanup() {
  if [[ -n "$OUT" ]]; then
    rm -rf "$OUT"
  fi
}
trap cleanup EXIT

# With no object argument, build the focused fixture against this checkout.
# This target intentionally requires LLVM PR #108. The hosted required gate's
# LLVM_REF does not provide `%K` until that compiler PR merges.
if [[ -z "$OBJ" ]]; then
  OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-shared-last-use.XXXXXX")
  RESOURCE_DIR="$OUT/clang-resource"
  cp -R "$("$CXX" -print-resource-dir)" "$RESOURCE_DIR"
  rm -rf "$RESOURCE_DIR/include/tileop-api"
  cp -R "$ROOT/include" "$RESOURCE_DIR/include/tileop-api"
  OBJ="$OUT/SharedLastUse.o"

  FLAGS=(-resource-dir "$RESOURCE_DIR" --target=linx64v5-unknown-linux-musl
         -c -mlxbc -fenable-matrix -O2 -std=c++20 -D__linx
         -DENABLE_TENSOR_INSTR)
  if [[ -n "${LINX_SYSROOT:-}" ]]; then
    FLAGS+=(--sysroot="$LINX_SYSROOT" -nostdinc++
            -isystem "$LINX_SYSROOT/usr/include/c++/v1")
  fi
  "$CXX" "${FLAGS[@]}" "$SCRIPT_DIR/src/SharedLastUse.cpp" -o "$OBJ"
fi

DISASSEMBLY=$($OBJDUMP -d "$OBJ")
if [[ -z "$OUT" ]]; then
  OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-shared-last-use-dis.XXXXXX")
fi
DIS_FILE="$OUT/disassembly.txt"
printf '%s\n' "$DISASSEMBLY" > "$DIS_FILE"

python3 - "$DIS_FILE" <<'PY'
import re
import sys

text = open(sys.argv[1], encoding="utf-8").read()

def function_body(fragment: str) -> str:
    match = re.search(
        rf"^[0-9a-f]+ <[^>]*{fragment}[^>]*>:\n(.*?)(?=^Disassembly of section|\Z)",
        text,
        re.MULTILINE | re.DOTALL,
    )
    if not match:
        raise SystemExit(f"FAIL: missing disassembly for {fragment}")
    return match.group(1)

def shared_uses(body: str) -> tuple[int, int]:
    retain = 0
    kill = 0
    for line in body.splitlines():
        if "TMATMUL" not in line and "B.IOS" not in line:
            continue
        for operand in re.findall(r"\bS\d+(?:\.reuse)?\b", line):
            if operand.endswith(".reuse"):
                retain += 1
            else:
                kill += 1
    return retain, kill

local = shared_uses(function_body("local_shared_last_use"))
shared = shared_uses(function_body("shared_shared_last_use"))

if local != (1, 2):
    raise SystemExit(
        f"FAIL: local/shared branch expected 1 retain + 2 kills, got {local}"
    )
if shared != (0, 2):
    raise SystemExit(
        f"FAIL: shared/shared branch expected 0 retains + 2 kills, got {shared}"
    )

# At least one kill remains a standalone B.IOS after bundle pretty-printing.
# Its raw word must have bit26 set (0x04000000).
if not re.search(r"^\s*[0-9a-f]+:\s+0[4567][0-9a-f]{6}\s+B\.IOS", text, re.MULTILINE):
    raise SystemExit("FAIL: no last-use B.IOS raw word with bit26=1")

print("PASS: default retain plus 4 explicit last-use Shared operands")
PY
