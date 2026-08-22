#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
LINX_SYSROOT=${LINX_SYSROOT:?set LINX_SYSROOT to the musl libc++ sysroot}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-target-cxx.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

FLAGS=(
  --target=linx64
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
grep -q '<1024 x i32> asm sideeffect' "$OUT/SharedTLoad.ll"
grep -q 'i64 asm sideeffect.*=\^Sr' "$OUT/SharedTLoad.ll"
grep -q '<1024 x i32> asm sideeffect.*\^Sr' "$OUT/SharedTLoad.ll"
if grep -q 'store i64' "$OUT/SharedTLoad.ll"; then
  echo "FAIL: Shared handle materialized to memory" >&2
  exit 1
fi

echo "Linx target C++ frontend MX/effective-shape contract: PASS"
