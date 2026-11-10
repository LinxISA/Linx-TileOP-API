#!/usr/bin/env python3
"""Row-expansion offset spelling, native object encoding and legality checks."""
import ast
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / 'test/tileop_api/src'
CXX = os.environ.get('LINX_CXX')
if not CXX and os.environ.get('COMPILER_DIR'):
    CXX = str(Path(os.environ['COMPILER_DIR']) / 'clang++')
TOOL_DIR = Path(CXX).parent if CXX else None
MC = os.environ.get('LINX_MC') or (str(TOOL_DIR / 'llvm-mc') if TOOL_DIR else None)


def datr_word(layout, offset):
    # B.DATR L32: DTYPE_NONE=31, Null=3, CMode/Sat/Canonicalize=0.
    return 0x00001023 | (layout << 7) | (31 << 20) | (3 << 27) | (offset << 15)


class FieldEncoding(unittest.TestCase):
    @unittest.skipUnless(MC, 'set LINX_MC, LINX_CXX or COMPILER_DIR for MC checks')
    def test_all_field_values_in_both_layouts(self):
        # Expand the production macro with a host preprocessor; assemble that
        # exact output rather than maintaining a second spelling table here.
        for layout, code in (('CUBE_M32', 29), ('CUBE_M16', 31)):
            for offset in range(8):
                with self.subTest(layout=layout, offset=offset):
                    source = ('#include <common/pto_row_expand_asm.hpp>\n'
                              'const char *assembly = '
                              f'PTO_ROW_EXPAND_CUBE_DATR_ASM("{layout}", "{offset}");\n')
                    preprocessed = subprocess.run([
                        os.environ.get('CXX', 'c++'), '-E', '-P', '-x', 'c++',
                        '-I', str(ROOT / 'include'), '-',
                    ], input=source, text=True, capture_output=True)
                    self.assertEqual(preprocessed.returncode, 0, preprocessed.stderr)
                    assembly = ''.join(ast.literal_eval(s) for s in re.findall(
                        r'"(?:[^"\\]|\\.)*"', preprocessed.stdout))
                    result = subprocess.run([
                        MC, '-triple=linx64v5', '-show-encoding',
                    ], input=assembly, text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    encodings = re.findall(r'encoding: \[([^]]+)\]', result.stdout)
                    self.assertEqual(len(encodings), 1, result.stdout)
                    encoded = bytes(int(b.strip(), 16) for b in encodings[0].split(','))
                    self.assertEqual(int.from_bytes(encoded, 'little'),
                                     datr_word(code, offset), result.stdout)


class NativeCompilation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not CXX:
            raise unittest.SkipTest('set LINX_CXX or COMPILER_DIR for native checks')
        cls.work = tempfile.TemporaryDirectory(prefix='issue314-native-')
        cls.addClassCleanup(cls.work.cleanup)
        cls.directory = Path(cls.work.name)
        resource = Path(subprocess.check_output(
            [CXX, '-print-resource-dir'], text=True).strip())
        overlay = cls.directory / 'resource'
        # Linx Clang searches builtin TileOP first. Always test candidate headers.
        shutil.copytree(resource, overlay, ignore=lambda directory, names:
                        ['tileop-api'] if Path(directory) == resource / 'include' else [])
        shutil.copytree(ROOT / 'include', overlay / 'include/tileop-api')
        cls.flags = [CXX, '--target=' + os.environ.get(
            'LINX_TARGET', 'linx64v5-unknown-linux-musl'),
            '-resource-dir', str(overlay), '-std=c++20', '-O2', '-mlxbc',
            '-fenable-matrix', '-D__linx', '-DENABLE_TENSOR_INSTR']
        if os.environ.get('LINX_SYSROOT'):
            sysroot = Path(os.environ['LINX_SYSROOT'])
            cls.flags += ['--sysroot=' + str(sysroot), '-nostdinc++',
                          '-isystem', str(sysroot / 'usr/include/c++/v1')]
        cls.objdump = os.environ.get('LINX_OBJDUMP', str(TOOL_DIR / 'llvm-objdump'))

    def compile(self, *args):
        return subprocess.run(self.flags + list(args), text=True, capture_output=True)

    def test_objects_and_broadcast_offset_encoding(self):
        output = self.directory / 'offset.o'
        result = self.compile('-c', str(SOURCES / 'Issue314RowExpandOffset.cpp'),
                              '-o', str(output))
        self.assertEqual(result.returncode, 0, result.stderr)
        dump = subprocess.run([self.objdump, '-d', '-M', 'no-tile-macros', str(output)],
                              text=True, capture_output=True)
        self.assertEqual(dump.returncode, 0, dump.stderr)
        functions = dict(re.findall(r'^[0-9a-f]+ <(\w+)>:\n(.*?)(?=\nDisassembly|\Z)',
                                    dump.stdout, re.M | re.S))
        cases = {'m32_bf16_0': (29, 0), 'm32_bf16_2': (29, 2),
                 'm16_bf16_0': (31, 0), 'm16_bf16_2': (31, 2),
                 'm16_bf16_4': (31, 4), 'm16_bf16_6': (31, 6),
                 'm32_u8_3': (29, 3), 'm16_u8_7': (31, 7)}
        for name, (layout, offset) in cases.items():
            with self.subTest(function=name):
                body = functions[name]
                # One unary broadcast, then seven binary ops with four source
                # combinations each (plain, dual view, matrix view, carrier view).
                ops = re.findall(r'BSTART\.TEPL\s+(TROWEXPAND\w*),', body)
                expected = ['TROWEXPAND']
                for suffix in ('ADD', 'SUB', 'MUL', 'DIV', 'MAX', 'MIN', 'EXPDIF'):
                    expected += ['TROWEXPAND' + suffix] * 4
                self.assertEqual(ops, expected, body)
                words = re.findall(
                    r'^\s*[0-9a-f]+:\s+([0-9a-f]{8})\s+B\.DATR\s+CUBE_M\d+',
                    body, re.M)
                self.assertEqual(len(words), 29, body)
                for word in words:
                    self.assertEqual(int(word, 16), datr_word(layout, offset), body)
                self.assertEqual(body.count('B.SUBVIEW'), 28, body)

    def test_existing_row_expand_objects(self):
        for name in ('Issue253RowExpandDualSubview', 'TRowExpandMul'):
            with self.subTest(fixture=name):
                output = self.directory / (name + '.o')
                result = self.compile('-c', str(SOURCES / (name + '.cpp')),
                                      '-o', str(output))
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertGreater(output.stat().st_size, 0)

    def test_invalid_offsets_are_rejected_on_every_path(self):
        for path in range(4):
            for offset, rowmajor, diagnostic in (
                (-1, False, 'must fit B.DATR.RMode'),
                (8, False, 'must fit B.DATR.RMode'),
                (1, False, 'must align to element bytes'),
                (4, False, 'exceeds CELL columns'),
                (2, True, 'not valid for RowMajor'),
            ):
                with self.subTest(path=path, offset=offset, rowmajor=rowmajor):
                    args = ['-fsyntax-only', '-ferror-limit=1',
                            f'-DEXPAND_PATH={path}', f'-DBAD_OFFSET={offset}']
                    if rowmajor:
                        args += ['-DROWMAJOR']
                    result = self.compile(*args, str(
                        SOURCES / 'Issue314RowExpandOffsetNegatives.cpp'))
                    self.assertNotEqual(result.returncode, 0, result.stderr)
                    self.assertIn(diagnostic, result.stderr)


if __name__ == '__main__':
    unittest.main()
