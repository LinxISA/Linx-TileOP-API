"""Regression coverage for the PTO 0.59 TCVT, view and column ARG audit."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import test_llvm138_narrow_tcmp as native

ROOT = Path(__file__).resolve().parents[1]


class HostShapePublication(unittest.TestCase):
    def test_nested_views_publish_to_backing_tile(self):
        # Execute shape publication on the host: compiling a view alone cannot
        # prove the backing Tile receives the new dimensions or preserves data.
        source = r'''
#include <common/pto_tile.hpp>
#include <cassert>
using namespace pto;
template<int R, int C> void check() {
  using T = Tile<Location::Vec, __bf16, 128, 4, BLayout::RowMajor, R, C>;
  T destination{}, source{};
  destination.RowMaskInternal = 1; destination.ColMaskInternal = 1;
  source.RowMaskInternal = 17; source.ColMaskInternal = 3;
  auto view = reinterpret_tile<uint16_t>(destination);
  auto nested = reinterpret_tile<int16_t>(view);
  auto *data = &destination.data();
  nested.SetValidShape(source);
  assert(destination.GetValidRow() == source.GetValidRow());
  assert(destination.GetValidCol() == source.GetValidCol());
  assert(view.GetValidRow() == source.GetValidRow());
  assert(nested.GetValidCol() == source.GetValidCol());
  assert(&nested.data() == data);
  nested.SetValidShape(nested);
  assert(destination.GetValidRow() == source.GetValidRow());
  assert(destination.GetValidCol() == source.GetValidCol());
  if constexpr (R != DYNAMIC) assert(destination.RowMaskInternal == 1);
  if constexpr (C != DYNAMIC) assert(destination.ColMaskInternal == 1);
}
int main() {
  check<DYNAMIC, 4>(); check<128, DYNAMIC>();
  check<DYNAMIC, DYNAMIC>(); check<128, 4>();
}
'''
        with tempfile.TemporaryDirectory(prefix='pto-view-publication-') as work:
            cpp, binary = Path(work) / 'view.cpp', Path(work) / 'view'
            cpp.write_text(source)
            compiler = os.environ.get('CXX') or shutil.which('c++')
            result = subprocess.run([
                compiler, '-std=c++20', '-D__linx', '-include',
                str(ROOT / 'test/linx_host_type_shim.hpp'),
                '-I' + str(ROOT / 'include'), str(cpp), '-o', str(binary),
            ], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run([str(binary)], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)


class NativeTCVTSelectRegression(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def build(self, body, *flags):
        path = self.output / 'regression.cpp'
        path.write_text('#include <common/pto_tileop.hpp>\nusing namespace pto;\n' + body)
        return self.compile(*flags, str(path))

    def test_fp4_and_odd_column_conversions(self):
        # Check both packed formats, round-trip directions, a full-capacity
        # profile and odd widths that cross an allocation boundary.
        body = ''
        for packed in ('__fp4_e2m1x2', '__fp4_e1m2x2'):
            for rows, cols in ((128, 32), (8, 3), (128, 1), (256, 1)):
                tag = packed[2:] + f'_{rows}_{cols}'
                body += f'''
using S_{tag} = Tile<Location::Vec, float, {rows}, {cols}>;
using D_{tag} = Tile<Location::Vec, {packed}, {rows}, {cols}>;
void cvt_{tag}(S_{tag} &s, D_{tag} &d) {{ TCVT(d,s); TCVT(s,d); }}
'''
        body += '''
using S = Tile<Location::Vec, float, 8, 3>;
using D = Tile<Location::Vec, __half, 8, 3>;
void odd(S &s, D &d) { TCVT(d,s); TCVT(s,d); }
using P = Tile<Location::Vec, __fp4_e2m1x2, 128, 1>;
static_assert(P::StorageBytes == 128);
using Q = Tile<Location::Vec, __fp4_e2m1x2, 128, 3>;
static_assert(Q::StorageBytes == 256);
'''
        result = self.build(body, '-fsyntax-only')
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_full_capacity_fp4_emits_objects(self):
        result = self.build('''
using S = Tile<Location::Vec, float, 128, 32>;
using D = Tile<Location::Vec, __fp4_e2m1x2, 128, 32>;
void narrow(S &s, D &d) { TCVT(d,s); }
void widen(S &s, D &d) { TCVT(s,d); }
''', '-c', '-o', str(self.output / 'packed.o'))
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_legacy_physical_row_mismatch_remains_illegal(self):
        result = self.build('''
using S = Tile<Location::Vec, float, 64, 1>;
using D = Tile<Location::Vec, __fp8_e8m0, 64, 1>;
void mismatch(S &s, D &d) { TCVT(d,s); }
''', '-fsyntax-only')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('physical Rows', result.stdout)

    def test_odd_physical_row_mismatch_is_illegal(self):
        result = self.build('''
using S = Tile<Location::Vec, float, 8, 3, BLayout::RowMajor, 4, 3>;
using D = Tile<Location::Vec, __half, 16, 3, BLayout::RowMajor, 4, 3>;
void mismatch(S &s, D &d) { TCVT(d,s); }
''', '-fsyntax-only')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('physical Rows', result.stdout)

    def test_physical_column_mismatch_is_illegal(self):
        result = self.build('''
using S = Tile<Location::Vec, float, 8, 3, BLayout::RowMajor, 8, 3>;
using D = Tile<Location::Vec, __half, 8, 5, BLayout::RowMajor, 8, 3>;
void mismatch(S &s, D &d) { TCVT(d,s); }
''', '-fsyntax-only')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('physical columns must match', result.stdout)

    def test_non_power_of_two_integer_columns_are_illegal(self):
        result = self.build('''
using S = Tile<Location::Vec, float, 8, 3>;
using D = Tile<Location::Vec, uint32_t, 8, 3>;
void forbidden(S &s, D &d) { TCVT(d,s); }
''', '-fsyntax-only')
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn('non-power-of-two columns require', result.stdout)

    def test_dynamic_reinterpret_select_public_api(self):
        body = ''
        for row, col in ((-1, 4), (128, -1), (-1, -1)):
            tag = f'{abs(row)}_{abs(col)}'
            body += f'''
using T_{tag} = Tile<Location::Vec, __bf16, 128, 4, BLayout::RowMajor, {row}, {col}>;
using P_{tag} = Tile<Location::Vec, uint8_t, 128, 4, BLayout::RowMajor, {row}, {col}>;
void select_{tag}(T_{tag} &d, T_{tag} &a, T_{tag} &b, const P_{tag} &p) {{
  auto dv = reinterpret_tile<uint16_t>(d);
  auto av = reinterpret_tile<uint16_t>(a);
  auto bv = reinterpret_tile<uint16_t>(b);
  TSEL(dv,p,av,bv); TSEL(dv,p,av);
  auto nested = reinterpret_tile<int16_t>(dv);
  TSEL(nested,p,av,bv);
}}
'''
        result = self.build(body, '-fsyntax-only')
        self.assertEqual(result.returncode, 0, result.stdout)

    def test_hif4_tcvt_rejects_source_destination_and_identity(self):
        for src, dst in (('__fp4_hif4x2', 'float'),
                         ('float', '__fp4_hif4x2'),
                         ('__fp4_hif4x2', '__fp4_hif4x2')):
            with self.subTest(src=src, dst=dst):
                result = self.build(f'''
using S = Tile<Location::Vec, {src}, 128, 32>;
using D = Tile<Location::Vec, {dst}, 128, 32>;
void forbidden(S &s, D &d) {{ TCVT(d,s); }}
''', '-fsyntax-only')
                self.assertNotEqual(result.returncode, 0, result.stdout)
                self.assertIn('Illegal TCVT datatype pair: HiF4X2', result.stdout)

    def test_column_arg_requires_u32(self):
        for op in ('TCOLARGMAX', 'TCOLARGMIN'):
            for dtype in ('uint32_t', 'float', '__half'):
                with self.subTest(op=op, dtype=dtype):
                    result = self.build(f'''
using S = Tile<Location::Vec, __half, 16, 16>;
using D = Tile<Location::Vec, {dtype}, 1, 16>;
void reduce(S &s, D &d) {{ {op}(d,s); }}
''', '-fsyntax-only')
                    if dtype == 'uint32_t':
                        self.assertEqual(result.returncode, 0, result.stdout)
                    else:
                        self.assertNotEqual(result.returncode, 0, result.stdout)
                        self.assertIn(op + ' destination must use U32 indices', result.stdout)


if __name__ == '__main__':
    unittest.main()
