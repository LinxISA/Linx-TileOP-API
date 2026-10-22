#!/usr/bin/env python3
"""Raw-memory oracle for the native S64/U64 ElementTile leaf fixture."""

import argparse
from pathlib import Path
import struct
import subprocess
import unittest


EDGE = (
    0, 0x8000000000000000, 0xFFFFFFFFFFFFFFFF,
    0x1122334455667788, 0x5566778811223344,
    0xFFFFFFFF00000000, 0x00000000FFFFFFFF, 1,
)


def values():
    return [EDGE[index % 8] ^
            (0x0101010101010101 * (index // 8))
            for index in range(32)]


def expected_symbols():
    raw = values()
    packed = struct.pack("<32Q", *raw)
    expected = {
        "element_i64_u64_input": packed,
        "element_i64_u64_output": packed,
        "element_i64_s64_input": packed,
        "element_i64_s64_output": packed,
        "element_i64_expected_low":
            struct.pack("<32I", *(value & 0xFFFFFFFF for value in raw)),
        "element_i64_expected_high":
            struct.pack("<32I", *(value >> 32 for value in raw)),
    }
    for stem in ("u64_input", "u64_output", "s64_input", "s64_output"):
        expected[f"element_i64_{stem}_guard_before"] = b"\x5a" * 4096
        expected[f"element_i64_{stem}_guard_after"] = b"\xa5" * 64
    return expected


def read_symbols(llvm_objdump, elf):
    completed = subprocess.run(
        [str(llvm_objdump), "-t", str(elf)], check=True,
        text=True, capture_output=True)
    result = {}
    for line in completed.stdout.splitlines():
        fields = line.split()
        if len(fields) >= 6 and fields[1] == "g" and fields[2] == "O":
            try:
                result[fields[5]] = (int(fields[0], 16), int(fields[4], 16))
            except ValueError:
                pass
    return result


def validate_guard_adjacency(symbols):
    for stem in ("u64_input", "u64_output", "s64_input", "s64_output"):
        before = symbols[f"element_i64_{stem}_guard_before"]
        body = symbols[f"element_i64_{stem}"]
        after = symbols[f"element_i64_{stem}_guard_after"]
        if before[0] + before[1] != body[0]:
            raise AssertionError(f"{stem}: before guard is not adjacent")
        if body[0] + body[1] != after[0]:
            raise AssertionError(f"{stem}: after guard is not adjacent")


class I64ElementTileRuntimeTest(unittest.TestCase):
    def test_reference_planes_are_independent(self):
        raw = values()
        low = [value & 0xFFFFFFFF for value in raw]
        high = [value >> 32 for value in raw]
        physical = [0] * 64
        for row in range(32):
            physical[((2 * 0) * 128 + row * 4) // 4] = low[row]
            physical[((2 * 0 + 1) * 128 + row * 4) // 4] = high[row]
        self.assertEqual(physical[:32], low)
        self.assertEqual(physical[32:], high)
        self.assertNotEqual(low, high)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--llvm-objdump", type=Path)
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--dump", type=Path)
    parser.add_argument("--dump-base", type=lambda value: int(value, 0))
    parser.add_argument("--print-range", action="store_true")
    args, remaining = parser.parse_known_args()
    if args.elf is None:
        unittest.main(argv=[__file__, *remaining])
        return
    if args.llvm_objdump is None:
        parser.error("--llvm-objdump is required with --elf")
    expected = expected_symbols()
    symbols = read_symbols(args.llvm_objdump, args.elf)
    missing = sorted(set(expected) - set(symbols))
    if missing:
        parser.error(f"ELF is missing native64 symbols: {missing}")
    validate_guard_adjacency(symbols)
    start = min(symbols[name][0] for name in expected)
    end = max(symbols[name][0] + symbols[name][1] for name in expected)
    if args.print_range:
        print(f"0x{start:x}:0x{end - start:x}")
        return
    if args.dump is None or args.dump_base is None:
        parser.error("comparison requires --dump and --dump-base")
    memory = args.dump.read_bytes()
    for name, golden in expected.items():
        address, size = symbols[name]
        if size != len(golden):
            raise AssertionError(
                f"{name}: ELF size {size} != golden size {len(golden)}")
        offset = address - args.dump_base
        observed = memory[offset:offset + size]
        if observed != golden:
            raise AssertionError(f"{name}: raw memory differs from golden")
    print("PASS: native64 ElementTile ND round-trip matches raw golden")


if __name__ == "__main__":
    main()
