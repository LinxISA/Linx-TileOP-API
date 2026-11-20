#!/usr/bin/env python3
"""Issue #325 target contract: compile diagnostics, never a numerical claim."""
import shlex
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
DIAGNOSTIC = 'legacy SIMT conversion/BF16 API unavailable'
HEADER = '#include <common/pto_tileop.hpp>\n'


class LegacySIMTContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = os.environ.get('LINX_CXX')
        if not compiler and os.environ.get('COMPILER_DIR'):
            compiler = str(Path(os.environ['COMPILER_DIR']) / 'clang++')
        if not compiler:
            raise unittest.SkipTest('set LINX_CXX or COMPILER_DIR for native checks')
        cls.work = tempfile.TemporaryDirectory(prefix='issue325-contract-')
        cls.addClassCleanup(cls.work.cleanup)
        cls.directory = Path(cls.work.name)
        resource = Path(subprocess.check_output(
            [compiler, '-print-resource-dir'], text=True).strip())
        overlay = cls.directory / 'resource'
        # Resource headers take precedence over -I for Linx's forced includes.
        shutil.copytree(resource, overlay, ignore=lambda directory, names:
                        ['tileop-api'] if Path(directory) == resource / 'include' else [])
        shutil.copytree(ROOT / 'include', overlay / 'include/tileop-api')
        cls.flags = [compiler, '--target=' + os.environ.get(
            'LINX_TARGET', 'linx64v5-unknown-linux-musl'),
            '-resource-dir', str(overlay), '-std=c++20', '-mlxbc',
            '-fenable-matrix', '-ferror-limit=0',
            '-mllvm', '-enable-all-vector-as-tilereg=true']
        cls.flags += shlex.split(os.environ.get('LINX_TEST_CXXFLAGS', ''))
        if os.environ.get('LINX_SYSROOT'):
            sysroot = Path(os.environ['LINX_SYSROOT'])
            cls.flags += ['--sysroot=' + str(sysroot), '-nostdinc++',
                          '-isystem', str(sysroot / 'usr/include/c++/v1')]

    def compile(self, source, *flags, cpu='janus'):
        return subprocess.run(
            self.flags + ['-mcpu=' + cpu, '-O2', *flags, '-x', 'c++', '-'],
            input=source, text=True, capture_output=True)

    def reject(self, source, message=DIAGNOSTIC, errors=None):
        result = self.compile(HEADER + source, '-c', '-o', os.devnull)
        self.assertNotEqual(result.returncode, 0, 'unsupported API compiled')
        self.assertIn(message, result.stderr)
        self.assertNotRegex(result.stderr,
                            r'unknown operand|PLEASE submit a bug report|'
                            r'LLVM ERROR|Stack dump:|Segmentation fault|'
                            r'cannot select|Cannot select')
        if errors is not None:
            lines = [line for line in result.stderr.splitlines() if 'error:' in line]
            self.assertEqual(len(lines), errors, result.stderr)
            for line in lines:
                self.assertIn(DIAGNOSTIC, line, result.stderr)
        return result.stderr

    def simt_macros(self):
        result = self.compile('', '-dM', '-E', cpu='simt')
        self.assertEqual(result.returncode, 0, result.stderr)
        return dict(re.findall(r'^#define (\w+)\s+([^\n]+)$', result.stdout, re.M))

    def test_include_only_and_storage_are_legal_on_janus(self):
        source = HEADER + '''
static_assert(sizeof(__bf16x2) == sizeof(uint32_t));
extern "C" void store_bits(uint32_t *out, const __bf16x2 &pair) {
  __builtin_memcpy(out, &pair.data, sizeof(*out));
}
'''
        for opt in ('-O0', '-O2'):
            with self.subTest(optimization=opt):
                result = self.compile(source, opt, '-c', '-o', os.devnull)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_original_conversion_and_add_compile_without_simt(self):
        result = self.compile(HEADER + """
void kernel(uint32_t *output, float a, float b) {
  __bf16x2 pair{};
  linx_cvt_packed(pair, a, b);
  output[0] = pto::bf16x2_to_bits(pair);
  output[1] = pto::bf16x2_to_bits(blkv_bf16x2_add(pair, pair));
}
""", '-c', '-o', os.devnull)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_raw_bits_helpers_preserve_target_float_carrier(self):
        source = HEADER + '''
extern "C" uint32_t read_bits(const __bf16x2 &x) {
  return pto::bf16x2_to_bits(x);
}
extern "C" void write_bits(__bf16x2 &x, uint32_t bits) {
  x = pto::bf16x2_from_bits(bits);
}
extern "C" uint32_t lanes(uint16_t lo, uint16_t hi) {
  return pto::bf16x2_to_bits(pto::bf16x2_pack_bits(lo, hi));
}
extern "C" uint16_t high_lane(const __bf16x2 &x) {
  return pto::bf16x2_lane_bits<1>(x);
}
'''
        result = self.compile(source, '-S', '-emit-llvm', '-o', '-')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotRegex(result.stdout, r'\b(?:fptoui|fptosi|uitofp|sitofp)\b')
        # The compiler's floating carrier must be moved as bits. Optimizers
        # may use integer loads/stores (including scoped Scalar intrinsics)
        # or explicit bitcasts.
        self.assertRegex(result.stdout,
                         r'load i32|bitcast float .* to i32|'
                         r'call i64 @llvm\.linx\.scalar\.load')
        self.assertRegex(result.stdout,
                         r'store i32|bitcast i32 .* to float|'
                         r'call void @llvm\.linx\.scalar\.store')
        result = self.compile(source, '-c', '-o', os.devnull)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.reject('''
uint16_t invalid(__bf16x2 x) { return pto::bf16x2_lane_bits<2>(x); }
''', message='BF16x2 lane must be 0 or 1')

    def test_all_conversion_entry_points(self):
        cases = {
            'normal': 'void f(__bf16 &d, float s) { linx_cvt(d, s); }',
            'unsupported_packed': 'void f(__fp16x2 &d, float s) { linx_cvt_packed(d, s, s); }',
            'packed_to_packed':
                'void f(__fp16x2 &d, __bf16x2 s) { linx_cvt(d, s); }',
            'address_space_normal':
                'void f(__fp8_e6m2x2_as6 &d, __bf16x2 s) { linx_cvt(d, s); }',
            'address_space_packed':
                'void f(__bf16x2_as6 &d, float s) { linx_cvt_packed(d, s, s); }',
            'normal_return': '__bf16 f(float s) { return linx_cvt_as<__bf16>(s); }',
            'package':
                'void f(__fp16x2 &d, float s) { linx_cvt_package(d, s, s); }',
            'package_return':
                '__fp16x2 f(float s) { return linx_cvt_package_as<__fp16x2>(s, s); }',
        }
        # Keep each wrapper isolated: one failing specialization cannot mask
        # the absence of the check from another entry point.
        for name, source in cases.items():
            with self.subTest(entry=name):
                self.reject(source, errors=1)

    def test_bf16_operations_and_aliases(self):
        operations = [('fadd', 2), ('fsub', 2), ('fmul', 2), ('fdiv', 2),
                      ('fexp', 1), ('fsqrt', 1), ('fmadd', 3), ('fmsub', 3),
                      ('fmax', 2)]
        aliases = [('add', 2), ('sub', 2), ('mul', 2), ('div', 2),
                   ('exp', 1), ('sqrt', 1), ('madd', 3), ('msub', 3),
                   ('max', 2)]
        for names in (operations, aliases):
            for output_reference in (False, True):
                with self.subTest(aliases=names is aliases,
                                  output_reference=output_reference):
                    lines = []
                    for family, dtype in [('bf16', '__bf16')]:
                        for name, arity in names:
                            inputs = ', '.join(['a'] * arity)
                            call = f'blkv_{family}_{name}('
                            if output_reference:
                                body = call + 'd, ' + inputs + ');'
                                returns = 'void'
                            else:
                                body = 'return ' + call + inputs + ');'
                                returns = dtype
                            lines.append(f'{returns} test_{family}_{name}('
                                         f'{dtype} &d, const {dtype} &a) {{ {body} }}')
                    self.reject('\n'.join(lines), errors=9)
        # The historical fmmad spelling is also a public alias.
        self.reject('''
__bf16 a(__bf16 x) { return blkv_bf16_fmmad(x, x, x); }

''', errors=1)

    def test_invalid_types_and_options_remain_rejected(self):
        cases = [
            ('void f(float &d, __bf16x2 s) { linx_cvt(d, s); }',
             'unsupported linx_cvt(dst, src) type pair'),
            ('void f(float &d, __fp16x2 s) { linx_cvt(d, s); }', 'deleted'),
            ('void f(__bf16x2 &d) { linx_cvt_packed(d, 1.f, 2); }',
             'unsupported linx_cvt_packed'),
            ('void f(__bf16x2 &d) { linx_cvt_packed<8>(d, 1.f, 2.f); }',
             'invalid bf16 RMode'),
            ('void f(__bf16x2 &d) { linx_cvt_packed<0, 2>(d, 1.f, 2.f); }',
             'invalid bf16 Sat'),
            ('__bf16x2 f(__bf16x2 x) { return blkv_bf16x2_add<8>(x, x); }',
             'invalid bf16 RMode'),
            ('__bf16 f(__bf16 x) { return blkv_bf16_add<0, 2>(x, x); }',
             'invalid bf16 Sat'),
        ]
        for source, message in cases:
            with self.subTest(source=source):
                self.reject(source, message=message)

    def test_bf16_requires_real_matching_operand_types(self):
        # A struct with the same .data shape and a BF16x2 passed to scalar
        # BF16 arithmetic must not silently acquire SIMT operand semantics.
        source = r'''
struct Fake { unsigned data; };
void a(__bf16 &d, Fake x) { blkv_bf16_fadd(d, x, x); }
void b(__bf16x2 &d, Fake x) { blkv_bf16x2_fsqrt(d, x); }
void c(__bf16 &d, __bf16x2 x) { blkv_bf16_fmadd(d, x, x, x); }
void e(__bf16x2 &d, __bf16 x) { blkv_bf16x2_fmax(d, x, x); }
'''
        result = self.reject(source,
                             message='requires matching BF16 operand types')
        errors = [line for line in result.splitlines()
                  if 'error:' in line and 'requires matching BF16' in line]
        self.assertEqual(len(errors), 4, result)

    def test_missing_capability_marker_fails_closed_even_for_simt_cpu(self):
        # Also emulate an older compiler on a new toolchain, without inventing
        # an opt-in macro or enabling SIMT on Janus.
        result = self.compile(HEADER + '''
__fp16x2 f(float x) { __fp16x2 d; linx_cvt_packed(d, x, x); return d; }
''', '-U__linx_simt_contract', '-S', '-emit-llvm', '-o', '-', cpu='simt')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(DIAGNOSTIC, result.stderr)

    def test_vec_attribute_does_not_bypass_translation_unit_contract(self):
        self.reject('''
__vec__ void f(__fp16x2 &d, float x) { linx_cvt_packed(d, x, x); }
''')

    def test_explicit_disabled_simt_feature_fails_closed(self):
        if self.simt_macros().get('__linx_simt_contract') != '1':
            self.skipTest('compiler does not publish the SIMT target contract')
        result = self.compile(HEADER + '''
void f(__fp16x2 &d, float x) { linx_cvt_packed(d, x, x); }
''', '-Xclang', '-target-feature', '-Xclang', '-simt',
             '-S', '-emit-llvm', '-o', '-', cpu='simt')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(DIAGNOSTIC, result.stderr)

    def test_unsupported_janus_numeric_controls(self):
        cases = [
            ('__bf16x2 f(__bf16x2 x) { return blkv_bf16x2_exp(x); }',
             'Janus packed BF16 exp is unsupported'),
            ('__bf16x2 f(__bf16x2 x) { return blkv_bf16x2_add<7>(x,x); }',
             'Janus packed BF16 RHB is unsupported'),
            ('__bf16x2 f(float x) { return linx_cvt_package_as<0,1,__bf16x2>(x,x); }',
             'Janus packed BF16 saturation is unsupported'),
        ]
        for source, message in cases:
            self.reject(source, message=message)

    def test_verified_simt_retains_legacy_ir(self):
        macros = self.simt_macros()
        if macros.get('__linx_simt_contract') != '1':
            self.skipTest('compiler does not publish the SIMT target contract')
        self.assertEqual(macros.get('__linx_simt'), '1')
        result = self.compile(HEADER + '''
extern "C" __bf16x2 legacy(float a, float b) {
  __bf16x2 pair;
  linx_cvt_packed(pair, a, b);
  return blkv_bf16x2_add(pair, pair);
}
''', '-S', '-emit-llvm', '-o', '-', cpu='simt')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('v.cvt.fp322bf16x2', result.stdout)
        self.assertIn('v.fadd', result.stdout)
        self.assertIn('=@2vr,@2vr,@2vr', result.stdout)
        # IR presence verifies routing only, not assembler legality or numbers.


if __name__ == '__main__':
    unittest.main()
