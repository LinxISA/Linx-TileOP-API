#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
LINX_SYSROOT=${LINX_SYSROOT:?set LINX_SYSROOT to the musl libc++ sysroot}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)

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
  -fsyntax-only
)

for source in MXScaleVariants.cpp SharedTransposeNonSquare.cpp; do
  "$TC_DIR/clang++" "${FLAGS[@]}" "$ROOT/test/tileop_api/src/$source"
done

echo "Linx target C++ frontend MX/effective-shape contract: PASS"
