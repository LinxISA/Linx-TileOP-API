#!/usr/bin/env python3
"""Independent raw-memory oracle for the public TCOLSUM contract fixture."""

import argparse
from pathlib import Path
import struct
import subprocess
import unittest


def source_values():
    return [100 * row + col + 1 for row in range(4) for col in range(8)]


def output_values():
    values = [0xDEADBEEF] * 32
    for col in range(7):
        values[col] = sum(100 * row + col + 1 for row in range(3))
    return values


def expected_symbols():
    return {
        "tcolsum_input_before": b"\x5a" * 64,
        "tcolsum_input": struct.pack("<32I", *source_values()),
        "tcolsum_input_after": b"\xa5" * 64,
        "tcolsum_output_before": b"\x5a" * 64,
        "tcolsum_output": struct.pack("<32I", *output_values()),
        "tcolsum_output_after": b"\xa5" * 64,
    }


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
    for stem in ("input", "output"):
        before = symbols[f"tcolsum_{stem}_before"]
        body = symbols[f"tcolsum_{stem}"]
        after = symbols[f"tcolsum_{stem}_after"]
        if before[0] + before[1] != body[0]:
            raise AssertionError(f"{stem}: before guard is not adjacent")
        if body[0] + body[1] != after[0]:
            raise AssertionError(f"{stem}: after guard is not adjacent")


class TColSumRuntimeTest(unittest.TestCase):
    def test_reference_uses_source_valid_rows(self):
        self.assertEqual(output_values()[:7],
                         [303, 306, 309, 312, 315, 318, 321])
        self.assertEqual(output_values()[7:], [0xDEADBEEF] * 25)


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
        parser.error(f"ELF is missing TCOLSUM symbols: {missing}")
    validate_guard_adjacency(symbols)
    start = min(symbols[name][0] for name in expected)
    end = max(symbols[name][0] + symbols[name][1] for name in expected)
    if args.print_range:
        print(f"0x{start:x}:0x{end - start:x}")
        return
    if args.dump is None or args.dump_base is None:
        parser.error("comparison requires --dump and --dump-base")
    memory = args.dump.read_bytes()
    if len(memory) != end - start:
        raise AssertionError("incomplete or oversized memory dump")
    for name, golden in expected.items():
        address, size = symbols[name]
        if size != len(golden):
            raise AssertionError(
                f"{name}: ELF size {size} != golden size {len(golden)}")
        offset = address - args.dump_base
        if memory[offset:offset + size] != golden:
            raise AssertionError(f"{name}: raw memory differs from golden")
    print("PASS: public TCOLSUM uses source ValidRow and preserves guards")


if __name__ == "__main__":
    main()
