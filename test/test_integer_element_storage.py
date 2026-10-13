#!/usr/bin/env python3
"""Independent raw-bit oracle for integer parent transport and column reversal."""
import argparse
from pathlib import Path
import struct
import unittest

from test_i64_element_tile_runtime import read_symbols

EDGES = (0, 0xffffffffffffffff, 0x8000000000000000,
         0x1122334455667788, 0xffffffff00000000,
         0x00000000ffffffff, 0x5566778811223344, 1)


def expected_symbols(count, bits):
    mask = (1 << bits) - 1
    values = [(EDGES[i % 8] ^ (0x0101010101010101 * (i // 8))) & mask
              for i in range(count)]
    columns = count // 32
    # Tensor-level oracle: reverse each row, independent of Tile assembly.
    reversed_values = [v for row in range(32)
                       for v in reversed(values[row * columns:(row + 1) * columns])]
    code = 'Q' if bits == 64 else 'I'
    result = {f'storage_{name}': struct.pack(f'<{count}{code}', *data)
              for name, data in [('input', values), ('roundtrip', values),
                                 ('reversed', reversed_values)]}
    for name in ('input', 'roundtrip', 'reversed'):
        result[f'storage_{name}_before'] = b'\x5a' * 64
        result[f'storage_{name}_after'] = b'\xa5' * 64
    return result


def resolve(symbols, count, bits):
    expected = expected_symbols(count, bits)
    for name, golden in expected.items():
        if name not in symbols or symbols[name][1] != len(golden):
            raise AssertionError(f'{name}: missing symbol or wrong capacity')
    for name in ('input', 'roundtrip', 'reversed'):
        body, before, after = (symbols[f'storage_{name}{suffix}']
                               for suffix in ('', '_before', '_after'))
        if before[0] + before[1] != body[0] or body[0] + body[1] != after[0]:
            raise AssertionError(f'{name}: guards are not adjacent')
    start = min(symbols[name][0] for name in expected)
    end = max(symbols[name][0] + len(data) for name, data in expected.items())
    return expected, start, end


class OracleTest(unittest.TestCase):
    def test_column_reversal_distinguishes_roundtrip(self):
        for bits in (32, 64):
            for count in (64, 128, 256, 512, 1024):
                expected = expected_symbols(count, bits)
                self.assertNotEqual(expected['storage_roundtrip'], expected['storage_reversed'])
                code = 'Q' if bits == 64 else 'I'
                original = struct.unpack(f'<{count}{code}', expected['storage_input'])
                reversed_values = struct.unpack(f'<{count}{code}', expected['storage_reversed'])
                for row in range(32):
                    for col in range(count // 32):
                        self.assertEqual(reversed_values[row * (count // 32) + col],
                                         original[(row + 1) * (count // 32) - 1 - col])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path)
    parser.add_argument('--llvm-objdump', type=Path)
    parser.add_argument('--count', type=int, choices=(64, 128, 256, 512, 1024))
    parser.add_argument('--bits', type=int, choices=(32, 64))
    parser.add_argument('--print-range', action='store_true')
    parser.add_argument('--dump', type=Path)
    parser.add_argument('--dump-base', type=lambda value: int(value, 0))
    args, remaining = parser.parse_known_args()
    if args.elf is None:
        unittest.main(argv=[__file__, *remaining])
        return
    if args.llvm_objdump is None or args.count is None or args.bits is None:
        parser.error('ELF validation requires --llvm-objdump, --count and --bits')
    symbols = read_symbols(args.llvm_objdump, args.elf)
    expected, start, end = resolve(symbols, args.count, args.bits)
    if args.print_range:
        print(f'{start:#x}:{end-start:#x}')
        return
    if args.dump is None or args.dump_base != start:
        parser.error('comparison requires --dump and the exact --dump-base')
    memory = args.dump.read_bytes()
    if len(memory) != end - start:
        raise AssertionError('incomplete or oversized memory dump')
    for name, golden in expected.items():
        offset = symbols[name][0] - start
        observed = memory[offset:offset + len(golden)]
        if observed != golden:
            first = next(i for i, (a, b) in enumerate(zip(observed, golden)) if a != b)
            raise AssertionError(f'{name}: first differing byte {first}')
    print(f'PASS: integer{args.bits} parent{args.count}, roundtrip/reversal/guards')


if __name__ == '__main__':
    main()
