import re
import subprocess
import unittest
from pathlib import Path

import test_llvm138_narrow_tcmp as narrow


class CubeLayoutTest(unittest.TestCase):
    setUpClass = classmethod(narrow.LLVM138NarrowTCMPTest.setUpClass.__func__)
    compile = narrow.LLVM138NarrowTCMPTest.compile

    def test_cube_comparison_and_select_layouts(self):
        source = self.output / "cube-layout.cpp"
        for rows, layout in ((16, "CubeM16"), (32, "CubeM32")):
            for mode in ("EQ", "NE", "LT", "LE", "GT", "GE"):
                with self.subTest(layout=layout, mode=mode):
                    source.write_text(
                        '#include <common/pto_tileop.hpp>\nusing namespace pto;\n'
                        f'using Values = Tile<Location::Vec, uint32_t, {rows}, 4, '
                        f'BLayout::{layout}>;\n'
                        f'using Mask = Tile<Location::Vec, uint8_t, {rows}, 4, '
                        f'BLayout::{layout}>;\n'
                        'void compare(Values &left, Values &right, Mask &mask) {\n'
                        f'TCMP<CmpMode::{mode}>(mask, left, right);\n'
                        'TSEL(left, mask, right);\n}\n'
                        'void scalar(Values &left, Values &right, Mask &mask, '
                        'uint32_t threshold) {\n'
                        f'TCMPS<CmpMode::{mode}>(mask, left, threshold);\n'
                        'TSEL(left, mask, right);\n}\n'
                    )
                    obj = self.output / "cube-layout.o"
                    result = self.compile("-c", str(source), "-o", str(obj))
                    self.assertEqual(result.returncode, 0, result.stdout)
                    text = subprocess.check_output([
                        str(Path(self.flags[0]).parent /
                            "llvm-objdump"), "-d",
                        "--disassembler-options=no-tile-macros", str(obj),
                    ], text=True)
                    expected = "CUBE_M16" if rows == 16 else "CUBE_M32"
                    attributes = re.findall(r"B\.DATR[^\n]*", text)
                    attributes = [line for line in attributes if expected in line]
                    self.assertEqual(len(attributes), 4, text)
                    self.assertTrue(all(expected in line for line in attributes), text)
                    comparisons = [line for line in attributes
                                   if mode in line or
                                   (mode == "EQ" and "Zero" in line)]
                    self.assertEqual(len(comparisons), 2, text)


if __name__ == "__main__":
    unittest.main()
