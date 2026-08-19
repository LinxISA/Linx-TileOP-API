#!/usr/bin/env bash
# Negative-test driver: compiles PostProcessNegatives.cpp once per
# -DSHOULD_FAIL_<case> and asserts the compile FAILS (the static_assert fires).
# Usage: TC=<toolchain-bin> ./run_negatives.sh
set -u
TC_DIR=${TC_DIR:-/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/bin}
CXX="$TC_DIR/clang++"
CASES="dtype maxabs_no_max rowmax_shape groupmax_shape lone_shared_a group_shape group_k group_n group_dynamic"
PASS=0; FAIL=0
for c in $CASES; do
  if "$CXX" --target=linx64v5-unknown-linux-musl -c -mlxbc -fenable-matrix -O2 \
       -std=c++20 -D__linx -DENABLE_TENSOR_INSTR -I../../include \
       -DSHOULD_FAIL_$c src/PostProcessNegatives.cpp -o /tmp/neg_$c.o >/dev/null 2>&1; then
    echo "FAIL (compiled but should not): $c"; FAIL=$((FAIL+1))
  else
    echo "PASS (rejected): $c"; PASS=$((PASS+1))
  fi
done
echo "== $PASS passed, $FAIL failed =="
