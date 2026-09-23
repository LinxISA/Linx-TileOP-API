#!/usr/bin/env bash
set -euo pipefail

OBJ=${1:?usage: verify_shared_last_use.sh <SharedLastUse.o>}
TC_DIR=${TC_DIR:?set TC_DIR to the LLVM bin directory with B.IOS %K support}
OBJDUMP=${OBJDUMP:-$TC_DIR/llvm-objdump}

DISASSEMBLY=$($OBJDUMP -d "$OBJ")
RETAIN_COUNT=$(grep -cE 'S[0-9]+\.reuse' <<<"$DISASSEMBLY" || true)
KILL_COUNT=$(grep -cE 'B\.IOS[[:space:]]+S[0-9]+,[[:space:]]+mask=1111' <<<"$DISASSEMBLY" || true)

if (( RETAIN_COUNT < 1 )); then
  echo "FAIL: ordinary TMATMUL did not retain its non-final Shared operand" >&2
  exit 1
fi
if (( KILL_COUNT < 1 )); then
  echo "FAIL: no last-use B.IOS instruction was disassembled" >&2
  exit 1
fi

# A standalone last-use B.IOS is a 32-bit instruction. Reuse/kill is bit 26,
# so its printed raw word must have 0x04000000 set. Retaining B.IOS operands
# may be folded into a TMATMUL bundle by llvm-objdump; their `.reuse` spelling
# above is the negative/non-final-use check.
if ! grep -qE '^[[:space:]]*[0-9a-f]+:[[:space:]]+0[4567][0-9a-f]{6}[[:space:]]+B\.IOS' <<<"$DISASSEMBLY"; then
  echo "FAIL: no last-use B.IOS raw word with bit26=1" >&2
  exit 1
fi

echo "PASS: $RETAIN_COUNT retaining and $KILL_COUNT last-use B.IOS instructions"
