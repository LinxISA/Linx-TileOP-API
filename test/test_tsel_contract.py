"""Native TSEL carrier schemas and diagnostics against PTO d0884b82."""
import re
import subprocess
import unittest
from pathlib import Path
import test_llvm138_narrow_tcmp as native


class TSELContractTest(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def build(self, body, syntax=False, ir=False):
        source = self.output / 'tsel.cpp'
        source.write_text('#include <common/pto_tileop.hpp>\nusing namespace pto;\n' + body)
        flags = ['-S', '-emit-llvm', '-o', '-'] if ir else (
            ['-fsyntax-only'] if syntax else ['-c', '-o', str(self.output / 'tsel.o')])
        return self.compile(*flags, str(source))

    def test_tile_sources_static_dynamic_and_aliases(self):
        bodies = []
        for rows, cols in ((4, 8), (-1, 8), (4, -1), (-1, -1)):
            tag = f'{abs(rows)}_{abs(cols)}'
            bodies.append(f'''
using D{tag}=Tile<Location::Vec,float,4,8,BLayout::RowMajor,{rows},{cols}>;
using T{tag}=Tile<Location::Vec,int32_t,4,8,BLayout::RowMajor,{rows},{cols}>;
using F{tag}=Tile<Location::Vec,uint32_t,4,8,BLayout::RowMajor,{rows},{cols}>;
using P{tag}=Tile<Location::Vec,uint8_t,4,8,BLayout::RowMajor,{rows},{cols}>;
void select_{tag}(D{tag}&d,P{tag}&p,const T{tag}&t,const F{tag}&f) {{ TSEL(d,p,t,f); }}
void alias_{tag}(D{tag}&d,P{tag}&p) {{ TSEL(d,p,d,d); TSEL(d,p,d); }}
''')
        result = self.build(''.join(bodies))
        self.assertEqual(result.returncode, 0, result.stdout)
        text = self.disassemble()
        self.assertIn('TSEL', text)
        self.assertNotIn('TCVT', text)
        bundles = self.select_bundles(text)
        self.assertEqual(len(bundles), 12)
        for bundle in bundles:
            self.assertEqual(bundle.count('B.IOT'), 2, bundle)
            self.assertNotIn('B.IOR', bundle)

    def disassemble(self):
        return subprocess.check_output([str(Path(self.flags[0]).parent / 'llvm-objdump'),
            '-d', '--disassembler-options=no-tile-macros', str(self.output / 'tsel.o')], text=True)

    @staticmethod
    def select_bundles(text):
        return re.findall(r'BSTART\.TEPL\s+TSEL[^\n]*\n(?:(?![^\n]*BSTART)[^\n]*\n)*', text)

    def test_typed_predicate_independent_basis_and_mixed_sources(self):
        result = self.build('''
using B=VecTileM32<uint32_t,32,4>;
using D=VecTileM32<float,32,4>;
using T=VecTileM32<int32_t,32,4>;
using F=VecTileM32<uint32_t,32,4>;
void select(D&d,B&b,const T&t,const F&f) {
 auto p=predicate::compare_scalar<CmpMode::EQ>(b,uint32_t(0));
 TSEL(d,p,t,f); predicate::select(d,p,t,f); TSEL(d,p,d);
 VecTileM32<uint16_t,32,4> narrow;
 TSEL(narrow,p,narrow,narrow);
}
''')
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertNotIn('TCVT', self.disassemble())

    def test_gpr_schemas_and_supported_widths(self):
        bodies = []
        for layout, rows in (('CubeM16', 16), ('CubeM32', 32)):
            for dtype, bits in (('uint8_t',8), ('int8_t',8), ('uint16_t',16),
                                ('__half',16), ('float',32), ('uint64_t',64)):
                if rows == 16 and bits == 64:
                    continue
                cols = 4 if rows == 16 and bits != 32 else 2
                tag = layout + dtype.replace('_','')
                mask_type = 'predicate::GPRMask' if bits == 8 else 'uint64_t'
                bodies.append(f'''
using D{tag}=Tile<Location::Vec,{dtype},{rows},{cols},BLayout::{layout}>;
void select_{tag}(D{tag}&d,const D{tag}&t,const D{tag}&f,{mask_type} p) {{ TSEL(d,p,t,f); }}
''')
        result = self.build(''.join(bodies))
        self.assertEqual(result.returncode, 0, result.stdout)
        text = self.disassemble()
        self.assertEqual(text.count('TSEL'), 11)
        bundles = self.select_bundles(text)
        self.assertEqual(len(bundles), 11)
        for bundle in bundles:
            self.assertEqual(bundle.count('B.IOT'), 1, bundle)
            self.assertEqual(bundle.count('B.IOR'), 1, bundle)
            # CUBE is inferred from the source descriptor. TSEL may not
            # encode a nonzero B.DATR Layout even in the GPR mask form.
            self.assertNotRegex(bundle, r'B\.DATR[^\n]*CUBE_M(?:16|32)')
        # Complete source-only B.IOR: no destination GPR, two words only
        # for the four 8-bit cases (two layouts, S8/U8).
        two_words = sum(bool(re.search(r'B.IOR\s+\[[^\]]*,[^\]]*\], \[\]', b))
                        for b in bundles)
        self.assertEqual(two_words, 4)
        self.assertNotIn('TCVT', text)

    def test_gpr_mixed_carriers_and_dynamic_shape(self):
        result = self.build('''
using D=Tile<Location::Vec,float,32,2,BLayout::CubeM32,DYNAMIC,DYNAMIC>;
using T=Tile<Location::Vec,int32_t,32,2,BLayout::CubeM32,DYNAMIC,DYNAMIC>;
using F=Tile<Location::Vec,uint32_t,32,2,BLayout::CubeM32,DYNAMIC,DYNAMIC>;
void mixed(D&d,const T&t,const F&f,uint64_t mask) {TSEL(d,mask,t,f);}
using U=VecTileM32<uint8_t,32,4>;
void alias(U&d,predicate::GPRMask mask) {TSEL(d,mask,d);}
''')
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertNotIn('TCVT', self.disassemble())

    def test_full_numeric_domain_and_rowmajor_raw_carriers(self):
        bodies = []
        types = ('double', 'float', '__tf32', '__hf32', '__half', '__bf16',
                 '__fp8_e4m3', '__fp8_e5m2', 'int64_t', 'int32_t',
                 'int16_t', 'int8_t', 'uint64_t', 'uint32_t', 'uint16_t', 'uint8_t')
        for i, dtype in enumerate(types):
            for layout, rows in (('RowMajor', 1), ('CubeM16', 16), ('CubeM32', 32)):
                if layout == 'CubeM16' and dtype in ('double', 'int64_t', 'uint64_t'):
                    continue
                tag = f'{i}_{layout}'
                bodies.append(f'''
using D{tag}=Tile<Location::Vec,{dtype},{rows},4,BLayout::{layout}>;
using P{tag}=Tile<Location::Vec,uint8_t,{rows},4,BLayout::{layout}>;
void f{tag}(D{tag}&d,P{tag}&p,const D{tag}&t,const D{tag}&f) {{TSEL(d,p,t,f);}}
''')
        bodies.append('''
using D=Tile<Location::Vec,uint8_t,1,4>;
template<typename T> using Raw=Tile<Location::Vec,T,1,4>;
void raw(D&d,D&p,Raw<__hif8>&a,Raw<__fp6_e3m2>&b,Raw<__fp8_e8m0>&c) {
 TSEL(d,p,a,b); TSEL(d,p,c,a);
}
''')
        result = self.build(''.join(bodies))
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_independent_output_and_dynamic_shape_publication(self):
        result = self.build('''
using D=Tile<Location::Vec,float,4,8,BLayout::RowMajor,DYNAMIC,DYNAMIC>;
extern "C" void select(D&d,const D&p,const D&t,const D&f) {TSEL(d,p,t,f);}
''', ir=True)
        self.assertEqual(result.returncode, 0, result.stdout)
        operations = [line for line in result.stdout.splitlines() if 'asm sideeffect' in line]
        self.assertEqual(len(operations), 1)
        self.assertNotIn('undef', operations[0])
        self.assertNotIn('poison', operations[0])
        # Three input Tile loads (mask/true/false), no old destination load;
        # payload plus both runtime dimensions are published to the result.
        loads = len(re.findall(r' = load <', result.stdout))
        transports = len(re.findall(r' = (?:tail )?call <[^\n]*@llvm\.linx\.blk\.tload', result.stdout))
        self.assertEqual(loads + transports, 3)
        shape_stores = re.findall(
            r'call void @llvm\.linx\.scalar\.store\(ptr ([^,]+), i64 [^,]+, i32 4, i32 1\)',
            result.stdout)
        self.assertEqual(len(shape_stores), 2, result.stdout)
        self.assertEqual(len(set(shape_stores)), 2, result.stdout)

    def test_illegal_numeric_sources_and_gpr_shapes(self):
        for body, diagnostic in (
            ('using F=VecTileM32<uint16_t,32,4>; void f(D&d,P&p,D&t,F&f){TSEL(d,p,t,f);}', 'numeric sources'),
            ('using F=VecTileM32<float,32,8>; void f(D&d,P&p,D&t,F&f){TSEL(d,p,t,f);}', 'physical shapes'),
            ('void f(D&d,D&t){TSEL(d,uint64_t(1),t,t);}', 'GPR mask valid shape'),
            ('using U=VecTileM32<uint8_t,32,4>; void f(U&d,U&t){TSEL(d,uint64_t(1),t,t);}', 'two GPR mask words'),
            ('void f(D&d,D&t){TSEL(d,predicate::GPRMask{1,0},t,t);}', 'two GPR mask words'),
            ('using U=VecTileM32<__hif8,32,4>; void f(U&d,P&p,U&t){TSEL(d,p,t,t);}', 'unsupported operation'),
            ('using U=VecTileM32<__hif8,32,4>; void f(P&d,P&p,U&t){auto v=reinterpret_tile<uint8_t>(t); TSEL(d,p,v,v);}', 'numeric sources'),
            ('using U=VecTileM32<__uint4x2,32,4>; void f(P&d,P&p,U&t){TSEL(d,p,t,t);}', 'numeric sources'),
        ):
            with self.subTest(diagnostic=diagnostic):
                result = self.build('using D=VecTileM32<float,32,4>; using P=VecTileM32<uint8_t,32,4>;\n'+body, syntax=True)
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn(diagnostic, result.stdout)


if __name__ == '__main__':
    unittest.main()
