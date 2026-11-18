#!/usr/bin/env python3
"""Issue 324: independent rational oracle, native objects and optional ELF run.

LINX_CXX/LINX_SYSROOT enable native checks. LINX_MODEL_ROOT additionally runs
the generated ELF with gfrun and compares the exported bytes to the oracle.
PTO_CVT_OUTPUT_DIR retains commands, ELF, disassembly, goldens and model logs.
"""
import ctypes
from fractions import Fraction
import json
import os
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
MODES = ('RNONE', 'RNE', 'RTZ', 'RDN', 'RUP', 'RNA', 'RTO', 'RHB')


def oracle(bits, mode, sat):
    """PTO common conversion on exact IEEE binary32 values, returning bits."""
    exponent, fraction = (bits >> 23) & 255, bits & 0x7fffff
    negative = bool(bits >> 31)
    if exponent == 255:
        if fraction:
            return 0
        return 0x80000000 if negative else 0x7fffffff
    value = Fraction(fraction if exponent == 0 else fraction + (1 << 23))
    power = -149 if exponent == 0 else exponent - 150
    value *= Fraction(2) ** power
    if negative:
        value = -value
    lower = value.numerator // value.denominator
    remainder = value - lower
    if mode in (0, 1):
        rounded = lower + (remainder > Fraction(1, 2) or
                           (remainder == Fraction(1, 2) and lower % 2 != 0))
    elif mode == 2:
        rounded = int(value)
    elif mode == 3:
        rounded = lower
    elif mode == 4:
        rounded = lower + (remainder != 0)
    elif mode == 5:
        rounded = lower + (remainder > Fraction(1, 2) or
                           (remainder == Fraction(1, 2) and not negative))
    elif mode == 6:
        rounded = lower if remainder == 0 or lower % 2 else lower + 1
    elif mode == 7:
        rounded = lower + (remainder >= Fraction(1, 2))
    if sat:
        rounded = max(-(1 << 31), min((1 << 31) - 1, rounded))
    return rounded & 0xffffffff


def inputs():
    result = [struct.unpack('<I', struct.pack('<f', v))[0]
              for v in (2.5, 3.5, -2.5, -3.5, 2.2, -2.2,
                        0, -0.0, 1, -1, 0.5, -0.5, 1.5, -1.5)]
    # Adjacent values at rounding, exponent, overflow and special boundaries.
    for center in (0, 1, 0x007fffff, 0x00800000, 0x3effffff, 0x3f000000,
                   0x3f800000, 0x4affffff, 0x4b000000, 0x4effffff,
                   0x4f000000, 0x4f800000, 0x5f000000, 0x7f7fffff,
                   0x7f800000, 0x7fc00000, 0x7f800001):
        for delta in (-1, 0, 1):
            bits = center + delta
            if 0 <= bits <= 0x7fffffff:
                result += [bits, bits | 0x80000000]
    rng = random.Random(324)
    result += [rng.getrandbits(32) for _ in range(256)]
    return result


def source(bits):
    cases = ','.join(hex(b) + 'u' for b in bits)
    return '''#include <common/pto_tileop.hpp>
extern "C" {
volatile uint32_t scalar_cvt_input[] = {''' + cases + '''};
// Guards catch over/under-writes; all result accesses are volatile.
volatile uint32_t scalar_cvt_result[16 * ''' + str(len(bits)) + ''' + 2] = {0x12345678};
}
template<int Mode, int Sat> __attribute__((noinline))
void convert(unsigned offset) {
  for (unsigned i = 0; i < sizeof(scalar_cvt_input)/4; ++i) {
    uint32_t raw = scalar_cvt_input[i];
    float value; __builtin_memcpy(&value, &raw, 4);
    int32_t result;
    linx_cvt<Mode, Sat>(result, value);
    scalar_cvt_result[offset + i] = static_cast<uint32_t>(result);
  }
}
int main() {
  constexpr unsigned N = sizeof(scalar_cvt_input)/4;
  scalar_cvt_result[16*N+1] = 0xabcdef01;
''' + '\n'.join(f'  convert<{m},{sat}>(1 + {sat * 8 + m} * N);'
                 for sat in range(2) for m in range(8)) + '''
  return 0;
}
'''


class ScalarConversion(unittest.TestCase):
    def test_required_halfway_results(self):
        expected = (
            [2, 4, -2, -4, 2, -2], [2, 4, -2, -4, 2, -2],
            [2, 3, -2, -3, 2, -2], [2, 3, -3, -4, 2, -3],
            [3, 4, -2, -3, 3, -2], [3, 4, -3, -4, 2, -2],
            [3, 3, -3, -3, 3, -3], [3, 4, -2, -3, 2, -2])
        for mode in range(8):
            self.assertEqual([oracle(b, mode, 0) for b in inputs()[:6]],
                             [x & 0xffffffff for x in expected[mode]])

    def test_integer_correction_against_rational_oracle(self):
        # Exercise production correction on the host; the primitive FCVT
        # result is independently supplied. Native execution below exercises
        # the actual primitive as well as the complete public entry point.
        with tempfile.TemporaryDirectory(prefix='scalar-cvt-host-') as tmp:
            d = Path(tmp)
            wrappers = '\n'.join(
                f'extern "C" uint32_t f{m}{sat}(uint32_t b, uint32_t r) {{'
                f'return pto::detail::scalar_cvt_f32_s32_finish<{m},{sat}>(b,r);}}'
                for m in range(8) for sat in range(2))
            (d/'host.cpp').write_text('#include <jcore/ScalarCvt.hpp>\n' + wrappers)
            subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O2',
                            '-fsanitize=undefined', '-fPIC', '-shared',
                            '-I'+str(ROOT/'include'), str(d/'host.cpp'),
                            '-o', str(d/'host.so')], check=True)
            lib = ctypes.CDLL(str(d/'host.so'))
            rng = random.Random(165)
            values = inputs() + [rng.getrandbits(32) for _ in range(10000)]
            for m in range(8):
                for sat in range(2):
                    fn = getattr(lib, f'f{m}{sat}')
                    fn.argtypes = (ctypes.c_uint32, ctypes.c_uint32)
                    fn.restype = ctypes.c_uint32
                    for b in values:
                        base = oracle(b, 2 if m in (6, 7) else m, 0)
                        self.assertEqual(fn(b, base), oracle(b, m, sat),
                                         f'mode={m} sat={sat} bits={b:#010x}')


@unittest.skipUnless(os.environ.get('LINX_CXX'), 'set LINX_CXX for native checks')
class NativeConversion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cxx = os.environ['LINX_CXX']
        if os.environ.get('PTO_CVT_OUTPUT_DIR'):
            cls.d = Path(os.environ['PTO_CVT_OUTPUT_DIR']).resolve()
            cls.d.mkdir(parents=True, exist_ok=True)
        else:
            cls.tmp = tempfile.TemporaryDirectory(prefix='scalar-cvt-native-')
            cls.addClassCleanup(cls.tmp.cleanup)
            cls.d = Path(cls.tmp.name)
        resource = Path(subprocess.check_output(
            [cls.cxx, '-print-resource-dir'], text=True).strip())
        overlay = cls.d/'resource'
        if overlay.exists():
            shutil.rmtree(overlay)
        shutil.copytree(resource, overlay, ignore=lambda d, names:
                        ['tileop-api'] if Path(d) == resource/'include' else [])
        shutil.copytree(ROOT/'include', overlay/'include/tileop-api')
        cls.flags = [cls.cxx, '--target=linx64v5-unknown-linux-musl', '-mcpu=janus',
                     '-std=c++20', '-mlxbc', '-fenable-matrix',
                     '-mllvm', '-enable-all-vector-as-tilereg=true',
                     '-resource-dir', str(overlay)]
        if os.environ.get('LINX_SYSROOT'):
            s = Path(os.environ['LINX_SYSROOT'])
            cls.flags += ['--sysroot='+str(s), '-nostdinc++',
                          '-isystem', str(s/'usr/include/c++/v1')]
        cls.objdump = str(Path(cls.cxx).parent/'llvm-objdump')
        cls.bits = inputs()
        (cls.d/'scalar.cpp').write_text(source(cls.bits))
        cls.commands = []

    def run_command(self, cmd, log):
        self.commands.append([str(s) for s in cmd])
        (self.d/'commands.json').write_text(json.dumps(self.commands, indent=2)+'\n')
        result = subprocess.run(cmd, text=True, capture_output=True, timeout=180)
        (self.d/log).write_text(result.stdout + result.stderr)
        return result

    def test_native_objects_and_wrappers(self):
        for opt in ('-O0', '-O2', '-O3'):
            obj = self.d/(opt[1:]+'.o')
            r = self.run_command(self.flags + [opt, '-c', str(self.d/'scalar.cpp'),
                                              '-o', str(obj)], opt[1:]+'-build.log')
            self.assertEqual(r.returncode, 0, r.stderr)
            dump = self.run_command([self.objdump, '-d', '--no-show-raw-insn', str(obj)],
                                    opt[1:]+'.dis')
            self.assertEqual(dump.returncode, 0, dump.stderr)
            for mnemonic in ('fcvtn', 'fcvtz', 'fcvtm', 'fcvtp', 'fcvta'):
                self.assertIn(mnemonic+'.fs2sw', dump.stdout)
            self.assertNotIn('v.cvt', dump.stdout)
        (self.d/'wrappers.cpp').write_text('''#include <common/pto_tileop.hpp>
extern "C" int32_t implicit(float x) { return linx_cvt_as<int32_t>(x); }
extern "C" int32_t explicit_mode(float x) {
 return linx_cvt_as<LINX_RHB, LINX_SAT, int32_t>(x);
}
extern "C" void alias(float *x) {
 linx_cvt<LINX_RNE>(*reinterpret_cast<int32_t *>(x), *x);
}
''')
        r = self.run_command(self.flags + ['-O2', '-c', str(self.d/'wrappers.cpp'),
                                          '-o', str(self.d/'wrappers.o')], 'wrappers.log')
        self.assertEqual(r.returncode, 0, r.stderr)

    def test_invalid_combinations(self):
        cases = [
            ('mode', 'int32_t d; float s=0; linx_cvt<8>(d,s);', 'invalid cvt RMode'),
            ('negative_mode', 'int32_t d; float s=0; linx_cvt<-1>(d,s);', 'invalid cvt RMode'),
            ('sat', 'int32_t d; float s=0; linx_cvt<1,2>(d,s);', 'invalid cvt Sat'),
            ('type', 'float d; int32_t s=0; linx_cvt(d,s);', 'supports only FP32 to S32'),
            ('double', 'int32_t d; double s=0; linx_cvt(d,s);', 'supports only FP32 to S32'),
            ('packed', '__fp16x2 d; float s=0; linx_cvt_packed(d,s,s);', 'linx_cvt_packed is unsupported'),
            ('p2p', '__fp16x2 d,s; linx_cvt(d,s);', 'supports only FP32 to S32'),
            ('as6', '__attribute__((address_space(6))) __fp16x2 *d=nullptr; '
             '__bf16x2 s; linx_cvt(*d,s);', 'supports only FP32 to S32'),
            ('package', '__fp16x2 d; float s=0; linx_cvt_package(d,s,s);', 'linx_cvt_packed is unsupported'),
            ('package_as', 'float s=0; (void)linx_cvt_package_as<__fp16x2>(s,s);', 'linx_cvt_packed is unsupported'),
            ('as', 'double s=0; (void)linx_cvt_as<int32_t>(s);', 'supports only FP32 to S32'),
        ]
        for name, body, diagnostic in cases:
            with self.subTest(case=name):
                p = self.d/(name+'.cpp')
                p.write_text('#include <common/pto_tileop.hpp>\nvoid f(){'+body+'}\n')
                r = self.run_command(self.flags + ['-O2', '-c', str(p), '-o',
                                                  str(self.d/(name+'.o'))], name+'.log')
                self.assertNotEqual(r.returncode, 0)
                self.assertIn(diagnostic, r.stderr)
                self.assertNotIn('Do not know how to promote', r.stderr)
                self.assertNotIn('PLEASE submit a bug report', r.stderr)

    @unittest.skipUnless(os.environ.get('LINX_MODEL_ROOT'), 'set LINX_MODEL_ROOT for numerical ELF execution')
    def test_native_elf_numerical_results(self):
        # Minimal single-PE launcher, matching the public issue 165 runner.
        (self.d/'start.s').write_text('''.global _start
.global scalar_cvt_done
.type _start,@function
.text
_start:
 bstart.std call main
 c.setret 2, ->ra
scalar_cvt_done:
 bstart.std direct, scalar_cvt_done
''')
        elf = self.d/'scalar.elf'
        r = self.run_command(self.flags + ['-O2', '-static', '-nostdlib',
                                          '-Wl,-e,_start', str(self.d/'start.s'),
                                          str(self.d/'scalar.cpp'), '-o', str(elf)], 'elf-build.log')
        self.assertEqual(r.returncode, 0, r.stderr)
        symbols = subprocess.check_output(['readelf', '-sW', str(elf)], text=True)
        address = next(int(line.split()[1], 16) for line in symbols.splitlines()
                       if line.split() and line.split()[-1] == 'scalar_cvt_result')
        done = next(int(line.split()[1], 16) for line in symbols.splitlines()
                    if line.split() and line.split()[-1] == 'scalar_cvt_done')
        expected = [0x12345678] + [oracle(b, m, sat) for sat in range(2)
                                  for m in range(8) for b in self.bits] + [0xabcdef01]
        golden = struct.pack('<'+'I'*len(expected), *expected)
        (self.d/'golden.bin').write_bytes(golden)
        dump = self.d/'observed.bin'
        if dump.exists():
            dump.unlink()
        model = Path(os.environ['LINX_MODEL_ROOT'])/'bin/gfrun'
        r = self.run_command([str(model), '-f', str(elf), '-s', 'softcore.multiThreadNum=1',
                              '--stop_pc', str(done),
                              '--dump-memory', f'0x{address:x}:{len(golden)}:{dump}'], 'model.log')
        self.assertEqual(r.returncode, 0, r.stderr[-3000:])
        self.assertTrue(dump.exists(), 'model did not export numerical results')
        observed = dump.read_bytes()
        self.assertEqual(len(observed), len(golden))
        actual = struct.unpack('<'+'I'*len(expected), observed)
        for i, (got, want) in enumerate(zip(actual, expected)):
            self.assertEqual(got, want, f'word={i} got={got:#x} expected={want:#x}')


if __name__ == '__main__':
    unittest.main()
