#!/usr/bin/env bash
# Negative-test driver: compiles PostProcessNegatives.cpp once per
# -DSHOULD_FAIL_<case> and asserts the compile FAILS (the static_assert fires).
# Usage: TC=<toolchain-bin> ./run_negatives.sh
set -euo pipefail
: "${TC_DIR:?set TC_DIR to the exact Linx LLVM bin directory}"
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"
ROOT_DIR=$(git rev-parse --show-toplevel)
CXX="$TC_DIR/clang++"
read -r -a EXTRA_FLAGS <<< "${CC_OPTS:-}"
CASES="dtype maxabs_no_max rowmax_shape groupmax_shape lone_shared_a basic_f16_dtype tgemv_oversize tgemv_dtype tgemv_pp_oversize"
PASS=0; FAIL=0

TMP_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-negatives.XXXXXX")
trap 'rm -rf "$TMP_ROOT"' EXIT

compile_case() {
  local output=$1
  shift
  "$CXX" --target=linx64 -c -fenable-matrix -O2 \
    -nostdinc++ -isystem "$ROOT_DIR/test/linx_cxx_shim" \
    -std=c++20 -D__linx -DENABLE_TENSOR_INSTR -I../../include \
    "${EXTRA_FLAGS[@]}" "$@" src/PostProcessNegatives.cpp -o "$output"
}

if ! compile_case "$TMP_ROOT/positive.o" >"$TMP_ROOT/positive.log" 2>&1; then
  echo "FAIL: positive baseline did not compile"
  cat "$TMP_ROOT/positive.log"
  exit 1
fi

for c in $CASES; do
  case "$c" in
    dtype) expected="TMATMUL destination dtype does not match PreQuantMode" ;;
    maxabs_no_max) expected="FPATR config max_abs requires RowMax or GroupMax" ;;
    rowmax_shape) expected="TMATMUL RowMaxOut must have ValidCol=1" ;;
    groupmax_shape) expected="TMATMUL GroupMaxOut must have ValidCol=ceil(N/GroupN)" ;;
    lone_shared_a) expected="Shared matmul A requires B to be Shared as well" ;;
    basic_f16_dtype) expected="TMATMUL destination dtype does not match PreQuantMode" ;;
    tgemv_oversize) expected="TGEMV vector logical Tile size must be 128 B..8 KB" ;;
    tgemv_dtype) expected="TGEMV destination dtype does not match PreQuantMode" ;;
    tgemv_pp_oversize) expected="TGEMV postprocess auxiliary logical Tile size must be 128 B..8 KB" ;;
  esac
  log="$TMP_ROOT/$c.log"
  if compile_case "$TMP_ROOT/$c.o" "-DSHOULD_FAIL_$c" >"$log" 2>&1; then
    echo "FAIL (compiled but should not): $c"; FAIL=$((FAIL+1))
  elif ! grep -Fq "$expected" "$log"; then
    echo "FAIL (wrong diagnostic): $c"
    cat "$log"
    FAIL=$((FAIL+1))
  else
    echo "PASS (rejected): $c"; PASS=$((PASS+1))
  fi
done
echo "== $PASS passed, $FAIL failed =="
if [[ "$FAIL" -ne 0 ]]; then
  exit 1
fi
