#!/usr/bin/env python3
"""Raw BF16x2 encoding tests; no numerical conversion or model claims."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PackedBits(unittest.TestCase):
    def test_raw_encodings_with_both_carriers(self):
        for carrier in ('integer', 'float'):
            for opt in ('-O0', '-O2', '-O3'):
                with self.subTest(carrier=carrier, optimization=opt):
                    with tempfile.TemporaryDirectory(prefix='packed-bits-') as tmp:
                        command = [os.environ.get('CXX', 'c++'), '-std=c++17',
                                   opt, '-Wall', '-Wextra', '-Werror',
                                   '-I' + str(ROOT / 'include'),
                                   str(ROOT / 'test/test_packed_bits.cpp'),
                                   '-o', str(Path(tmp) / 'test')]
                        if carrier == 'float':
                            command.append('-DPTO_TEST_FLOAT_BF16X2')
                        subprocess.run(command, check=True, capture_output=True,
                                       text=True)
                        subprocess.run([str(Path(tmp) / 'test')], check=True)

    def test_invalid_lane_is_rejected(self):
        source = ('#include "common/pto_packed_bits.hpp"\n'
                  'void invalid(__bf16x2 value) { '
                  '(void)pto::bf16x2_lane_bits<2>(value); }\n')
        result = subprocess.run(
            [os.environ.get('CXX', 'c++'), '-std=c++17', '-fsyntax-only',
             '-I' + str(ROOT / 'include'), '-x', 'c++', '-'],
            input=source, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('BF16x2 lane must be 0 or 1', result.stderr)


if __name__ == '__main__':
    unittest.main()
