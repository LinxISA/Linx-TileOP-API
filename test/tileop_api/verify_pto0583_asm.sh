#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
LINX_TARGET=${LINX_TARGET:-linx64-unknown-linux-musl}
CLANG="$TC_DIR/clang"
OBJDUMP="$TC_DIR/llvm-objdump"
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-pto0583-mc.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

"$CLANG" --target="$LINX_TARGET" -c \
  "$ROOT/test/tileop_api/pto0583_asm_contract.s" -o "$OUT/contract.o"
"$OBJDUMP" -d "$OUT/contract.o" >"$OUT/contract.diss"

require_disassembly() {
  local pattern=$1
  local description=$2
  if ! grep -Eq "$pattern" "$OUT/contract.diss"; then
    echo "FAIL: missing canonical $description in PTO 0.58.3 disassembly" >&2
    sed -n '1,120p' "$OUT/contract.diss" >&2
    exit 1
  fi
}

require_disassembly 'B\.FPATR[[:space:]]+0, 0, 0, 0, 0, 0, 0, 0, 0, 0' \
  'ten-field B.FPATR'
require_disassembly 'B\.IOT[[:space:]]+t#1, mask=1100, last,.*->m<64KB>' \
  'Local SizeCode=10 form'
require_disassembly 'B\.IOS[[:space:]]+mask=1111, ->S255<256KB>' \
  'Shared SizeCode=12 form'
require_disassembly 'B\.DATR[[:space:]]+ND2M32\.normal, Zero' \
  'ND2M32 CUBE load layout'
require_disassembly 'B\.DATR[[:space:]]+N82ND\.normal, Null' \
  'N82ND CUBE store layout'
require_disassembly 'B\.SUBVIEW[[:space:]]+0, a0, 0, 1' \
  'B.SUBVIEW range modifier'
require_disassembly 'B\.ASSEMBLE[[:space:]]+1, 0, a0, 100, 12' \
  'B.ASSEMBLE range modifier'
require_disassembly 'B\.IOT[[:space:]]+t#3, mask=1111, last,.*->t<1KB>' \
  'present conditional MX scale binder'
if grep -Eq 'B\.IOT[[:space:]]+t#2, mask=1111' "$OUT/contract.diss"; then
  echo "FAIL: absent conditional MX scale binder survived assembly" >&2
  sed -n '1,120p' "$OUT/contract.diss" >&2
  exit 1
fi

for invalid in \
  'B.IOT t#1, mask=0011, last, ->m<128B>' \
  'B.IOT t#1, mask=1111, last, ->m<128KB>' \
  'B.FPATR 0, 0, 0, 0, 0, 0, 0, 1, 1'; do
  printf '.text\n%s\n' "$invalid" >"$OUT/invalid.s"
  if "$CLANG" --target="$LINX_TARGET" -c "$OUT/invalid.s" -o "$OUT/invalid.o" \
      >"$OUT/invalid.stdout" 2>"$OUT/invalid.stderr"; then
    echo "invalid form assembled successfully: $invalid" >&2
    exit 1
  fi
done

echo "PTO ISA 0.58.3 MC/disassembly/negative contract: PASS"
