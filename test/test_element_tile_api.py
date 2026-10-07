#!/usr/bin/env python3
"""Executable public ElementTile/partition contracts, using the existing shim."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ElementTileAPITest(unittest.TestCase):
    def compile(self, code, *, run=False, accepts=True, release=False, runtime_reject=False):
        compiler = os.environ.get('CXX') or shutil.which('c++')
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(prefix='public-element-tile-') as directory:
            source = Path(directory) / 'contract.cpp'
            output = Path(directory) / 'contract'
            source.write_text('#include <common/pto_tile.hpp>\n#include <common/pto_tile_region.hpp>\n' + code)
            command = [compiler, '-std=c++20', '-D__linx',
                       '-include', str(ROOT / 'test/linx_host_type_shim.hpp'),
                       '-I', str(ROOT / 'include'), str(source)]
            if release:
                command += ['-DNDEBUG']
            command += ['-o', str(output)] if run else ['-fsyntax-only']
            result = subprocess.run(command, text=True, capture_output=True)
            if accepts:
                self.assertEqual(result.returncode, 0, result.stderr)
                if run:
                    executed = subprocess.run([str(output)], capture_output=True)
                    if runtime_reject:
                        self.assertNotEqual(executed.returncode, 0, 'invalid runtime range accepted')
                    else:
                        self.assertEqual(executed.returncode, 0, executed.stderr)
            else:
                self.assertNotEqual(result.returncode, 0, 'invalid profile compiled')

    def test_type_and_logical_partition(self):
        self.compile(r'''
#include <cassert>
using namespace pto;
static_assert(ElementTile<uint32_t,32>::Numel == 32);
static_assert(ElementTile<uint32_t,128>::Numel == 128);
static_assert(is_element_tile_v<ElementTile<uint32_t,32>>);
template<class T> concept HasElements = requires(T& tile) { TPARTELEMENT(tile); };
static_assert(HasElements<ElementTile<uint32_t,32>>);
static_assert(!HasElements<ElementTile<uint32_t,128>>);
static_assert(!HasElements<CubeTileM32<uint32_t,32,1>>);
static_assert(!HasElements<CubeTileN8<unsigned long,2,8>>);
int main() {
  ElementTile<uint32_t,32> element_tile;
  auto& element_view=TPARTELEMENT(element_tile);
  auto& original_carrier=element_tile.data();
  static_assert(std::is_same_v<decltype(element_view),decltype(original_carrier)>);
  assert(&element_view==&original_carrier);
  ElementTile<uint32_t,128> parent;
  uint32_t output[128]{};
  for (unsigned valid=0; valid<=128; ++valid) {
    auto parts=TPARTVIEW<32>(parent,valid);
    assert(parts.size()==4);
    unsigned sum=0;
    for(unsigned part=0;part<parts.size();++part) {
      unsigned expected=0;
      for(unsigned element=part;element<valid;element+=4) ++expected;
      assert(parts.valid_size(part)==expected);
      sum+=parts.valid_size(part);
      auto memory=parts.logical_region(output,part);
      assert(memory.data()==output+part);
      assert(memory.GetStrideBytes(3)==4*sizeof(uint32_t));
    }
    assert(sum==valid);
  }
  // The original two-dimensional public overload still works.
  auto old_parts=TPARTVIEW<ElementTile<uint32_t,32>,1,4>(parent);
  auto old_view=old_parts[0][3];
  (void)old_view;
}
''', run=True)

    def test_element_view_compiler_metadata(self):
        marker = 'pto.element.view:v1;dtype=u32;rows=32;cols=1;layout=cube_m32'
        header = (ROOT / 'include/common/pto_tile_region.hpp').read_text()
        self.assertIn('#if defined(__clang__) && defined(__linx)', header)
        self.assertIn(f'annotate("{marker}")', header)

        compiler = shutil.which('clang++')
        if compiler is None:
            self.skipTest('clang++ is required for the AnnotateAttr AST check')
        with tempfile.TemporaryDirectory(prefix='element-view-metadata-') as directory:
            source = Path(directory) / 'metadata.cpp'
            source.write_text(r'''
#include <common/pto_tile.hpp>
#include <common/pto_tile_region.hpp>
void instantiate(pto::ElementTile<uint32_t,32>& tile) {
  auto& elements=pto::TPARTELEMENT(tile);
  (void)elements;
}
''')
            result = subprocess.run([
                compiler, '-std=c++20', '-D__linx',
                '-include', str(ROOT / 'test/linx_host_type_shim.hpp'),
                '-I', str(ROOT / 'include'), '-Xclang', '-ast-dump',
                '-fsyntax-only', str(source),
            ], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('TPARTELEMENT', result.stdout)
            self.assertIn(f'AnnotateAttr', result.stdout)
            self.assertIn(f'"{marker}"', result.stdout)

    def test_native_pointer_transport_surface(self):
        compiler = os.environ.get('LINX_CXX')
        resource = os.environ.get('LINX_RESOURCE_DIR')
        sysroot = os.environ.get('LINX_SYSROOT')
        if not (compiler and resource and sysroot):
            self.skipTest('set LINX_CXX/LINX_RESOURCE_DIR/LINX_SYSROOT for native transport')
        code = r"""
#include <common/pto_tileop.hpp>
using namespace pto;
void transport(const uint32_t* input, uint32_t* output, unsigned valid) {
  ElementTile<uint32_t,128> parent;
  TLOAD(parent,input,valid);
  auto parts=TPARTVIEW<32>(parent,valid);
  auto view=parts.part(0);
  ElementTile<uint32_t,32> values;
  TADDS(values,view,0u);
  auto& elements=TPARTELEMENT(values);
  (void)elements;
  TSTORE(output,values,parts,0);
}
"""
        with tempfile.TemporaryDirectory(prefix='native-element-tile-') as directory:
            source=Path(directory)/'native.cpp'
            source.write_text(code)
            result=subprocess.run([compiler,'-std=c++20','-O2',
                '--target=linx64v5-unknown-linux-musl','-D__linx','-mlxbc',
                '-fenable-matrix','-mllvm','-enable-all-vector-as-tilereg=true',
                '-resource-dir='+resource,'--sysroot='+sysroot,
                '-I',str(ROOT/'include'),'-c',str(source),
                '-o',str(Path(directory)/'native.o')],text=True,capture_output=True)
            self.assertEqual(result.returncode,0,result.stderr)
            # O0 must still inline the logical accessor and its native storage
            # getter, so required region analysis can see the actual carrier.
            ir = Path(directory) / 'native.o0.ll'
            unoptimized = subprocess.run([
                compiler, '-std=c++20', '-O0',
                '--target=linx64v5-unknown-linux-musl', '-D__linx', '-mlxbc',
                '-fenable-matrix', '-mllvm', '-enable-all-vector-as-tilereg=true',
                '-resource-dir='+resource, '--sysroot='+sysroot,
                '-I', str(ROOT/'include'), '-S', '-emit-llvm', str(source),
                '-o', str(ir),
            ], text=True, capture_output=True)
            self.assertEqual(unoptimized.returncode, 0, unoptimized.stderr)
            ir_text = ir.read_text()
            accessor_call = re.search(
                r'^.*\bcall\b[^\n]*@(?:[^\s(]*TPARTELEMENT|'
                r'_ZNK?3pto4Tile[^\s(]*4data)[^\n]*$', ir_text, re.M)
            self.assertIsNone(accessor_call,
                              accessor_call.group(0) if accessor_call else '')

    def test_release_rejects_bad_ranges(self):
        for operation in (
            'auto parts=pto::TPARTVIEW<32>(parent,129); (void)parts;',
            'auto parts=pto::TPARTVIEW<32>(parent,128); (void)parts.part(4);',
            'auto parts=pto::TPARTVIEW<32>(parent,128); (void)parts.valid_size(4);',
            'auto parts=pto::TPARTVIEW<32>(parent,128); (void)parts.logical_region(output,4);'):
            with self.subTest(operation=operation):
                self.compile('int main() { pto::ElementTile<uint32_t,128> parent; '
                    'uint32_t output[128]{}; '+operation+' return 0; }',
                    run=True, release=True, runtime_reject=True)

    def test_reject_unsupported_dtype(self):
        self.compile('pto::ElementTile<unsigned long,32> unsupported;\n', accepts=False)

    def test_reject_unsupported_capacity(self):
        self.compile('pto::ElementTile<uint32_t,64> unsupported;\n', accepts=False)

    def test_reject_wrong_part_size(self):
        self.compile('void bad(pto::ElementTile<uint32_t,128>& tile) { auto parts=pto::TPARTVIEW<64>(tile,128); }\n', accepts=False)

    def test_reject_parent_element_indexing(self):
        self.compile('void bad(pto::ElementTile<uint32_t,128>& tile) { auto& elements=pto::TPARTELEMENT(tile); }\n', accepts=False)


if __name__ == '__main__':
    unittest.main()
