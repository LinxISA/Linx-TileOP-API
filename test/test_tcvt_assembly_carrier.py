import re
import unittest

import test_llvm138_narrow_tcmp as native


class TCvtAssemblyCarrier(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def test_packed_parent_and_continuation_reuse(self):
        source = self.output / 'tcvt-assembly.cpp'
        source.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using In = CubeTileM32<__bf16,32,32>;
using Wide = CubeTileM32<__bf16,32,64>;
using Fragment = CubeTileM32<__fp4_e2m1x2,32,32>;
using Parent = CubeTileM32<__fp4_e2m1x2,32,64>;
extern "C" void direct(Parent &out, In &a, In &b) {
  TileArray<Fragment,1,2> parts;
  TCVT(parts[0][0], a);
  TCVT(parts[0][1], b);
  out = TASSEMBLY<Parent>(std::move(parts));
}
extern "C" void subview(Parent &out, Wide &in) {
  auto views = TPARTVIEW<In,1,2>(in);
  TileArray<Fragment,1,2> parts;
  auto a = views[0][0];
  auto b = views[0][1];
  TCVT(parts[0][0], a);
  TCVT(parts[0][1], b);
  out = TASSEMBLY<Parent>(std::move(parts));
}
''')
        result = self.compile('-S', '-emit-llvm', str(source), '-o', '-')
        self.assertEqual(result.returncode, 0, result.stdout)
        continuations = [line for line in result.stdout.splitlines()
                         if 'call void asm' in line and 'B.ASSEMBLE' in line]
        self.assertEqual(len(continuations), 2, result.stdout)
        for call in continuations:
            self.assertRegex(call, r'B\.IOT \$3\.reuse, \$0\.reuse,')
        self.assertNotRegex(result.stdout, r'call[^\n]*assignData')
        result = self.compile('-c', str(source), '-o', str(self.output / 'tcvt.o'))
        self.assertEqual(result.returncode, 0, result.stdout)


if __name__ == '__main__':
    unittest.main()
