#!/usr/bin/env bash
# Issue #28: public consumer macros must not rename inline-asm labels.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
COMPILER_DIR=${COMPILER_DIR:-/home/zhuwei/linx-llvm/build/bin}
SYSROOT=${SYSROOT:-/home/zhuwei/linx-BLK-build/output/linx_blockisa_llvm_musl/sysroot}
CXX=${CXX:-"${COMPILER_DIR}/clang++"}
SRC=${ROOT}/test/tileop_api/src/DTypeMacro.cpp

"${CXX}" \
  --target=linx64v5-unknown-linux-musl \
  --sysroot="${SYSROOT}" \
  -mlxbc -fenable-matrix -std=c++20 \
  -I"${ROOT}/include" \
  -DDType=__half \
  -fsyntax-only "${SRC}"

echo "Issue #28 DType macro regression: PASS"
