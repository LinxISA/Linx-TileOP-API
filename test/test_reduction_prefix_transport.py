import re
import subprocess
import unittest
from pathlib import Path

import test_llvm138_narrow_tcmp as native


class ReductionPrefixTransport(unittest.TestCase):
    setUpClass = classmethod(native.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = native.LLVM138NarrowTCMPTest.compile

    def test_prefix_consumers_preserve_parent_carrier(self):
        source = self.output / "prefix.cpp"
        source.write_text(r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using Parent = Tile<Location::Vec, float, 32, 256, BLayout::CubeM32, 32, 1>;
using Prefix = Tile<Location::Vec, float, 32, 1, BLayout::CubeM32>;
extern "C" void right(Prefix &d, Prefix &a, Parent &p) {
  auto v = TREDUCEPREFIXVIEW<Prefix>(p); TMAX(d, a, v);
}
extern "C" void left(Prefix &d, Parent &p, Prefix &b) {
  auto v = TREDUCEPREFIXVIEW<Prefix>(p); TMAX(d, v, b);
}
extern "C" void both(Prefix &d, Parent &p, Parent &q) {
  auto v = TREDUCEPREFIXVIEW<Prefix>(p);
  auto w = TREDUCEPREFIXVIEW<Prefix>(q); TMAX(d, v, w);
}
extern "C" void unary(Prefix &d, Parent &p) {
  auto v = TREDUCEPREFIXVIEW<Prefix>(p); TEXP(d, v);
}
extern "C" void scalar(Prefix &d, Parent &p, float x) {
  auto v = TREDUCEPREFIXVIEW<Prefix>(p); TMULS(d, v, x);
}
extern "C" void convert(Prefix &d, Parent &p) {
  const auto v = TREDUCEPREFIXVIEW<Prefix>(p); TCVT(d, v);
}
extern "C" void fused(Prefix &d, Prefix &a, Prefix &b, Parent &p) {
  auto v = TREDUCEPREFIXVIEW<Prefix>(p); TFMA(d, a, b, v);
}
extern "C" void expand(Parent &d, Parent &a, Parent &p) {
  const auto v = TREDUCEPREFIXVIEW<Prefix>(p); TROWEXPANDADD(d, a, v);
}
''')
        result = self.compile("-S", "-emit-llvm", str(source), "-o", "-")
        self.assertEqual(result.returncode, 0, result.stdout)
        bodies = dict(re.findall(
            r"^define [^\n]*@(\w+)\([^\n]*\{\n(.*?)^\}",
            result.stdout, re.MULTILINE | re.DOTALL,
        ))
        for name, count in (("right", 1), ("left", 1), ("both", 2),
                            ("unary", 1), ("scalar", 1), ("convert", 1),
                            ("fused", 1), ("expand", 2)):
            with self.subTest(consumer=name):
                body = bodies[name]
                self.assertNotRegex(body, r"\bload <8192 x i32>")
                loads = re.findall(r"call <8192 x i32> @llvm\.linx\.blk\.tload[^\n]+", body)
                self.assertEqual(len(loads), count, body)
                for load in loads:
                    self.assertIn("i64 1, i64 32, i64 1, i64 1, i64 0, i64 21,", load)
                self.assertIn("B.SUBVIEW", body)

        obj = self.output / "prefix.o"
        result = self.compile("-c", str(source), "-o", str(obj))
        self.assertEqual(result.returncode, 0, result.stdout)
        physical = subprocess.check_output([
            str(Path(self.flags[0]).parent / "llvm-objdump"), "-d",
            "--disassembler-options=no-tile-macros", str(obj),
        ], text=True)
        self.assertIn("ND2M32", physical)
        self.assertIn("B.SUBVIEW", physical)
        self.assertNotRegex(physical, r"BSTART\.TLOAD\s+S32")


if __name__ == "__main__":
    unittest.main()
