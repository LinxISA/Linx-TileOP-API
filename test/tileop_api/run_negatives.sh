#!/usr/bin/env bash
# Negative-test driver: compiles each negative fixture once per
# -DSHOULD_FAIL_<case> and verifies that the intended rejection occurs.
# Usage: TC_DIR=<toolchain-bin> ./run_negatives.sh
set -euo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"
ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
TC_DIR=${TC_DIR:-/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/bin}
LINX_TARGET=${LINX_TARGET:-linx64v5-unknown-linux-musl}
CXX="$TC_DIR/clang++"
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-negatives.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

# The Linx driver provides tileop-api from its Clang resource directory before
# ordinary -I paths.  Copy that resource tree and replace only tileop-api so
# this suite cannot silently compile against stale installed headers.  A
# symlink-only overlay is insufficient because Clang resolves the resource
# directory before searching its builtin include subtree.
RESOURCE_DIR="$OUT/clang-resource"
cp -R "$("$CXX" -print-resource-dir)" "$RESOURCE_DIR"
rm -rf "$RESOURCE_DIR/include/tileop-api"
cp -R "$ROOT/include" "$RESOURCE_DIR/include/tileop-api"

FLAGS=(-resource-dir "$RESOURCE_DIR" --target="$LINX_TARGET" -fsyntax-only -mlxbc
       -fenable-matrix -O2 -std=c++20 -D__linx -DENABLE_TENSOR_INSTR)
if [[ -n "${LINX_SYSROOT:-}" ]]; then
  FLAGS+=(--sysroot="$LINX_SYSROOT" -nostdinc++
          -isystem "$LINX_SYSROOT/usr/include/c++/v1")
fi

# Include tracing is the sentinel: unlike a successful compile, this proves
# the driver's builtin lookup selected the copied checkout header subtree.
TRACE="$OUT/resource-include.trace"
if ! "$CXX" "${FLAGS[@]}" -H -fsyntax-only src/RangeSubview.cpp \
     -o /dev/null >/dev/null 2>"$TRACE" ||
   ! grep -Fq "$RESOURCE_DIR/include/tileop-api/common/pto_tileop.hpp" "$TRACE"; then
  echo "BLOCKED: compiler did not select checkout tileop-api headers" >&2
  sed -n '1,30p' "$TRACE" >&2
  exit 2
fi
CASES="dtype maxabs_no_max rowmax_shape groupmax_shape lone_shared_a local_transpose old_rowmajor mismatched_m_layout local_k shared_cube_layout gemv_rows mixed_numeric_class unsigned_prequant bad_d_valid_shape bad_acc_dtype bad_bias_dtype bad_mx_scale_dtype bad_mx_scale_shape missing_mx_scale_a missing_mx_scale_b extra_mx_scale_a extra_mx_scale_b hif4_ordinary_matmul hif4_missing_scale_a hif4_missing_scale_b hif4_scale_dtype hif4_scale_shape bad_transpose_d group_shape group_k group_n group_dynamic"
TS_CASES="dtype_full dtype_part layout_full layout_part mask0 mask16 mask3 size_large"
RANGE_CASES="subview_dest assemble_source subview_length"
SUBVIEW_LEGALITY_CASES="subview_rowmajor subview_vec_cube tpartview_rowmajor tpartview_shared tpartview_vec_cube"
GMOV_CASES="fp64 s64 u64 dtype shape valid_shape layout capacity location shared"
TCVT_CASES="cube_layout cube_valid_shape cube_n8 valid_shape"
PASS=0; FAIL=0

# A negative suite is meaningless when every compile is rejected before the
# intended static_assert. Prove the matching compiler can consume one positive
# TileOP translation unit before counting any negative result.
for source in TStoreShared.cpp PostProcessNegatives.cpp TStoreSharedNegatives.cpp CubeTCvt.cpp TCvtCubeNegatives.cpp GMov.cpp SharedRange.cpp TileArrayCube.cpp RangeSubview.cpp; do
  stem=${source%.cpp}
  if ! "$CXX" "${FLAGS[@]}" "src/$source" -o "$OUT/$stem.o" \
      >"$OUT/$stem.stdout" 2>"$OUT/$stem.stderr"; then
    echo "BLOCKED: compiler cannot compile positive preflight $source" >&2
    sed -n '1,20p' "$OUT/$stem.stderr" >&2
    exit 2
  fi
done

expect_rejected() {
  local label=$1 define=$2 source=$3 pattern=$4
  local stderr="$OUT/$label.stderr"
  if "$CXX" "${FLAGS[@]}" -D"$define" "src/$source" \
       -o "$OUT/$label.o" >/dev/null 2>"$stderr"; then
    echo "FAIL (compiled but should not): $label"
    FAIL=$((FAIL+1))
  elif grep -Eq "$pattern" "$stderr"; then
    echo "PASS (expected diagnostic): $label"
    PASS=$((PASS+1))
  else
    echo "FAIL (wrong diagnostic): $label"
    sed -n '1,30p' "$stderr"
    FAIL=$((FAIL+1))
  fi
}

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
for c in $RANGE_CASES; do
  if [[ "$c" == subview_dest ]]; then
    define=SHOULD_FAIL_SUBVIEW_DEST
    pattern='B\.SUBVIEW is source-only'
  elif [[ "$c" == assemble_source ]]; then
    define=SHOULD_FAIL_ASSEMBLE_SOURCE
    pattern='B\.ASSEMBLE is destination-only'
  else
    define=SHOULD_FAIL_SUBVIEW_LENGTH
    pattern='B\.SUBVIEW length cannot exceed the parent Tile capacity'
  fi
  expect_rejected "range_$c" "$define" RangeNegatives.cpp "$pattern"
done
for c in $SUBVIEW_LEGALITY_CASES; do
  define=SHOULD_FAIL_$(echo "$c" | tr '[:lower:]' '[:upper:]')
  expect_rejected "subview_$c" "$define" SubviewLegalityNegatives.cpp \
    'is_legal_subview_parent_v<.*evaluated to false'
done
for c in $GMOV_CASES; do
  define=SHOULD_FAIL_GMOV_$(echo "$c" | tr '[:lower:]' '[:upper:]')
  case "$c" in
    fp64|s64|u64) pattern='GMOV supports the 17 non-packed carrier types' ;;
    dtype) pattern='GMOV source and destination dtypes must match' ;;
    shape|valid_shape|layout)
      pattern='GMOV source and destination descriptors must match' ;;
    capacity) pattern='GMOV source and destination logical sizes must match' ;;
    location) pattern='GMOV source and destination must be Local Vec Tiles' ;;
    shared) pattern='does not satisfy .is_tile_data_v.' ;;
  esac
  expect_rejected "gmov_$c" "$define" GMovNegatives.cpp "$pattern"
done
for c in $TCVT_CASES; do
  case "$c" in
    cube_layout) define=SHOULD_FAIL_TCVT_CUBE_LAYOUT ;;
    cube_valid_shape) define=SHOULD_FAIL_TCVT_CUBE_VALID_SHAPE ;;
    cube_n8) define=SHOULD_FAIL_TCVT_CUBE_N8 ;;
    valid_shape) define=SHOULD_FAIL_TCVT_VALID_SHAPE ;;
  esac
  if "$CXX" "${FLAGS[@]}" -D"$define" src/TCvtCubeNegatives.cpp \
       -o "$OUT/neg_tcvt_$c.o" >/dev/null 2>&1; then
    echo "FAIL (compiled but should not): tcvt_$c"; FAIL=$((FAIL+1))
  else
    echo "PASS (rejected): tcvt_$c"; PASS=$((PASS+1))
  fi
done
echo "== $PASS passed, $FAIL failed =="
test "$FAIL" -eq 0
