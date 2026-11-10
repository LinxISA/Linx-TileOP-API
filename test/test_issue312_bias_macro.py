#!/usr/bin/env python3
"""Caller -DBIAS must preserve MX/GEMV expansion and native code generation."""
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CASES = (None, '1', '42')


def bias_flags(bias):
    if bias is None:
        return []
    return ['-DBIAS' if bias == '1' else '-DBIAS=' + bias]


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.digest()


def semantic_ir(path):
    text = path.read_text()
    # Command-line macro definitions shift Clang SourceManager offsets. Only
    # normalize the !srcloc nodes used for inline-asm diagnostic locations;
    # retain every instruction, asm constraint, operand and other metadata.
    locations = set(re.findall(r'!srcloc !(\d+)', text))
    return re.sub(r'^!(\d+) = .*$',
                  lambda match: ('!' + match[1] + ' = !{}'
                                 if match[1] in locations else match[0]),
                  text, flags=re.M)


def source(bias):
    check = ('#ifdef BIAS\n#error BIAS unexpectedly defined\n#endif\n'
             if bias is None else
             f'#if !defined(BIAS) || BIAS != {bias}\n'
             '#error caller BIAS was changed\n#endif\n')
    return '#include <common/pto_tileop.hpp>\n' + check


class HostExpansion(unittest.TestCase):
    def test_bias_does_not_change_header_expansion(self):
        # Keep the real assembly macros: stripping asm would hide changes in
        # the emitters. Preprocessing needs no Linx backend or type checking.
        with tempfile.TemporaryDirectory(prefix='issue312-host-') as td:
            work = Path(td)
            cpp = work / 'header.cpp'
            output = work / 'header.ii'
            baseline = None
            for bias in CASES:
                with self.subTest(BIAS=bias):
                    cpp.write_text(source(bias))
                    flags = bias_flags(bias)
                    result = subprocess.run([
                        os.environ.get('CXX', 'c++'), '-std=c++20', '-D__linx',
                        '-DENABLE_TENSOR_INSTR', '-include',
                        str(ROOT / 'test/linx_host_type_shim.hpp'),
                        '-I', str(ROOT / 'include'), *flags,
                        '-E', '-P', str(cpp), '-o', str(output),
                    ], text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    actual = digest(output)
                    if bias is None:
                        baseline = actual
                    else:
                        self.assertEqual(actual, baseline,
                                         'caller BIAS changed header expansion')


class NativeCompilation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = os.environ.get('LINX_CXX')
        if not compiler and os.environ.get('COMPILER_DIR'):
            compiler = str(Path(os.environ['COMPILER_DIR']) / 'clang++')
        if not compiler:
            raise unittest.SkipTest('set LINX_CXX or COMPILER_DIR for native checks')
        cls.work = tempfile.TemporaryDirectory(prefix='issue312-native-')
        cls.addClassCleanup(cls.work.cleanup)
        cls.directory = Path(cls.work.name)
        resource = Path(subprocess.check_output(
            [compiler, '-print-resource-dir'], text=True).strip())
        overlay = cls.directory / 'resource'
        # Linx Clang searches builtin TileOP before -I. Force the candidate
        # headers into a resource overlay to avoid testing installed headers.
        shutil.copytree(resource, overlay, ignore=lambda directory, names:
                        ['tileop-api'] if Path(directory) == resource / 'include' else [])
        shutil.copytree(ROOT / 'include', overlay / 'include/tileop-api')
        cls.flags = [compiler, '--target=' + os.environ.get(
            'LINX_TARGET', 'linx64v5-unknown-linux-musl'),
            '-resource-dir', str(overlay), '-std=c++20', '-O2',
            '-mlxbc', '-fenable-matrix', '-D__linx', '-DENABLE_TENSOR_INSTR']
        if os.environ.get('LINX_SYSROOT'):
            sysroot = Path(os.environ['LINX_SYSROOT'])
            cls.flags += ['--sysroot=' + str(sysroot), '-nostdinc++',
                          '-isystem', str(sysroot / 'usr/include/c++/v1')]

    def test_public_header_with_caller_bias(self):
        cpp = self.directory / 'header.cpp'
        for bias in CASES:
            with self.subTest(BIAS=bias):
                cpp.write_text(source(bias))
                flags = bias_flags(bias)
                result = subprocess.run(self.flags + flags + [
                    '-fsyntax-only', str(cpp),
                ], text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_mx_scale_variants_keep_identical_ir(self):
        # Existing fixture instantiates PLAIN/ACC/BIAS, Local/Shared matrices,
        # GEMV, absent/single/both scales and a PostProcess path.
        cpp = ROOT / 'test/tileop_api/src/MXScaleVariants.cpp'
        output = self.directory / 'variants.ll'
        baseline = None
        for bias in CASES:
            with self.subTest(BIAS=bias):
                flags = bias_flags(bias)
                result = subprocess.run(self.flags + flags + [
                    '-S', '-emit-llvm', str(cpp), '-o', str(output),
                ], text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                actual = semantic_ir(output)
                if bias is None:
                    baseline = actual
                else:
                    self.assertEqual(actual, baseline,
                                     'caller BIAS changed emitted LLVM IR')


if __name__ == '__main__':
    unittest.main()
