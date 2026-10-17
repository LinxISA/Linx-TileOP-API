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

# Every named TLSU/CUBE start emitted by this header must be accepted by the
# same LLVM MC catalog. VEC/SFU wrappers intentionally retain the raw TEPL
# carrier; their architectural alias inventory is checked in LinxISA/LLVM.
python3 - "$ROOT/contracts/linxisa-v0.58-engine-ops.json" \
  "$OUT/named-engine-starts.s" <<'PY'
import json
import sys
from pathlib import Path

contract = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
lines = [".text"]
for key in ("tlsu_ops", "cube_ops"):
    lines.extend(f"{row['mnemonic']} FP16" for row in contract[key])
Path(sys.argv[2]).write_text("\n".join(lines) + "\n", encoding="utf-8")
PY
"$MC" -triple="$MC_TRIPLE" -filetype=obj \
  "$OUT/named-engine-starts.s" -o "$OUT/named-engine-starts.o"
"$OBJDUMP" -d "$OUT/named-engine-starts.o" >"$OUT/named-engine-starts.diss"
if [[ $(grep -c 'BSTART\.' "$OUT/named-engine-starts.diss") -ne 40 ]]; then
  echo 'FAIL: not all 40 golden TLSU/CUBE named starts survived MC round-trip' >&2
  sed -n '1,160p' "$OUT/named-engine-starts.diss" >&2
  exit 1
fi

require_disassembly() {
  local pattern=$1
  local description=$2
  if ! grep -Eq "$pattern" "$OUT/contract.diss"; then
    echo "FAIL: missing canonical $description in PTO 0.58.7 disassembly" >&2
    sed -n '1,120p' "$OUT/contract.diss" >&2
    exit 1
  fi
}

require_disassembly 'B\.FPATR[[:space:]]+0, 0, 0, 0, 0, 0, 0, 0, 0, 0' \
  'ten-field B.FPATR'
require_disassembly 'B\.IOT[[:space:]]+t#1\.reuse, mask=1100, last,.*->m<64KB>' \
  'Local SizeCode=10 form'
require_disassembly 'B\.IOS[[:space:]]+S1\.reuse, mask=1111' \
  'Shared retained-source form'
require_disassembly 'B\.IOT[[:space:]]+t#1\.reuse, u#2\.reuse, mask=1111, last' \
  'two-source retained Local form'
require_disassembly 'B\.IOS[[:space:]]+mask=1111, ->S63<256KB>' \
  'Shared SizeCode=12 form'
require_disassembly 'B\.DATR[[:space:]]+ND2M32, DTYPE_NONE, Zero' \
  'ND2M32 CUBE load layout'
require_disassembly 'B\.DATR[[:space:]]+N82ND, DTYPE_NONE, Null' \
  'N82ND CUBE store layout'
require_disassembly 'B\.SUBVIEW[[:space:]]+0, a0, 0, 1' \
  'B.SUBVIEW range modifier'
if ! awk '
  /B\.IOT[[:space:]]+t#4\.reuse, mask=1111, last/ { binder=NR; next }
  /B\.SUBVIEW[[:space:]]+0, a0, 0, 1/ { if (NR == binder + 1) adjacent=1 }
  END { exit adjacent ? 0 : 1 }
' "$OUT/contract.diss"; then
  echo 'FAIL: B.SUBVIEW is not immediately adjacent to its Local B.IOT binder' >&2
  sed -n '1,120p' "$OUT/contract.diss" >&2
  exit 1
fi
require_disassembly 'B\.ASSEMBLE[[:space:]]+1, 0, a0, 100, 12' \
  'B.ASSEMBLE range modifier'
require_disassembly 'B\.ASSEMBLE[[:space:]]+1, 0, zero, 100, 12' \
  'Shared-destination B.ASSEMBLE range modifier'
require_disassembly 'B\.IOT[[:space:]]+t#3\.reuse, mask=1111, last,.*->t<1KB>' \
  'present conditional MX scale binder'
if grep -Eq 'B\.IOT[[:space:]]+t#2\.reuse, mask=1111' "$OUT/contract.diss"; then
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

echo "PTO ISA 0.58.7 MC/disassembly/negative contract: PASS"
