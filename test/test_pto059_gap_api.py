"""Native object/diagnostic coverage for the 0.59 API gaps."""
import re
import subprocess
import unittest
from pathlib import Path
import test_llvm138_narrow_tcmp as native


class PTO059GapAPI(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def source(self, body):
        path = self.output / 'gap.cpp'
        path.write_text('#include <common/pto_tileop.hpp>\nusing namespace pto;\n' + body)
        return str(path)

    def test_tlea_objects_and_bundle(self):
        for src, dst, layout, rows in [('int32_t', 'int64_t', 'RowMajor', 1),
                                       ('uint32_t', 'uint64_t', 'CubeM32', 32),
                                       ('int64_t', 'int64_t', 'CubeM32', 32),
                                       ('uint64_t', 'uint64_t', 'RowMajor', 1)]:
            with self.subTest(src=src, layout=layout):
                source = self.source(f'''
using S = Tile<Location::Vec,{src},{rows},4,BLayout::{layout}>;
using D = Tile<Location::Vec,{dst},{rows},4,BLayout::{layout}>;
void convert(D &d, S &s, unsigned width) {{ TLEA(d,s,width); }}
''')
                obj = self.output / 'tlea.o'
                result = self.compile('-c', source, '-o', str(obj))
                self.assertEqual(result.returncode, 0, result.stdout)
                asm = subprocess.check_output([str(Path(self.flags[0]).parent / 'llvm-objdump'),
                    '-d', '--disassembler-options=no-tile-macros', str(obj)], text=True)
                self.assertRegex(asm, r'BSTART.TEPL\s+TLEA')
                self.assertIn('B.IOR', asm)
                if layout == 'CubeM32':
                    self.assertIn('CUBE_M32', asm)

    def test_tlea_dynamic_shapes_and_physical_columns(self):
        source = self.source("""
template<int R,int C> using S=Tile<Location::Vec,int32_t,8,8,BLayout::RowMajor,R,C>;
template<int R,int C> using D=Tile<Location::Vec,int64_t,8,16,BLayout::RowMajor,R,C>;
void sd(D<DYNAMIC,4>&d,S<DYNAMIC,4>&s){TLEA(d,s,16);}
void ds(D<4,DYNAMIC>&d,S<4,DYNAMIC>&s){TLEA(d,s,32);}
void dd(D<DYNAMIC,DYNAMIC>&d,S<DYNAMIC,DYNAMIC>&s){TLEA(d,s,64);}
""")
        result = self.compile('-c', source, '-o', str(self.output/'tlea-dynamic.o'))
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_tlea_rejects_illegal_types_and_layouts(self):
        for src,dst,layout,diagnostic in [
                ('float','uint64_t','RowMajor','source index tile'),
                ('int32_t','uint64_t','RowMajor','signedness'),
                ('uint32_t','uint32_t','RowMajor','destination must be'),
                ('int32_t','int64_t','CubeN8','destination layout')]:
            with self.subTest(src=src,dst=dst,layout=layout):
                source = self.source(f'''
using S = Tile<Location::Vec,{src},32,8,BLayout::{layout}>;
using D = Tile<Location::Vec,{dst},32,8,BLayout::{layout}>;
void convert(D &d, S &s) {{ TLEA(d,s,32); }}
''')
                result = self.compile('-fsyntax-only',source)
                self.assertNotEqual(result.returncode,0,result.stdout)
                self.assertIn(diagnostic,result.stdout)

    def test_tci64_objects(self):
        for dtype in ('int64_t','uint64_t'):
            source = self.source(f'''
void generate({dtype} start) {{
 Tile<Location::Vec,{dtype},1,16,BLayout::RowMajor> row;
 Tile<Location::Left,{dtype},32,4,BLayout::CubeM32> cube;
 TCI(row,start); TCI<decltype(row),{dtype},1>(row,start);
 TCI_ROW(cube,start); TCI_COL(cube,start);
}}
''')
            result = self.compile('-c',source,'-o',str(self.output/'tci.o'))
            self.assertEqual(result.returncode,0,result.stdout)

    def test_broadcast_valid_slot(self):
        for valid_columns, success in ((1,False),(2,True)):
            source = self.source(f'''
using D = Tile<Location::Vec,__half,32,8,BLayout::CubeM32>;
using S = Tile<Location::Vec,__half,32,2,BLayout::CubeM32,32,{valid_columns}>;
void expand(D &d,S &s) {{ TROWEXPAND<D,S,2>(d,s); }}
''')
            result = self.compile('-fsyntax-only',source)
            self.assertEqual(result.returncode==0,success,result.stdout)
            if not success:self.assertIn('slot must be within valid columns',result.stdout)

    def test_range_compiler_capability_gate(self):
        source = self.source("void ordinary() {}")
        result = self.compile('-U__linx_indexed_tlsu_range', '-fsyntax-only', source)
        self.assertEqual(result.returncode, 0, result.stdout)
        source = self.source("""
using V=VecTileM32<uint32_t,32,4>;
using I=VecTileM32<uint64_t,32,4>;
void bounded(V &v,I &i,global_tensor<uint32_t,RowMajor<32,4>>&g) {
 MGATHER(v,g,i,IndexedByteRange{0,0});
}
""")
        result = self.compile('-U__linx_indexed_tlsu_range','-fsyntax-only',source)
        self.assertNotEqual(result.returncode,0,result.stdout)
        self.assertIn('requires LLVM b19e8c0e184c range support', result.stdout)

    def test_tlea_rejects_backing_type_reinterpretation(self):
        source = self.source("""
using S=Tile<Location::Vec,float,1,4,BLayout::RowMajor>;
using D=Tile<Location::Vec,int64_t,1,4,BLayout::RowMajor>;
void convert(D &d,S &s) { auto index=reinterpret_tile<int32_t>(s); TLEA(d,index,32); }
""")
        result=self.compile('-fsyntax-only',source)
        self.assertNotEqual(result.returncode,0,result.stdout)
        self.assertIn('operation dtype must equal the backing dtype',result.stdout)


if __name__ == '__main__':
    unittest.main()
