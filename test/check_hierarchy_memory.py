#!/usr/bin/env python3
"""Verify the public hierarchy fixture against independent raw-bit snapshots."""
import argparse
from pathlib import Path
import re
import struct


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--symbols', required=True, type=Path)
    parser.add_argument('--dump', type=Path)
    parser.add_argument('--dump-base', type=lambda value: int(value, 0))
    parser.add_argument('--print-range', action='store_true')
    args = parser.parse_args()
    symbols = {}
    for line in args.symbols.read_text().splitlines():
        fields = line.split()
        if len(fields) >= 5 and re.fullmatch(r'[0-9a-fA-F]+', fields[0]):
            if fields[-1].startswith('hierarchy_f'):
                symbols[fields[-1]] = (int(fields[0], 16), int(fields[-2], 16))
    expected = {}
    for dtype, count, factor, seed, code in (('f16', 2048, 40503, 0x35a7, 'H'), ('f32', 16384, 104729, 0x80003039, 'I')):
        mask = (1 << (8 * struct.calcsize(code))) - 1
        data = [(i * factor + seed) & mask for i in range(count)]
        expected[f'hierarchy_{dtype}_output'] = struct.pack(f'<{count}{code}', *data)
        expected[f'hierarchy_{dtype}_input'] = struct.pack(f'<{count + 1}{code}', data[0], *data)
    for name, data in expected.items():
        if name not in symbols or symbols[name][1] != len(data):
            raise ValueError(f'{name}: missing symbol or changed allocation')
    begin = min(symbols[name][0] for name in expected)
    end = max(symbols[name][0] + len(data) for name, data in expected.items())
    if args.print_range:
        print(f'{begin:#x}:{end - begin:#x}')
        return
    if args.dump is None or args.dump_base is None:
        parser.error('--dump and --dump-base required for comparison')
    observed = args.dump.read_bytes()
    if args.dump_base != begin or len(observed) != end - begin:
        raise ValueError('memory dump must match the exact observation range')
    for name, data in expected.items():
        offset = symbols[name][0] - begin
        actual = observed[offset:offset + len(data)]
        if actual != data:
            first = next(i for i, (a, b) in enumerate(zip(actual, data)) if a != b)
            raise ValueError(f'{name}: first differing byte {first}')
        print(f'{name}: {len(data)} bytes PASS')


if __name__ == '__main__':
    main()
