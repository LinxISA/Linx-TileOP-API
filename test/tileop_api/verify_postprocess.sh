#!/usr/bin/env bash
# P0-8: verify Matrix inline-asm bundles encode correctly and emit no
# .FIXP mnemonic. Run from the tileop_api test dir with $1 = object file.
set -euo pipefail
OBJ="${1:?usage: $0 <matrix .o>}"
TC_DIR=${TC_DIR:-/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/bin}
OBJDUMP="$TC_DIR/llvm-objdump"

echo "== Matrix ops present =="
"$OBJDUMP" -d "$OBJ" | grep -oE "BSTART\.CUBE\s+[A-Z.]+" | sort | uniq -c

echo "== FPATR lines (must carry the RowMax/GroupMax config) =="
"$OBJDUMP" -d "$OBJ" | grep "B.FPATR" | head -6

echo "== no .FIXP mnemonic anywhere =="
if "$OBJDUMP" -d "$OBJ" | grep -qE "TMATMUL[A-Z.]*\.FIXP|TGEMV[A-Z.]*\.FIXP|\.FIXP"; then
  echo "FAIL: .FIXP mnemonic found"; exit 1
fi
echo "PASS"
