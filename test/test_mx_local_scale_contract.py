#!/usr/bin/env python3
"""Pinned Local Matrix-MX scale descriptor contracts, independent of code emission."""
import os
from pathlib import Path
import subprocess
import tempfile
import shutil
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MXScaleContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.host_tree = tempfile.TemporaryDirectory(prefix='pto-mx-scale-host-')
        sys.path.insert(0, str(ROOT/'tools'))
        from check_tileop_usage_examples import host_include_tree
        cls.host_include = Path(cls.host_tree.name)/'include'
        host_include_tree(cls.host_include)

    @classmethod
    def tearDownClass(cls):
        cls.host_tree.cleanup()

    def compile(self, scale_b, message=None, dtype='uint32_t', rows=16, cols=1, e8m0=False):
        atype = '__fp8_e4m3' if e8m0 else '__fp4_hif4x2'
        btype = '__fp8_e5m2' if e8m0 else '__fp4_hif4x2'
        stype = '__fp8_e8m0' if e8m0 else 'uint32_t'
        k = 32 if e8m0 else 64
        if e8m0:
            dtype = stype
        source = f'''#include <common/pto_tileop.hpp>
using namespace pto;
using A = CubeTileM16<{atype},16,{k}>;
using B = CubeTileN8<{btype},{k},16>;
using SA = Tile<Location::Scaling,{stype},32,4,BLayout::CubeM32,16,1>;
using SB = Tile<Location::Scaling,{dtype},32,{max(4,cols)},{scale_b},{rows},{cols}>;
constexpr bool test() {{
  pto_matmul_detail::validate_matrix_scale_contract<FixpAttr::keep_acc(),true,true,SA,A,SB,B>();
  return true;
}}
static_assert(test());
'''
        compiler = os.environ.get('PTO_TARGET_CXX')
        if compiler is None:
            compiler = os.environ.get('CXX') or shutil.which('c++')
            flags = ['-std=c++20','-D__linx','-include',
                     str(ROOT/'test/linx_host_type_shim.hpp'),'-I'+str(self.host_include)]
        else:
            flags = ['--target=linx64v5-unknown-linux-musl',
                     '--sysroot='+os.environ['PTO_TARGET_SYSROOT'],
                     '-resource-dir='+os.environ['PTO_TARGET_RESOURCE_DIR'],
                     '-mlxbc','-fenable-matrix','-std=c++20','-D__linx','-I'+str(ROOT/'include')]
        with tempfile.TemporaryDirectory(prefix='pto-mx-scale-') as work:
            path = Path(work)/'contract.cpp'
            path.write_text(source)
            result = subprocess.run([compiler,*flags,'-fsyntax-only',str(path)],capture_output=True,text=True)
        if message is None:
            self.assertEqual(result.returncode,0,result.stderr)
        else:
            self.assertNotEqual(result.returncode,0)
            self.assertIn(message,result.stderr)

    def test_hif4_local_scale_b(self):
        self.compile('BLayout::CubeM32')

    def test_e8m0_local_scale_b(self):
        self.compile('BLayout::CubeM32', e8m0=True)

    def test_local_row_major_rejected(self):
        self.compile('BLayout::RowMajor','Local MX ScaleB must use CUBE_M32 layout')

    def test_local_transposed_shape_rejected(self):
        self.compile('BLayout::CubeM32','Local MX ScaleB valid shape must be N x ceil(K/groupB)',rows=1,cols=16)

    def test_wrong_scale_carrier_rejected(self):
        self.compile('BLayout::CubeM32','MX ScaleB dtype must match',dtype='float')

    def test_wrong_scale_groups_rejected(self):
        self.compile('BLayout::CubeM32','Local MX ScaleB valid shape must be N x ceil(K/groupB)',cols=2)


if __name__ == '__main__':
    unittest.main()
