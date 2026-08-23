#!/usr/bin/env bash
# Negative-test driver: compiles PostProcessNegatives.cpp once per
# -DSHOULD_FAIL_<case> and asserts the compile FAILS (the static_assert fires).
# Usage: TC=<toolchain-bin> ./run_negatives.sh
set -euo pipefail
TC_DIR=${TC_DIR:-/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/bin}
LINX_TARGET=${LINX_TARGET:-linx64-unknown-linux-musl}
CXX="$TC_DIR/clang++"
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-negatives.XXXXXX")
trap 'rm -rf "$OUT"' EXIT
FLAGS=(--target="$LINX_TARGET" -c -fenable-matrix -O2 -std=c++20 -D__linx
       -DENABLE_TENSOR_INSTR -I../../include)
if [[ -n "${LINX_SYSROOT:-}" ]]; then
  FLAGS+=(--sysroot="$LINX_SYSROOT" -nostdinc++
          -isystem "$LINX_SYSROOT/include/c++/v1")
fi
CASES="dtype maxabs_no_max rowmax_shape groupmax_shape lone_shared_a local_transpose old_rowmajor mismatched_m_layout local_k shared_cube_layout gemv_rows mixed_numeric_class unsigned_prequant bad_d_valid_shape bad_acc_dtype bad_bias_dtype bad_mx_scale_dtype bad_mx_scale_shape missing_mx_scale_a missing_mx_scale_b extra_mx_scale_a extra_mx_scale_b bad_transpose_d group_shape group_k group_n group_dynamic"
TS_CASES="dtype_full dtype_part layout_full layout_part mask0 mask16 mask3 size_small size_large"
PASS=0; FAIL=0

# A negative suite is meaningless when every compile is rejected before the
# intended static_assert. Prove the matching compiler can consume one positive
# TileOP translation unit before counting any negative result.
for source in TStoreShared.cpp PostProcessNegatives.cpp TStoreSharedNegatives.cpp; do
  stem=${source%.cpp}
  if ! "$CXX" "${FLAGS[@]}" "src/$source" -o "$OUT/$stem.o" \
      >"$OUT/$stem.stdout" 2>"$OUT/$stem.stderr"; then
    echo "BLOCKED: compiler cannot compile positive preflight $source" >&2
    sed -n '1,20p' "$OUT/$stem.stderr" >&2
    exit 2
  fi
done

for c in $CASES; do
  if "$CXX" "${FLAGS[@]}" -DSHOULD_FAIL_$c src/PostProcessNegatives.cpp \
       -o "$OUT/neg_$c.o" >/dev/null 2>&1; then
    echo "FAIL (compiled but should not): $c"; FAIL=$((FAIL+1))
  else
    echo "PASS (rejected): $c"; PASS=$((PASS+1))
  fi
done
for c in $TS_CASES; do
  if "$CXX" "${FLAGS[@]}" -DSHOULD_FAIL_$c src/TStoreSharedNegatives.cpp \
       -o "$OUT/neg_ts_$c.o" >/dev/null 2>&1; then
    echo "FAIL (compiled but should not): $c"; FAIL=$((FAIL+1))
  else
    echo "PASS (rejected): $c"; PASS=$((PASS+1))
  fi
done
echo "== $PASS passed, $FAIL failed =="
test "$FAIL" -eq 0
