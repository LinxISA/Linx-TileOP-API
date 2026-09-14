#!/usr/bin/env bash
# Issue #129 / #28: public consumer macros must not rename inline-asm labels.
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
COMPILER_DIR=${COMPILER_DIR:-/home/zhuwei/linx-llvm/build/bin}
SYSROOT=${SYSROOT:-${LINX_SYSROOT:-/home/zhuwei/linx-BLK-build/output/linx_blockisa_llvm_musl/sysroot}}
CXX=${CXX:-"${COMPILER_DIR}/clang++"}
SRC=${ROOT}/test/tileop_api/src/DTypeMacro.cpp

# The Linx compiler's resource directory contains a copy of TileOP headers.
# Overlay it so this regression always tests the checkout under ROOT.
RESOURCE_OVERLAY=$(mktemp -d "${TMPDIR:-/tmp}/tileop-dtype-resource.XXXXXX")
trap 'rm -rf "${RESOURCE_OVERLAY}"' EXIT
cp -R "$("${CXX}" -print-resource-dir)"/. "${RESOURCE_OVERLAY}/"
rm -rf "${RESOURCE_OVERLAY}/include/tileop-api"
cp -R "${ROOT}/include" "${RESOURCE_OVERLAY}/include/tileop-api"

"${CXX}" \
  --target=linx64v5-unknown-linux-musl \
  --sysroot="${SYSROOT}" \
  -resource-dir="${RESOURCE_OVERLAY}" \
  -mlxbc -fenable-matrix -std=c++20 \
  -I"${ROOT}/include" \
  -DDType=__half \
  -fsyntax-only "${SRC}"

echo "DType macro regression (TCVT_ASS and all special _ASS emitters): PASS"
