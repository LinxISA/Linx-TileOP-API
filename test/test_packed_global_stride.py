#!/usr/bin/env python3
"""Logical packed-lane strides must become byte row pitches at the GM boundary."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class PackedStrideTest(unittest.TestCase):
    def test_architectural_lane_width(self):
        code = r'''
#include <common/pto_tile.hpp>
#include <common/global_iterator.hpp>
#include <cassert>
using namespace pto;
template<class T> void packed() {
  T values[1024]{};
  global_tensor<T, MatrixLayout<2, 8, 16, 1>> even(values);
  global_tensor<T, MatrixLayout<2, 4, 5, 1>> odd(values);
  assert(even.GetStrideBytes(3)==8);
  assert(odd.GetStrideBytes(3)==3);
  using Part = Tile<Location::Vec,T,32,8,BLayout::CubeN8>;
  using Even = global_tensor<T,RowMajor<64,16>>;
  global_iterator<Even,Part> even_parts(values);
  assert(even_parts(1,1).data()==values+260);
  using Odd = global_tensor<T,RowMajor<64,5>>;
  using Narrow = Tile<Location::Vec,T,32,4,BLayout::RowMajor>;
  global_iterator<Odd,Narrow> odd_parts(values);
  assert(odd_parts(1,1).data()==values+98);
  using Columns = global_tensor<T,MatrixLayout<5,128,1,9>>;
  using ColumnPart = Tile<Location::Vec,T,4,64,BLayout::RowMajor>;
  global_iterator<Columns,ColumnPart> column_parts(values);
  assert(column_parts(0,1).data()==values+320);
}
template<class T> void ordinary() {
  T values[64]{};
  global_tensor<T, MatrixLayout<2, 8, 16, 1>> memory(values);
  assert(memory.GetStride(3)==16);
  assert(memory.GetStrideBytes(3)==16*sizeof(T));
  using Part=Tile<Location::Vec,T,4,4,BLayout::RowMajor>;
  using Rows=global_tensor<T,MatrixLayout<64,16,64,2,LayoutEnum::kRowMajor>>;
  global_iterator<Rows,Part> rows(values);
  assert(rows(0,1).data()==values+8);
  assert(rows(0,1).GetStride(3)==64 && rows(0,1).GetStride(4)==2);
  using Columns=global_tensor<T,MatrixLayout<16,64,2,64,LayoutEnum::kColMajor>>;
  global_iterator<Columns,Part> columns(values);
  assert(columns(1,0).data()==values+8);
  assert(columns(1,0).GetStride(3)==2 && columns(1,0).GetStride(4)==64);
}
int main() {
  packed<__fp4_hif4x2>(); packed<__fp4_e2m1x2>(); packed<__fp4_e1m2x2>();
  packed<__int4x2>(); packed<__uint4x2>();
  ordinary<float>(); ordinary<uint64_t>(); ordinary<uint32_t>(); ordinary<__half>();
  ordinary<__fp6_e3m2>(); ordinary<__fp6_e2m3>();
}
'''
        with tempfile.TemporaryDirectory(prefix='pto-packed-stride-') as work:
            cpp, binary = Path(work)/'test.cpp', Path(work)/'test'
            cpp.write_text(code)
            compiler = os.environ.get('CXX') or shutil.which('c++')
            compiled = subprocess.run([compiler,'-std=c++20','-D__linx','-include',
                str(ROOT/'test/linx_host_type_shim.hpp'),'-I'+str(ROOT/'include'),
                str(cpp),'-o',str(binary)],text=True,capture_output=True)
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            result = subprocess.run([str(binary)],text=True,capture_output=True)
            self.assertEqual(result.returncode,0,result.stderr)


if __name__ == '__main__':
    unittest.main()
