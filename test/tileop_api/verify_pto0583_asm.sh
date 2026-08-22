#!/usr/bin/env bash
set -euo pipefail

TC_DIR=${TC_DIR:?set TC_DIR to the matching Linx LLVM bin directory}
CLANG="$TC_DIR/clang"
OBJDUMP="$TC_DIR/llvm-objdump"
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT=$(mktemp -d "${TMPDIR:-/tmp}/tileop-pto0583-mc.XXXXXX")
trap 'rm -rf "$OUT"' EXIT

"$CLANG" --target=linx64 -c \
  "$ROOT/test/tileop_api/pto0583_asm_contract.s" -o "$OUT/contract.o"
"$OBJDUMP" -d "$OUT/contract.o" >"$OUT/contract.diss"

grep -Eq 'B\.FPATR[[:space:]]+0, 0, 0, 0, 0, 0, 0, 1, 1' "$OUT/contract.diss"
grep -Eq 'B\.IOT[[:space:]]+t#1, mask=1100, last,.*->m<64KB>' "$OUT/contract.diss"
grep -Eq 'B\.IOS[[:space:]]+mask=1111, ->S255<256KB>' "$OUT/contract.diss"
grep -Eiq 'B\.DATR.*layout21' "$OUT/contract.diss"
grep -Eiq 'B\.DATR.*layout26' "$OUT/contract.diss"

for invalid in \
  'B.IOT t#1, mask=0011, last, ->m<128B>' \
  'B.IOT t#1, mask=1111, last, ->m<128KB>' \
  'B.FPATR 0, 0, 0, 0, 0, 0, 0'; do
  printf '.text\n%s\n' "$invalid" >"$OUT/invalid.s"
  if "$CLANG" --target=linx64 -c "$OUT/invalid.s" -o "$OUT/invalid.o" \
      >"$OUT/invalid.stdout" 2>"$OUT/invalid.stderr"; then
    echo "invalid form assembled successfully: $invalid" >&2
    exit 1
  fi
done

echo "PTO ISA 0.58.3 MC/disassembly/negative contract: PASS"
