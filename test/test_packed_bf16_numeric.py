#!/usr/bin/env python3
"""Exact-rational oracle and post-kernel model observation for packed BF16.

A numerical PASS requires matching every dumped byte at the requested observer
PC. The observer is not an architectural kernel End or a runtime lifecycle test.
"""
import shlex
import ctypes
from fractions import Fraction
import hashlib
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


def finite(x):
    exponent, fraction = (x >> 7) & 255, x & 127
    value = Fraction((128 if exponent else 0) + fraction) * Fraction(2) ** (
        exponent - 134 if exponent else -133)
    return -value if x & 0x8000 else value


def nan(x): return (x & 0x7fff) > 0x7f80

def inf(x): return (x & 0x7fff) == 0x7f80

def zero(x): return (x & 0x7fff) == 0


def round_value(value, mode, negative_zero=False, square=False):
    negative = value < 0 or (value == 0 and negative_zero)
    value = abs(value)
    lo, hi = 0, 0x7f80
    def candidate(code):
        return Fraction(2) ** 128 if code == 0x7f80 else finite(code)
    def metric(v): return v * v if square else v
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if metric(candidate(mid)) <= value: lo = mid
        else: hi = mid
    exact = metric(candidate(lo)) == value
    result = lo
    if not exact:
        midpoint = (candidate(lo) + candidate(hi)) / 2
        relation = (value > metric(midpoint)) - (value < metric(midpoint))
        if mode == 6: result |= 1
        elif ((mode == 3 and negative) or (mode == 4 and not negative) or
              (mode in (0, 1, 5) and
               (relation > 0 or (relation == 0 and (mode == 5 or lo & 1))))):
            result = hi
    return result | (0x8000 if negative else 0)


def oracle(op, a, b, c, mode):
    if nan(a) or (op != 4 and nan(b)) or (op in (5, 6) and nan(c)):
        return 0x7fc0
    if op == 7:
        if zero(a) and zero(b): return a & b
        return a if finite_or_inf(a) > finite_or_inf(b) else b
    if op == 4:
        if zero(a): return a
        if a & 0x8000: return 0x7fc0
        if inf(a): return a
        return round_value(finite(a), mode, square=True)
    if op == 1: b ^= 0x8000
    if op in (0, 1):
        if inf(a) or inf(b):
            if inf(a) and inf(b) and (a ^ b) & 0x8000: return 0x7fc0
            return a if inf(a) else b
        v = finite(a) + finite(b)
        negzero = (bool(a & 0x8000) if zero(a) and zero(b) and not (a ^ b) & 0x8000 else mode == 3)
        return round_value(v, mode, negzero)
    sign = (a ^ b) & 0x8000
    if op == 3:
        if (zero(a) and zero(b)) or (inf(a) and inf(b)): return 0x7fc0
        if inf(a) or zero(b): return sign | 0x7f80
        if inf(b) or zero(a): return sign
        return round_value(finite(a) / finite(b), mode)
    if (inf(a) and zero(b)) or (inf(b) and zero(a)): return 0x7fc0
    if op == 2:
        if inf(a) or inf(b): return sign | 0x7f80
        return round_value(finite(a) * finite(b), mode, bool(sign))
    if op == 6: c ^= 0x8000
    if inf(a) or inf(b):
        if inf(c) and bool(c & 0x8000) != bool(sign): return 0x7fc0
        return sign | 0x7f80
    if inf(c): return c
    product, addend = finite(a) * finite(b), finite(c)
    negzero = (bool(sign) if product == addend == 0 and bool(sign) == bool(c & 0x8000) else mode == 3)
    return round_value(product + addend, mode, negzero)


def finite_or_inf(x):
    if inf(x): return float('-inf') if x & 0x8000 else float('inf')
    return finite(x)


# Host bridge calls the same integer backend as the Janus wrappers. Test the
# public wrappers separately in the target fixture below.
BRIDGE = r'''
#include <common/pto_packed_bf16.hpp>
using namespace pto::detail;
template<int M> unsigned evaluate(unsigned op, unsigned a, unsigned b, unsigned c) {
 switch(op) {
 case 0: return bf16_add<M>(a,b);
 case 1: return bf16_add<M>(a,b^0x8000);
 case 2: return bf16_multiply<M>(a,b);
 case 3: return bf16_divide<M>(a,b);
 case 4: return bf16_sqrt<M>(a);
 case 5: return bf16_fused<M>(a,b,c);
 case 6: return bf16_fused<M>(a,b,c^0x8000);
 default: return bf16_maximum(a,b);
 }
}
extern "C" unsigned evaluate(unsigned op,unsigned a,unsigned b,unsigned c,unsigned m) {
 switch(m) {
 case 0:return evaluate<0>(op,a,b,c); case 1:return evaluate<1>(op,a,b,c);
 case 2:return evaluate<2>(op,a,b,c); case 3:return evaluate<3>(op,a,b,c);
 case 4:return evaluate<4>(op,a,b,c); case 5:return evaluate<5>(op,a,b,c);
 default:return evaluate<6>(op,a,b,c);
 }
}
extern "C" unsigned convert(unsigned bits,unsigned mode) {
 float f; __builtin_memcpy(&f,&bits,4);
 switch(mode) {
 case 0:return bf16_from_float<0>(f);case 1:return bf16_from_float<1>(f);
 case 2:return bf16_from_float<2>(f);case 3:return bf16_from_float<3>(f);
 case 4:return bf16_from_float<4>(f);case 5:return bf16_from_float<5>(f);
 default:return bf16_from_float<6>(f);
 }
}
'''


def vectors(count):
    rng = random.Random(325)
    special = [0, 0x8000, 1, 0x8001, 0x7f, 0x80, 0x3f80, 0xbf80,
               0x3f81, 0x7f7f, 0xff7f, 0x7f80, 0xff80, 0x7fc1, 0x7f81]
    for mode in range(7):
        for op in range(8):
            for i in range(count):
                a, b, c = [rng.choice(special) if i < 40 else rng.randrange(65536)
                           for _ in range(3)]
                yield op, a, b, c, mode


class NumericHost(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.TemporaryDirectory(prefix='packed-bf16-host-')
        cls.addClassCleanup(cls.work.cleanup)
        d = Path(cls.work.name)
        (d/'bridge.cpp').write_text(BRIDGE)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2',
                        '-shared', '-fPIC', '-I'+str(ROOT/'include'),
                        str(d/'bridge.cpp'), '-o', str(d/'bridge.so')], check=True)
        cls.lib = ctypes.CDLL(str(d/'bridge.so'))
        cls.lib.evaluate.argtypes = [ctypes.c_uint]*5
        cls.lib.convert.argtypes = [ctypes.c_uint]*2

    def test_exact_rational_arithmetic_all_modes(self):
        for case in vectors(160):
            actual, expected = self.lib.evaluate(*case), oracle(*case)
            self.assertEqual(actual, expected, (case, hex(actual), hex(expected)))

    def test_fused_cancellation_and_extreme_rounding(self):
        for case in target_cases():
            if case[0] != 8:
                self.assertEqual(self.lib.evaluate(*case), oracle(*case), case)
        # Fused BF16 preserves 2^-14; rounding the product first gives zero.
        self.assertEqual(self.lib.evaluate(5, 0x3f81, 0x3f81, 0xbf82, 0), 0x3880)
        product = self.lib.evaluate(2, 0x3f81, 0x3f81, 0, 0)
        self.assertEqual(self.lib.evaluate(0, product, 0xbf82, 0, 0), 0)

    def test_all_sqrt_encodings(self):
        for a in range(65536):
            self.assertEqual(self.lib.evaluate(4,a,0,0,0), oracle(4,a,0,0,0), hex(a))

    def test_float_conversion_boundaries_and_specials(self):
        for upper in range(65536):
            for tail in (0, 0x7fff, 0x8000, 0x8001, 0xffff):
                bits = (upper << 16) | tail
                for mode in range(7):
                    self.assertEqual(self.lib.convert(bits, mode),
                                     conversion_oracle(bits, mode), (hex(bits), mode))


def conversion_oracle(bits, mode):
    upper, tail = bits >> 16, bits & 65535
    if (bits & 0x7fffffff) > 0x7f800000:
        return 0x7fc0
    if tail:
        if mode == 6:
            upper |= 1
        elif (mode == 3 and bits >> 31 or
              mode == 4 and not bits >> 31 or
              mode in (0, 1, 5) and (tail > 32768 or
                tail == 32768 and (mode == 5 or upper & 1))):
            upper += 1
    return upper & 65535


def target_cases():
    yield from vectors(48)
    # Directed cancellation, ties, overflow/underflow and fused-vs-unfused cases.
    triples = [(0x3f81, 0x3f81, 0xbf82), (0x3f80, 0x3b80, 0xbf80),
               (0x7f7f, 0x4000, 0xff7f), (1, 0x3f00, 1),
               (0x80, 0x3f7f, 0x8001), (0x3f80, 0xbf80, 0x8000)]
    fp32 = [0, 0x80000000, 1, 0x00008000, 0x007fffff, 0x00800000,
            0x3f807fff, 0x3f808000, 0x3f808001, 0x3f818000, 0xbf808000,
            0xbf808001, 0x7f7f8000, 0xff7f8000, 0x7f800000, 0xff800000,
            0x7f800001, 0xffc12345, 0x3f800000, 0x40000000]
    for mode in range(7):
        for op in range(8):
            for a, b, c in triples:
                yield op, a, b, c, mode
        for i, bits in enumerate(fp32):
            yield 8, bits, fp32[(i + 9) % len(fp32)], 0, mode


class NumericTarget(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cxx = os.environ.get('LINX_CXX')
        if not cxx: raise unittest.SkipTest('set LINX_CXX for Janus compilation')
        cls.work = tempfile.TemporaryDirectory(prefix='packed-bf16-target-')
        cls.addClassCleanup(cls.work.cleanup)
        cls.d = Path(cls.work.name)
        resource = Path(subprocess.check_output([cxx,'-print-resource-dir'],text=True).strip())
        overlay = cls.d/'resource'
        shutil.copytree(resource,overlay,ignore=lambda directory,names:
                        ['tileop-api'] if Path(directory)==resource/'include' else [])
        shutil.copytree(ROOT/'include',overlay/'include/tileop-api')
        # In-tree Clang may not have compiler-rt installed in its resource dir.
        runtime = os.environ.get('LINX_BUILTINS')
        if runtime:
            lib = overlay/'lib/linux'
            lib.mkdir(parents=True, exist_ok=True)
            shutil.copy2(runtime, lib/'libclang_rt.builtins-linx64v5.a')
        cls.flags=[cxx,'--target=linx64v5-unknown-linux-musl','-mcpu=janus',
                   '-resource-dir',str(overlay),'-std=c++20','-mlxbc','-fenable-matrix',
                   '-O2','-mllvm','-enable-all-vector-as-tilereg=true']
        cls.flags += shlex.split(os.environ.get('LINX_TEST_CXXFLAGS', ''))
        if os.environ.get('LINX_SYSROOT'):
            sysroot=Path(os.environ['LINX_SYSROOT'])
            cls.flags += ['--sysroot='+str(sysroot),'-nostdinc++','-isystem',str(sysroot/'usr/include/c++/v1')]

    def test_public_api_and_model(self):
        cases = list(target_cases())
        # One word carries two different lanes; execute public wrappers and raw
        # reads from guest memory so the input cannot be constant-folded.
        rows=[]; expected=[]
        for op, a, b, c, mode in cases:
            if op == 8:
                rows.append(f'{{8,{a}u,{b}u,0,{mode}}}')
                expected.append(conversion_oracle(a, mode) |
                                (conversion_oracle(b, mode) << 16))
            else:
                rows.append(f'{{{op},{a | ((a^0x8000)<<16)}u,{b | (b<<16)}u,{c | (c<<16)}u,{mode}}}')
                expected.append(oracle(op,a,b,c,mode) |
                                (oracle(op,a^0x8000,b,c,mode)<<16))
        source = '#include <common/pto_tileop.hpp>\n'+r'''
struct Input {unsigned op,a,b,c,mode;};
extern "C" {
volatile Input inputs[] = {ROWS};
alignas(128) unsigned result[COUNT*4+5];
}
template<int M> __bf16x2 run(unsigned op,__bf16x2 a,__bf16x2 b,__bf16x2 c,bool reference) {
 if(op==8){
  unsigned ab=pto::bf16x2_to_bits(a),bb=pto::bf16x2_to_bits(b);
  float lo,hi;__builtin_memcpy(&lo,&ab,4);__builtin_memcpy(&hi,&bb,4);
  if(reference) return linx_cvt_package_as<M,0,__bf16x2>(lo,hi);
  __bf16x2 out;linx_cvt_packed<M>(out,lo,hi);return out;
 }
 if(reference){
  switch(op){
  case 0:blkv_bf16x2_add<M>(a,a,b);break;case 1:blkv_bf16x2_sub<M>(a,a,b);break;
  case 2:blkv_bf16x2_mul<M>(a,a,b);break;case 3:blkv_bf16x2_div<M>(a,a,b);break;
  case 4:blkv_bf16x2_sqrt<M>(a,a);break;case 5:blkv_bf16x2_fmmad<M>(a,a,b,c);break;
  case 6:blkv_bf16x2_msub<M>(a,a,b,c);break;default:blkv_bf16x2_max(a,a,b);break;
  }
  return a;
 }
 switch(op){
 case 0:return blkv_bf16x2_add<M>(a,b);case 1:return blkv_bf16x2_sub<M>(a,b);
 case 2:return blkv_bf16x2_mul<M>(a,b);case 3:return blkv_bf16x2_div<M>(a,b);
 case 4:return blkv_bf16x2_sqrt<M>(a);case 5:return blkv_bf16x2_madd<M>(a,b,c);
 case 6:return blkv_bf16x2_msub<M>(a,b,c);default:return blkv_bf16x2_max(a,b);
 }
}
__attribute__((noinline)) void kernel(){
 for(unsigned i=0;i<COUNT;++i){
  Input x{inputs[i].op,inputs[i].a,inputs[i].b,inputs[i].c,inputs[i].mode};auto a=pto::bf16x2_from_bits(x.a),b=pto::bf16x2_from_bits(x.b),c=pto::bf16x2_from_bits(x.c);
  __bf16x2 out;
  switch(x.mode){
  case 0:out=run<0>(x.op,a,b,c,i&1);break;case 1:out=run<1>(x.op,a,b,c,i&1);break;
  case 2:out=run<2>(x.op,a,b,c,i&1);break;case 3:out=run<3>(x.op,a,b,c,i&1);break;
  case 4:out=run<4>(x.op,a,b,c,i&1);break;case 5:out=run<5>(x.op,a,b,c,i&1);break;
  default:out=run<6>(x.op,a,b,c,i&1);break;}
  result[i]=pto::bf16x2_to_bits(out);
  float lo=pto::bf16x2_to_float<0>(out),hi=pto::bf16x2_to_float<1>(out);
  __builtin_memcpy(result+COUNT+i*2,&lo,4);
  __builtin_memcpy(result+COUNT+i*2+1,&hi,4);
  // Raw roundtrip retains NaN payloads and both lane positions.
  result[COUNT*3+i]=pto::bf16x2_to_bits(pto::bf16x2_from_bits(x.a));
 }
 __bf16x2 pair;
 linx_cvt_packed(pair,1.f,2.f);
 result[COUNT*4]=pto::bf16x2_to_bits(pair);
 result[COUNT*4+1]=pto::bf16x2_to_bits(blkv_bf16x2_add(pair,pair));
 float lo=pto::bf16x2_to_float<0>(pair),hi=pto::bf16x2_to_float<1>(pair);
 __builtin_memcpy(result+COUNT*4+2,&lo,4);__builtin_memcpy(result+COUNT*4+3,&hi,4);
 result[COUNT*4+4]=0xa55aa55a;
}
extern "C" __attribute__((noinline,noreturn)) void bf16_numeric_done(){
 for(;;) asm volatile("" ::: "memory");
}
int main(){kernel();bf16_numeric_done();}
'''.replace('ROWS',','.join(rows)).replace('COUNT',str(len(cases)))
        cpp=self.d/'model.cpp';cpp.write_text(source)
        obj=self.d/'model.o'
        result=subprocess.run(self.flags+['-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        ir=subprocess.run(self.flags+['-S','-emit-llvm',str(cpp),'-o','-'],capture_output=True,text=True)
        self.assertEqual(ir.returncode,0,ir.stderr)
        self.assertNotRegex(ir.stdout,r'asm[^\n]*v\.(?:cvt|fadd|fmul)')
        model=os.environ.get('LINX_MODEL_ROOT')
        if not model: self.skipTest('object passed; set LINX_MODEL_ROOT for numerical execution')
        model=Path(model).resolve()
        start = self.d/'start.s'
        start.write_text('.global _start\n.type _start,@function\n.text\n_start:\n'
                         ' bstart.std call main\n c.setret 1, ->ra\n'
                         '.Lhalt:\n c.bstart.std direct .Lhalt\n')
        elf=self.d/'model.elf'
        command=self.flags+['-ffunction-sections','-fdata-sections','-nostdlib++','-nostartfiles','-static','-Wl,--gc-sections',str(obj),str(start),'-o',str(elf)]
        linked=subprocess.run(command,capture_output=True,text=True)
        self.assertEqual(linked.returncode,0,linked.stderr)
        nm=Path(self.flags[0]).parent/'llvm-nm'
        symbols=subprocess.check_output([str(nm),str(elf)],text=True)
        address=int(re.search(r'^([0-9a-fA-F]+) \w result$',symbols,re.M)[1],16)
        arithmetic_expected = expected[:]
        expected += [lane for word in arithmetic_expected
                     for lane in ((word & 65535) << 16, (word >> 16) << 16)]
        expected += [int(row.split(',')[1].removesuffix('u')) for row in rows]
        expected += [0x40003f80,0x40804000,0x3f800000,0x40000000,0xa55aa55a]
        golden=struct.pack('<'+'I'*len(expected),*expected)
        output=self.d/'result.bin'
        command=[str(model/'bin/gfrun'),'-f',str(elf),'-s','softcore.multiThreadNum=1','--dump-memory',f'0x{address:x}:{len(golden)}:{output}']
        stop_address = int(re.search(r'^([0-9a-fA-F]+) \w bf16_numeric_done$', symbols, re.M)[1], 16)
        # ParseArgs reads integer controls in decimal (despite old help examples).
        command += ['--stop_pc', str(stop_address)]
        state = self.d/'state.json'
        command += ['--dump-run-state', str(state)]
        timed_out = False
        try:
            run = subprocess.run(command, cwd=model, capture_output=True, text=True,
                                 timeout=int(os.environ.get('LINX_MODEL_TIMEOUT', '300')))
            log, exit_code = run.stdout + run.stderr, run.returncode
        except subprocess.TimeoutExpired as error:
            timed_out, exit_code = True, None
            log = (error.stdout or b'').decode() + (error.stderr or b'').decode()
        actual = output.read_bytes() if output.exists() else b''
        terminal = json.loads(state.read_text()) if state.exists() else {}
        passed = (not timed_out and exit_code == 0 and actual == golden and
                  terminal.get('stop_pc_requested') is True and
                  terminal.get('block_limit_requested') is False and
                  terminal.get('threads', [{}])[0].get('next_pc') == stop_address)
        sha256 = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
        artifact=os.environ.get('LINX_BF16_ARTIFACT_DIR')
        if artifact:
            dst=Path(artifact);dst.mkdir(parents=True,exist_ok=True)
            for f in self.d.iterdir():
                if f.is_file(): shutil.copy2(f,dst/f.name)
            (dst/'golden.bin').write_bytes(golden);(dst/'model.log').write_text(log)
            (dst/'result.json').write_text(json.dumps({
                'status': 'PASS' if passed else 'FAIL',
                'acceptance': 'numerical memory comparison at explicit post-kernel observation PC',
                'formal_kernel_end': False, 'command': command,
                'exit': exit_code, 'timeout': timed_out, 'cases': len(cases),
                'output_words': len(expected), 'compiler': self.flags[0],
                'compiler_version': subprocess.check_output([self.flags[0], '--version'], text=True),
                'compiler_sha256': sha256(self.flags[0]), 'elf_sha256': sha256(elf),
                'source_sha256': sha256(cpp), 'golden_sha256': hashlib.sha256(golden).hexdigest(),
                'output_sha256': hashlib.sha256(actual).hexdigest(),
                'headers_sha256': {str(p.relative_to(ROOT)): sha256(p) for p in
                    (ROOT/'include/common/pto_packed_bf16.hpp', ROOT/'include/common/pto_packed_bits.hpp',
                     ROOT/'include/jcore/template_asm.hpp')},
                'model': subprocess.check_output(['git','-C',str(model),'rev-parse','HEAD'],text=True).strip(),
                'model_sha256': sha256(model/'bin/gfrun'),
                'model_changes': subprocess.check_output(['git','-C',str(model),'status','--porcelain'],text=True),
                'terminal': terminal}, indent=2))
        self.assertFalse(timed_out, 'model execution timed out: '+log[-2000:])
        self.assertEqual(exit_code,0,log[-6000:])
        self.assertTrue(terminal.get('stop_pc_requested'), terminal)
        self.assertFalse(terminal.get('block_limit_requested'), terminal)
        self.assertEqual(terminal['threads'][0]['next_pc'], stop_address, terminal)

        self.assertTrue(output.exists(),log[-6000:])
        if actual!=golden:
            got=struct.unpack('<'+'I'*(len(actual)//4),actual)
            mismatches=[(i,hex(a),hex(b),cases[i] if i<len(cases) else 'golden') for i,(a,b) in enumerate(zip(got,expected)) if a!=b]
            self.fail(f'output bytes {len(actual)} != golden {len(golden)}; '+str(mismatches[:15]))


if __name__=='__main__': unittest.main()
