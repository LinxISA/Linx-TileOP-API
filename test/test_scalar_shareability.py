"""Exercise scope APIs with host bytes and actual target IR/object output."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class ScalarShareability(unittest.TestCase):
    def test_host_bytes_and_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp)/'host'
            subprocess.run(['c++','-std=c++17','-I'+str(ROOT/'include'),
                            str(ROOT/'test/test_scalar_shareability.cpp'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_target_scope_survives_optimization(self):
        compiler = os.environ.get('LINX_CLANG')
        if not compiler:
            self.skipTest('set LINX_CLANG to a compiler implementing #416')
        source = r'''
#include "common/pto_memory.hpp"
using namespace pto;
extern "C" void init(int *output, unsigned char *src, unsigned long n) {
  scoped_memset<MemoryScope::IntraCore>(output, 0xff, n);
  scoped_memcpy<MemoryScope::IntraCore, MemoryScope::Private>(output, src, n);
}
extern "C" int count(int *num, int *hist, int threshold) {
  scalar_store<MemoryScope::IntraCore>(num, 0);
  return scalar_load<MemoryScope::IntraCore>(hist + threshold + 1);
}
extern "C" int alloc(int *num) {
  return scalar_atomic<MemoryScope::IntraCore, AtomicOperation::Add>(num, 1);
}
extern "C" PTO_SCALAR_SHAREABILITY("inter") unsigned long defaults(unsigned long *p) {
  *static_cast<volatile unsigned long *>(p) = 37;
  return *static_cast<volatile unsigned long *>(p);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp); (d/'case.cpp').write_text(source)
            for opt in ['-O0','-O2','-O3']:
                obj=d/(opt[1:]+'.o')
                cmd=[compiler,'--target=linx64v5-unknown-linux-musl','-mcpu=janus','-std=c++17',opt,
                     '-I'+str(ROOT/'include'),'-c',str(d/'case.cpp'),'-o',str(obj)]
                # Standard C++ type_traits is supplied by the target sysroot.
                sysroot=os.environ.get('LINX_SYSROOT')
                if sysroot:
                    cmd.insert(1,'--sysroot='+sysroot)
                    cmd.insert(1,'-isystem'+str(Path(sysroot)/'usr/include/c++/v1'))
                result = subprocess.run(cmd, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                dis=subprocess.check_output([str(Path(compiler).parent/'llvm-objdump'),'-d',
                    '--no-show-raw-insn',str(obj)],text=True)
                for mnemonic in ['sbi.intra','lbui.private','lwui.intra','swi.intra','lw.add.intra','sdi.inter','ldi.inter']:
                    self.assertIn(mnemonic,dis)
                self.assertNotRegex(dis,r'\b(?:memset|memcpy)\b')
                # Inspect optimized IR as well as the final carrier mnemonics.
                ircmd = [x for x in cmd if x != '-c']
                ircmd[ircmd.index('-o') + 1] = str(d/'case.ll')
                ircmd.extend(['-S', '-emit-llvm'])
                result = subprocess.run(ircmd, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                ir = (d/'case.ll').read_text()
                self.assertIn('@llvm.linx.scalar.store', ir)
                self.assertIn('@llvm.linx.scalar.load', ir)
                self.assertIn('@llvm.linx.scalar.atomic', ir)

    def test_target_tile_scalar_interop(self):
        compiler = os.environ.get('LINX_CLANG')
        if not compiler:
            self.skipTest('set LINX_CLANG to a compiler implementing #416')
        source = r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using TileF = Tile<Location::Vec, float, 2, 1024, BLayout::RowMajor, -1, -1>;
using GlobalF = global_tensor<float, RowMajor<-1, -1>>;
extern "C" float mixed(float *input, float *output, int rows, int cols) {
  GlobalF src(input, rows, cols), dst(output, rows, cols);
  input[0] = 1.25f;
  TileF tile(rows, cols);
  TLOAD(tile, src);
  TSTORE(dst, tile);
  // This is a compile/encoding fixture. A real consumer must perform the
  // architecture's required completion and synchronization before this read.
  return output[0];
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d/'mixed.cpp').write_text(source)
            resource = Path(subprocess.check_output([compiler, '-print-resource-dir'], text=True).strip())
            overlay = d/'resource'
            shutil.copytree(resource, overlay, ignore=lambda directory, names:
                            ['tileop-api'] if Path(directory) == resource/'include' else [])
            shutil.copytree(ROOT/'include', overlay/'include/tileop-api')
            cmd = [compiler, '--target=linx64v5-unknown-linux-musl', '-mcpu=janus', '-std=c++20', '-O2', '-mlxbc', '-fenable-matrix', '-DENABLE_TENSOR_INSTR',
                   '-resource-dir', str(overlay), '-I'+str(ROOT/'include'), '-c', str(d/'mixed.cpp'), '-o', str(d/'mixed.o')]
            sysroot = os.environ.get('LINX_SYSROOT')
            if sysroot:
                cmd[1:1] = ['--sysroot='+sysroot,
                            '-isystem'+str(Path(sysroot)/'usr/include/c++/v1')]
            result = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            dis = subprocess.check_output([str(Path(compiler).parent/'llvm-objdump'),
                '-d', '--no-show-raw-insn', str(d/'mixed.o')], text=True)
            for mnemonic in ['swi.intra', 'lwui.intra', 'TLOAD', 'TSTORE']:
                self.assertIn(mnemonic.lower(), dis.lower())

    def test_unsupported_target_compiler_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'bad.cpp'
            p.write_text('#define __linx 1\n#include "common/pto_memory.hpp"\n'
                         'int f(int*p){return pto::scalar_load<pto::MemoryScope::IntraCore>(p);}')
            cxx=shutil.which('clang++') or shutil.which('c++')
            r=subprocess.run([cxx,'-std=c++17','-fsyntax-only','-I'+str(ROOT/'include'),str(p)],capture_output=True,text=True)
            self.assertNotEqual(r.returncode,0)
            self.assertIn('compiler lacks PTO scalar shareability support',r.stderr)

if __name__=='__main__': unittest.main()
