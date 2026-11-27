"""Indexed GM displacements use 32/64-bit integer lanes, including masks/ranges."""
import unittest

import test_llvm138_narrow_tcmp as native


class IndexedTLSUOffsetTypes(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def source(self, dtype, operation, bounded):
        args = {
            'MGATHER': 'data, gm, offsets',
            'MSCATTER': 'gm, data, offsets',
            'MGATHER_MASK': 'data, gm, offsets, mask',
            'MSCATTER_MASK': 'gm, data, offsets, mask',
        }[operation]
        if bounded:
            args += ', IndexedByteRange{0, 4096}'
        return f'''#include <common/pto_tileop.hpp>
using namespace pto;
using Data = Tile<Location::Vec, float, 1, 128, BLayout::RowMajor>;
using Offset = Tile<Location::Vec, {dtype}, 1, 128, BLayout::RowMajor>;
using Mask = Tile<Location::Vec, uint8_t, 1, 128, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<1, 128>>;
void check(Data &data, Offset &offsets, Mask &mask, GM &gm) {{
  {operation}({args});
}}
'''

    def test_legal_integer_offsets(self):
        # Every public ordinary/masked overload, with and without a range.
        for dtype in ('int32_t', 'uint32_t', 'int64_t', 'uint64_t'):
            source = self.output / (dtype + '.cpp')
            variants = [(op, bounded)
                        for op in ('MGATHER', 'MSCATTER', 'MGATHER_MASK', 'MSCATTER_MASK')
                        for bounded in (False, True)]
            functions = []
            for i, (op, bounded) in enumerate(variants):
                text = self.source(dtype, op, bounded)
                functions.append(text[text.index('void check('):].replace(
                    'void check(', f'void check_{i}('))
            prefix = self.source(dtype, 'MSCATTER', False).split('void check(')[0]
            source.write_text(prefix + '\n'.join(functions))
            result = self.compile('-fsyntax-only', str(source))
            self.assertEqual(result.returncode, 0, result.stdout)
            result = self.compile('-c', str(source), '-o', str(self.output / (dtype + '.o')))
            self.assertEqual(result.returncode, 0, result.stdout)

    def test_reject_narrow_and_floating_offsets(self):
        # The concat regression used U16; equal-width floating offsets are
        # invalid too. Confirm each overload diagnoses before code generation.
        for op in ('MGATHER', 'MSCATTER', 'MGATHER_MASK', 'MSCATTER_MASK'):
            for bounded in (False, True):
                with self.subTest(operation=op, range=bounded):
                    source = self.output / 'invalid.cpp'
                    source.write_text(self.source('uint16_t', op, bounded))
                    result = self.compile('-fsyntax-only', str(source))
                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn('indexed TLSU byte offsets must use S32, U32, S64, or U64', result.stdout)
        for dtype in ('int16_t', 'uint8_t', 'float'):
            source = self.output / 'invalid.cpp'
            source.write_text(self.source(dtype, 'MSCATTER', False))
            result = self.compile('-fsyntax-only', str(source))
            self.assertNotEqual(result.returncode, 0, result.stdout)
            self.assertIn('indexed TLSU byte offsets must use S32, U32, S64, or U64', result.stdout)


if __name__ == '__main__':
    unittest.main()
