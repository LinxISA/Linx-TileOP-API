import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "include/jcore/template_asm.hpp").read_text(encoding="utf-8")
FIXTURE = (ROOT / "test/tileop_api/src/ValidShapeImmediate.cpp").read_text(encoding="utf-8")
LLVM_MC = Path("/home/zhuwei/linx-llvm/build/bin/llvm-mc")

class TestStaticValidShapeLowering(unittest.TestCase):
    def test_shared_tload_uses_zero_in_dim_first_operand(self) -> None:
        self.assertIn('"B.DIM zero, %c[VCOL], ->lb0\\n"', HEADER)
        self.assertIn('"B.DIM zero, %c[VROW], ->lb1\\n"', HEADER)
        self.assertIn('[VCOL]"i"(valid_col)', HEADER)
        self.assertIn('[VROW]"i"(valid_row)', HEADER)
        self.assertNotIn('[VCOL]"ri"(valid_col)', HEADER)
        self.assertNotIn('[VROW]"ri"(valid_row)', HEADER)

    def test_shared_tload_keeps_c_b_dimi_compression_coverage(self) -> None:
        self.assertIn('"B.DIM zero, %c[COL], ->lb2\\n"', HEADER)
        self.assertTrue(LLVM_MC.exists())
        result = subprocess.run(
            [str(LLVM_MC), "-triple=linx64v5", "-show-encoding"],
            input=b"B.DIM zero, 16, ->lb0\n",
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=True,
        )
        self.assertIn(b"C.B.DIMI", result.stdout)

    def test_fixture_declares_static_and_dynamic_tiles(self) -> None:
        self.assertIn('using S = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;', FIXTURE)
        self.assertIn('using D = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, -1, -1>;', FIXTURE)
        self.assertIn('TLOAD(dst, src);', FIXTURE)

if __name__ == '__main__':
    unittest.main()
