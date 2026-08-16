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
CASES="dtype maxabs_no_max rowmax_shape groupmax_shape lone_shared_a"
PASS=0; FAIL=0
for c in $CASES; do
  if "$CXX" --target=linx64 -c -fenable-matrix -O2 \
       -nostdinc++ -isystem "$ROOT_DIR/test/linx_cxx_shim" \
       -std=c++20 -D__linx -DENABLE_TENSOR_INSTR -I../../include \
       "${EXTRA_FLAGS[@]}" \
       -DSHOULD_FAIL_$c src/PostProcessNegatives.cpp -o /tmp/neg_$c.o >/dev/null 2>&1; then
    echo "FAIL (compiled but should not): $c"; FAIL=$((FAIL+1))
  else
    echo "PASS (rejected): $c"; PASS=$((PASS+1))
  fi
done
echo "== $PASS passed, $FAIL failed =="
