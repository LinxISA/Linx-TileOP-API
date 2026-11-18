#!/usr/bin/env python3
"""Local TIMG2COL call-boundary regression; no numerical/model claim."""
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'test/tileop_api/src/Issue323LocalTimg2colInline.cpp'
CASES = ('m16_params_empty', 'm16_words_empty', 'm16_group_empty',
         'm16_group_full', 'm16_group_tail', 'm32_group_empty', 'm32_group_full')


class NativeCompilation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = os.environ.get('LINX_CXX')
        if not compiler and os.environ.get('COMPILER_DIR'):
            compiler = str(Path(os.environ['COMPILER_DIR']) / 'clang++')
        if not compiler:
            raise unittest.SkipTest('set LINX_CXX or COMPILER_DIR for native checks')
        cls.work = tempfile.TemporaryDirectory(prefix='issue323-native-')
        cls.addClassCleanup(cls.work.cleanup)
        cls.directory = Path(cls.work.name)
        resource = Path(subprocess.check_output(
            [compiler, '-print-resource-dir'], text=True).strip())
        overlay = cls.directory / 'resource'
        # Linx Clang searches resource TileOP before -I. Test candidate headers.
        shutil.copytree(resource, overlay, ignore=lambda directory, names:
                        ['tileop-api'] if Path(directory) == resource / 'include' else [])
        shutil.copytree(ROOT / 'include', overlay / 'include/tileop-api')
        cls.flags = [compiler, '--target=' + os.environ.get(
            'LINX_TARGET', 'linx64v5-unknown-linux-musl'),
            '-resource-dir', str(overlay), '-std=c++20', '-mlxbc',
            '-fenable-matrix', '-D__linx', '-DENABLE_TENSOR_INSTR',
            '-mllvm', '-enable-all-vector-as-tilereg=true']
        if os.environ.get('LINX_SYSROOT'):
            sysroot = Path(os.environ['LINX_SYSROOT'])
            cls.flags += ['--sysroot=' + str(sysroot), '-nostdinc++',
                          '-isystem', str(sysroot / 'usr/include/c++/v1')]
        cls.objdump = os.environ.get('LINX_OBJDUMP',
                                     str(Path(compiler).parent / 'llvm-objdump'))

    def compile(self, *args, source=SOURCE):
        result = subprocess.run(self.flags + list(args) + [str(source)],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_zero_threshold_ir_has_no_wrapper_boundary(self):
        output = self.directory / 'zero-threshold.ll'
        self.compile('-O2', '-mllvm', '-inline-threshold=0',
                     '-S', '-emit-llvm', '-o', str(output))
        ir = output.read_text()
        # Includes both calls and definitions, not just the primary overload.
        self.assertNotRegex(ir, r'@_Z8TIMG2COL[^\s(]*\(')
        functions = dict(re.findall(
            r'^define [^\n]*@(\w+)\([^\n]*\).*?\{\n(.*?)^}', ir, re.M | re.S))
        for name in CASES:
            with self.subTest(function=name):
                body = functions[name]
                self.assertEqual(body.count('BSTART.TIMG2COL'), 1, body)
                self.assertIn('B.IOT mask=1111, last, ->', body)
                # The collective producer must precede the consumer branch.
                self.assertIn('br i1', body.split('BSTART.TIMG2COL', 1)[1])

    def test_o2_o3_objects_have_only_explicit_stores(self):
        for opt in ('-O2', '-O3'):
            with self.subTest(optimization=opt):
                output = self.directory / (opt[1:] + '.o')
                self.compile(opt, '-c', '-o', str(output))
                dump = subprocess.run([self.objdump, '-d', '-M', 'no-tile-macros',
                                       str(output)], text=True, capture_output=True)
                self.assertEqual(dump.returncode, 0, dump.stderr)
                self.assertNotIn('_Z8TIMG2COL', dump.stdout)
                functions = dict(re.findall(
                    r'^[0-9a-f]+ <(\w+)>:\n(.*?)'
                    r'(?=^Disassembly of section|^[0-9a-f]+ <[A-Za-z_]|\Z)',
                    dump.stdout, re.M | re.S))
                self.assertTrue(set(CASES).issubset(functions), dump.stdout)
                for name in CASES:
                    body = functions[name]
                    self.assertEqual(body.count('BSTART.TIMG2COL'), 1, body)
                    # Explicit TSTORE is a TLSU bundle. Native TSTORE.M* spill
                    # or TLOAD reload is forbidden anywhere in these fixtures.
                    self.assertEqual(len(re.findall(
                        r'BSTART\.TLSU\s+TSTORE\b', body)), 1, body)
                    self.assertNotRegex(body, r'BSTART\.TLSU\s+TLOAD\b', body)
                self.assertNotRegex(dump.stdout, r'\bT(?:STORE|LOAD)\.')

    @unittest.skipUnless(os.environ.get('LINX_MODEL_ROOT'),
                         'set LINX_MODEL_ROOT for four-PE golden/canary checks')
    def test_four_pe_golden_and_canary(self):
        model = Path(os.environ['LINX_MODEL_ROOT']).resolve()
        start = self.directory / 'start.s'
        # Same bare entry as the public #164 reproducer; no Tile crosses main's
        # return ABI. The model starts four PEs at this entry.
        start.write_text(
            '.global _start\n.type _start,@function\n.text\n_start:\n'
            ' bstart.std call main\n c.setret 2, ->ra\n c.bstart.sys fall\n'
            ' addi zero, 0x5e, ->x1\n acrc 1\n c.bstop\n')
        nm = str(Path(self.flags[0]).parent / 'llvm-nm')
        rows_by_case = (16, 16, 16, 64, 17, 32, 128)
        for name, rows in zip(CASES, rows_by_case):
            with self.subTest(function=name):
                directory = self.directory / name
                directory.mkdir()
                source = directory / 'model.cpp'
                binary, output = directory / 'model.elf', directory / 'result.bin'
                values = [float(i % 13 - 6) for i in range(128 * 3)]
                source.write_text(
                    f'#include "{SOURCE}"\n'
                    'alignas(128) float input[] = {' +
                    ','.join(repr(v) + 'f' for v in values) + '};\n'
                    f'alignas(128) float result[{rows * 8 + 16}] = {{' +
                    ','.join(['93.0f'] * (rows * 8 + 16)) + '};\n'
                    f'int main() {{ {name}(input, result); return 0; }}\n')
                self.compile('-O2', '-ffunction-sections', '-fdata-sections',
                             '-nostdlib++', '-nostartfiles', '-static',
                             '-Wl,--gc-sections', str(start), '-o', str(binary),
                             source=source)
                symbols = subprocess.check_output([nm, str(binary)], text=True)
                address = int(re.search(r'^([0-9a-fA-F]+) \w result$',
                                        symbols, re.M)[1], 16)
                expected = [values[r * 3 + c] if c < 3 else 0.0
                            for r in range(rows) for c in range(8)] + [93.0] * 16
                golden = struct.pack('<' + str(len(expected)) + 'f', *expected)
                command = [str(model / 'bin/gfrun'), '-f', str(binary), '-s',
                           'softcore.multiThreadNum=4', '--dump-memory',
                           f'0x{address:x}:{len(golden)}:{output}']
                result = subprocess.run(command, cwd=model, text=True,
                                        capture_output=True, timeout=60)
                log = result.stdout + result.stderr
                self.assertEqual(result.returncode, 0, log)
                self.assertRegex(log, r'Reach the End of Benchmark! R2 = 0\s')
                self.assertTrue(output.exists(), log)
                self.assertEqual(output.read_bytes(), golden)


if __name__ == '__main__':
    unittest.main()
