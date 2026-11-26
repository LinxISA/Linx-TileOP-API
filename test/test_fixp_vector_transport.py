"""Keep register-only U64/N8 fixp parameters out of memory transports."""
import re
import subprocess
import unittest
from pathlib import Path

import test_llvm138_narrow_tcmp as native


class FixpVectorTransport(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def test_vector_options_preserve_register_parameter(self):
        source = self.output / "fixp.cpp"
        source.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using Quant = CubeTileN8<uint64_t, 2, 32, 1, 32>;
using A = CubeTileM32<int8_t, 32, 32>;
using B = CubeTileN8<int8_t, 32, 32>;
using D = Tile<Location::Acc, _Float16, 32, 32, BLayout::CubeM32>;
using F16 = CubeTileM32<_Float16, 32, 32>;
using F16B = CubeTileN8<_Float16, 32, 32>;
using S8 = Tile<Location::Acc, int8_t, 32, 32, BLayout::CubeM32>;
extern "C" void vector_quant(D &d, A &a, B &b, uint64_t *input) {
  Quant quant;
  global_tensor<uint64_t, RowMajor<1, 32>> gm(input);
  TLOAD(quant, gm);
  TMATMUL(d, a, b, fixp::vector<FixpPreQuantMode::VDEQF16>(quant));
}
extern "C" void s8_quant(S8 &d, F16 &a, F16B &b, uint64_t *input) {
  Quant quant;
  global_tensor<uint64_t, RowMajor<1, 32>> gm(input);
  TLOAD(quant, gm);
  TMATMUL(d, a, b, fixp::s8(quant));
}
extern "C" void quant_relu(S8 &d, F16 &a, F16B &b, uint64_t *input) {
  Quant quant;
  global_tensor<uint64_t, RowMajor<1, 32>> gm(input);
  TLOAD(quant, gm);
  TMATMUL(d, a, b, fixp::s8(quant).relu());
}
extern "C" void prelu(D &d, F16 &a, F16B &b, uint64_t *input) {
  Quant parameter;
  global_tensor<uint64_t, RowMajor<1, 32>> gm(input);
  TLOAD(parameter, gm);
  TMATMUL(d, a, b, fixp::f16().prelu(parameter));
}
extern "C" void quant_prelu(S8 &d, F16 &a, F16B &b,
                            uint64_t *input, uint64_t *slope) {
  Quant quant, parameter;
  global_tensor<uint64_t, RowMajor<1, 32>> gm(input), pm(slope);
  TLOAD(quant, gm);
  TLOAD(parameter, pm);
  TMATMUL(d, a, b, fixp::s8(quant).prelu(parameter));
}
''')
        result = self.compile("-S", "-emit-llvm", str(source), "-o", "-")
        self.assertEqual(result.returncode, 0, result.stdout)
        # Check the actual parameter values, not just successful compilation:
        # a raw NORM spill would compile but discard the CUBE representation.
        bodies = dict(re.findall(
            r"^define [^\n]*@(\w+)\([^\n]*\{\n(.*?)^\}",
            result.stdout, re.MULTILINE | re.DOTALL,
        ))
        for name, count in (("vector_quant", 1), ("s8_quant", 1),
                            ("quant_relu", 1), ("prelu", 1),
                            ("quant_prelu", 2)):
            with self.subTest(consumer=name):
                body = bodies[name]
                producers = re.findall(
                    r'(%[\w.]+) = (?:tail )?call <128 x i32> asm[^\n]*TLOAD[^\n]*',
                    body)
                self.assertEqual(len(producers), count, body)
                consumer = next(line for line in body.splitlines()
                                if 'asm sideeffect "BSTART.CUBE TMATMUL' in line)
                for producer in producers:
                    self.assertIn("<128 x i32> " + producer, consumer)
                self.assertNotRegex(body, r'\b(?:load|store) <128 x i32>')
        obj = self.output / "fixp.o"
        result = self.compile("-c", str(source), "-o", str(obj))
        self.assertEqual(result.returncode, 0, result.stdout)
        physical = subprocess.check_output([
            str(Path(self.flags[0]).parent / "llvm-objdump"), "-d",
            "--disassembler-options=no-tile-macros", str(obj),
        ], text=True)
        self.assertIn("ND2N8", physical)
        self.assertNotIn("N82ND", physical)
        self.assertIn("TMATMUL", physical)


if __name__ == "__main__":
    unittest.main()
