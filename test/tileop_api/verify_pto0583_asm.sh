#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
MC="$TC_DIR/llvm-mc"
OBJDUMP="$TC_DIR/llvm-objdump"
MC_TRIPLE=${MC_TRIPLE:-linx64v5}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-pto0583-mc.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

"$MC" -triple="$MC_TRIPLE" -filetype=obj \
  "$ROOT/test/tileop_api/pto0583_asm_contract.s" -o "$OUT/contract.o"
"$OBJDUMP" -d "$OUT/contract.o" >"$OUT/contract.diss"

require_disassembly() {
  local pattern=$1
  local description=$2
  if ! grep -Eq "$pattern" "$OUT/contract.diss"; then
    echo "FAIL: missing canonical $description in PTO 0.58.4 disassembly" >&2
    sed -n '1,120p' "$OUT/contract.diss" >&2
    exit 1
  fi
}

require_disassembly 'B\.FPATR[[:space:]]+0, 0, 0, 0, 0, 0, 0, 0, 0, 0' \
  'ten-field B.FPATR'
require_disassembly 'B\.IOT[[:space:]]+t#1, mask=1100, last,.*->m<64KB>' \
  'Local SizeCode=10 form'
require_disassembly 'B\.IOS[[:space:]]+mask=1111, ->S63<256KB>' \
  'Shared SizeCode=12 form'
require_disassembly 'B\.DATR[[:space:]]+ND2M32\.normal, Zero' \
  'ND2M32 CUBE load layout'
require_disassembly 'B\.DATR[[:space:]]+N82ND\.normal, Null' \
  'N82ND CUBE store layout'
require_disassembly 'B\.SUBVIEW[[:space:]]+0, a0, 0, 1' \
  'B.SUBVIEW range modifier'
require_disassembly 'B\.SUBVIEW[[:space:]]+1, x3, 2047, 12' \
  'Shared-source B.SUBVIEW range modifier'
require_disassembly 'B\.ASSEMBLE[[:space:]]+1, 0, a0, 100, 12' \
  'B.ASSEMBLE range modifier'
require_disassembly 'B\.ASSEMBLE[[:space:]]+1, 0, zero, 100, 12' \
  'Shared-destination B.ASSEMBLE range modifier'
require_disassembly 'B\.IOT[[:space:]]+t#3, mask=1111, last,.*->t<1KB>' \
  'present conditional MX scale binder'
if grep -Eq 'B\.IOT[[:space:]]+t#2, mask=1111' "$OUT/contract.diss"; then
  echo "FAIL: absent conditional MX scale binder survived assembly" >&2
  sed -n '1,120p' "$OUT/contract.diss" >&2
  exit 1
fi

for invalid in \
  'B.IOT t#1, mask=0011, last, ->m<128B>' \
  'B.IOT t#1, mask=1111, last, ->m<512KB>' \
  'B.FPATR 0, 0, 0, 0, 0, 0, 0, 1, 1'; do
  printf '.text\n%s\n' "$invalid" >"$OUT/invalid.s"
  if "$MC" -triple="$MC_TRIPLE" -filetype=obj "$OUT/invalid.s" \
      -o "$OUT/invalid.o" \
      >"$OUT/invalid.stdout" 2>"$OUT/invalid.stderr"; then
    echo "invalid form assembled successfully: $invalid" >&2
    exit 1
  fi
done

echo "PTO ISA 0.58.4 MC/disassembly/negative contract: PASS"
