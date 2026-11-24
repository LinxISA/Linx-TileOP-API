"""Static matrix dimensions stay immediate while cooperative group_M is live."""
import re
import unittest

import test_llvm138_narrow_tcmp as native


class StaticMatmulShapes(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def compile_matrix(self, name, source):
        cpp = self.output / f'{name}.cpp'
        cpp.write_text(source)
        result = self.compile('-S', '-emit-llvm', str(cpp), '-o', '-')
        self.assertEqual(result.returncode, 0, result.stdout)
        function = re.search(
            rf'^define [^\n]*@{name}\([^\n]*\{{\n(.*?)^\}}',
            result.stdout, re.MULTILINE | re.DOTALL)
        self.assertIsNotNone(function, result.stdout)
        body = function.group(1)
        self.assertNotIn('resolve_matmul_shape', body)
        matrix = next(line for line in body.splitlines()
                      if 'asm sideeffect' in line and 'TMATMUL' in line)
        # Distinct N/K catch swapped dimensions as well as lost constants.
        self.assertRegex(matrix, r'i64 16, i64 32,')
        obj = self.output / f'{name}.o'
        compiled = self.compile('-c', str(cpp), '-o', str(obj))
        self.assertEqual(compiled.returncode, 0, compiled.stdout)
        self.assertGreater(obj.stat().st_size, 0)
        return body, matrix, result.stdout

    def test_static_operands(self):
        self.compile_matrix('static_matrix', r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using A = Tile<Location::Left, float, 16, 32, BLayout::CubeM16>;
using B = Tile<Location::Right, float, 32, 16, BLayout::CubeN8>;
using D = CubeAccumulatorM16<float, 16, 16>;
extern "C" void static_matrix(D &d, A &a, B &b) { TMATMUL(d,a,b); }
''')

    def test_cooperative_operands(self):
        body, matrix, ir = self.compile_matrix('cooperative_matrix', r'''
// Scope the diagnostic fixture explicitly, without a global runtime ABI
// override. Redirect the header's builtin diagnostic to this explicitly
// scoped declaration for the duration of the include.
extern "C" int matmul_test_printf(const char *, ...)
    __attribute__((linx_scalar_shareability("intra")));
#define __builtin_printf matmul_test_printf
#include <common/pto_tileop.hpp>
#undef __builtin_printf
using namespace pto;
using A = Tile<Location::Left, float, 16, 32, BLayout::CubeM16>;
using S = SharedMatrixRight<float, 16, 32>;
using D = CubeAccumulatorM16<float, 16, 16>;
extern "C" void cooperative_matrix(D &d, A &a, S &b, size_t groupM) {
  auto shared = TMOV_L2S_INSERT(b);
  TMATMUL(d,a,shared,groupM);
}
''')
        # M remains a register operand, with the runtime range guard intact.
        self.assertRegex(matrix, r'i64 %\w+, i64 16, i64 32,')
        self.assertRegex(body, r'icmp u(?:gt|lt) i64 .*12[78]')
        self.assertIn('@matmul_test_printf(', body)
        self.assertIn('"linx-scalar-shareability"="intra"', ir)


if __name__ == '__main__':
    unittest.main()
