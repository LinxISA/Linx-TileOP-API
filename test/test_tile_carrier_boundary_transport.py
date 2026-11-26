"""Check real API copies, BF16, and runtime valid-shape carrier boundaries."""
import re
import subprocess
import unittest
from pathlib import Path

import test_llvm138_narrow_tcmp as native


class TileCarrierBoundaryTransport(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def test_reinterpret_back_to_float_across_call(self):
        source = self.output / "reinterpret_convert.cpp"
        source.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
extern "C" __attribute__((noinline)) void opaque();
using F=Tile<Location::Vec,float,128,1,BLayout::RowMajor>;
using B=Tile<Location::Vec,__bf16,128,1,BLayout::RowMajor>;
extern "C" void roundtrip(B &dst, F &src) {
  F f=src;
  auto u=reinterpret_tile<uint32_t>(f);
  TANDS(u,u,uint32_t(0x7f800000));
  B b; TCVT(b,f);
  opaque();
  TMULS(dst,b,__builtin_bit_cast(__bf16,uint16_t(0x3f80)));
}
''')
        result=self.compile('-mlinx-runtime-intra-abi','-c',str(source),
                            '-o',str(self.output/'reinterpret_convert.o'))
        self.assertEqual(result.returncode,0,result.stdout)

    def test_dynamic_predicate_stays_in_registers(self):
        source=self.output/'dynamic_predicate.cpp'
        source.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using U=Tile<Location::Vec,uint16_t,1,128,BLayout::RowMajor,1,-1>;
extern "C" void dynamic_select(U &dst,U &a,U &b,int n) {
  U mask(n);
  TCMPS<CmpMode::EQ>(mask,a,uint16_t(0));
  TSEL(dst,mask,a,b);
}
using C=VecTileM32<uint16_t,32,2,32,1>;
extern "C" void cube_compare(C &dst,C &a) {
  TCMPS<CmpMode::EQ>(dst,a,uint16_t(0));
}
''')
        result=self.compile('-mlinx-runtime-intra-abi','-S','-emit-llvm',str(source),'-o','-')
        self.assertEqual(result.returncode,0,result.stdout)
        body=result.stdout.split('@dynamic_select(',1)[1].split('\n}',1)[0]
        # No predicate may be serialized as an ordinary numeric carrier.
        self.assertNotRegex(body,r'\b(?:load|store) <\d+ x i32>')
        compare=re.search(r'(%[\w.]+) = (?:tail )?call [^\n]*BSTART.TEPL 45[^\n]*',body)
        self.assertIsNotNone(compare,body)
        select=next(l for l in body.splitlines() if 'BSTART.TEPL TSEL' in l)
        self.assertIn(compare.group(1),select)
        cube=result.stdout.split('@cube_compare(',1)[1].split('\n}',1)[0]
        comparison=next(l for l in cube.splitlines() if 'BSTART.TEPL 45' in l)
        self.assertNotIn('B.DATR CUBE_',comparison)
        self.assertIn('B.DATR Zero, EQ',comparison)
        result=self.compile('-mlinx-runtime-intra-abi','-c',str(source),'-o',str(self.output/'dynamic_predicate.o'))
        self.assertEqual(result.returncode,0,result.stdout)

    def test_typed_boundaries(self):
        source = self.output / "boundaries.cpp"
        source.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using F=VecTileM32<float,32,1>;
using B=VecTileM32<__bf16,32,2,32,1>;
using D=Tile<Location::Vec,_Float16,32,128,BLayout::RowMajor,32,-1>;
using U=Tile<Location::Vec,uint16_t,1,128,BLayout::RowMajor,1,-1>;
extern "C" void copy(F &dst,F &a,F &b) { F c=b; TMAX(dst,a,c); }
extern "C" void bf16(B &dst,B &a,B &b) { TMAX(dst,a,b); }
extern "C" void dynamic(D &dst,D &src) { TABS(dst,src); }
extern "C" void reinterpreted(U &dst,U &src,uint16_t s) {
  auto d=reinterpret_tile<int16_t>(dst);
  auto a=reinterpret_tile<int16_t>(src);
  TANDS(d,a,static_cast<int16_t>(s));
}
''')
        result = self.compile("-S", "-emit-llvm", str(source), "-o", "-")
        self.assertEqual(result.returncode, 0, result.stdout)
        bodies=dict(re.findall(r'^define [^\n]*@(\w+)\([^\n]*\{\n(.*?)^\}',
                               result.stdout,re.MULTILINE|re.DOTALL))
        for name in ['copy','bf16','dynamic','reinterpreted']:
            with self.subTest(name=name):
                self.assertNotRegex(bodies[name],r'\b(?:load|store) <\d+ x i32>')
                self.assertNotIn('llvm.memcpy',bodies[name])
                self.assertNotRegex(bodies[name],r'call [^\n]*@_ZN.*(?:data|GetValid|reinterpret)')
        self.assertEqual(bodies['copy'].count('@llvm.linx.blk.tload'),2)
        self.assertIn('i64 1, i64 32, i64 1, i64 5, i64 0, i64 21,',bodies['bf16'])
        self.assertRegex(bodies['dynamic'],r'@llvm.linx.blk.tload[^\n]*i64 %[^,]+, i64 32, i64 128, i64 4, i64 0, i64 0,')
        self.assertIn('i64 1, i64 128, i64 18, i64 0, i64 0,',bodies['reinterpreted'])
        obj=self.output/'boundaries.o'
        result=self.compile('-c',str(source),'-o',str(obj))
        self.assertEqual(result.returncode,0,result.stdout)
        dis=subprocess.check_output([str(Path(self.flags[0]).parent/'llvm-objdump'),
                                     '-d','--disassembler-options=no-tile-macros',str(obj)],text=True)
        self.assertNotRegex(dis,r'BSTART\.TLOAD\s+S32')
        self.assertIn('ND2M32',dis)
        self.assertIn('TABS',dis)


if __name__=='__main__':
    unittest.main()
