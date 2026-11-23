"""Static matrix dimensions stay immediate while cooperative group_M is live."""
import re
import unittest

import test_llvm138_narrow_tcmp as native


class StaticMatmulShapes(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def test_static_and_cooperative_operands(self):
        cpp = self.output / 'matrix.cpp'
        cpp.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using A = Tile<Location::Left, float, 16, 16, BLayout::CubeM16>;
using B = Tile<Location::Right, float, 16, 16, BLayout::CubeN8>;
using S = SharedMatrixRight<float, 16, 16>;
using D = CubeAccumulatorM16<float, 16, 16>;
extern "C" void static_matrix(D &d, A &a, B &b) { TMATMUL(d,a,b); }
extern "C" void cooperative_matrix(D &d, A &a, S &b, size_t groupM) {
  auto shared = TMOV_L2S_INSERT(b);
  TMATMUL(d,a,shared,groupM);
}
''')
        result = self.compile('-S', '-emit-llvm', str(cpp), '-o', '-')
        self.assertEqual(result.returncode, 0, result.stdout)
        bodies = dict(re.findall(
            r'^define [^\n]*@(\w+)\([^\n]*\{\n(.*?)^\}',
            result.stdout, re.MULTILINE | re.DOTALL))
        for name in ('static_matrix', 'cooperative_matrix'):
            body = bodies[name]
            self.assertNotIn('resolve_matmul_shape', body)
            matrix = next(line for line in body.splitlines()
                          if 'asm sideeffect' in line and 'TMATMUL' in line)
            # N/K retain immediate 16 in both forms. A runtime group_M is
            # encoded through the register M operand and its range guard.
            self.assertRegex(matrix, r'i64 16, i64 16,')
            if name == 'cooperative_matrix':
                self.assertRegex(matrix, r'i64 %\w+, i64 16, i64 16,')
        result = self.compile('-c', str(cpp), '-o', str(self.output / 'matrix.o'))
        self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == '__main__':
    unittest.main()
