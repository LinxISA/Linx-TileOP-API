#!/usr/bin/env python3
from pathlib import Path
import unittest


HEADER = (Path(__file__).parents[1] / "include/jcore/template_asm.hpp").read_text()
DOC_ROOT = (Path(__file__).parents[1] /
            "docs/tileop-usage/layout-and-rearrangement/layout")
DOC_INDEX = (Path(__file__).parents[1] /
             "docs/tileop-usage/README.md").read_text()


class PTO0585LayoutInterfaceTest(unittest.TestCase):
    def test_selectors(self):
        for name, selector in {
            "TPERMUTE": 117,
            "TSHUF": 118,
            "TPACK": 119,
            "TUNPACK": 120,
            "TGPR2T": 126,
        }.items():
            self.assertIn(f"void {name}(", HEADER)
            self.assertIn(f'"BSTART.TEPL {selector},', HEADER)

    def test_operand_schemas(self):
        self.assertIn('"B.IOT %[Src0], %[Src1], mask=1111\\n"', HEADER)
        self.assertIn(
            '"B.IOT %[Indices], mask=1111, last, ->%[Dst]<%Z[Size]>\\n"',
            HEADER,
        )
        self.assertEqual(HEADER.count('"B.IOR [%[Control]],[]\\n"'), 3)
        self.assertIn('"B.IOR [%[Gpr0],%[Gpr1],%[Gpr2]],[]\\n"', HEADER)
        self.assertIn('"B.IOR [%[Gpr3]],[]\\n"', HEADER)
        self.assertIn(
            '"B.IOT mask=1111, last, ->%[Dst]<%Z[Size]>\\n"', HEADER
        )

    def test_usage_documentation(self):
        for name in ("TPERMUTE", "TSHUF", "TPACK", "TUNPACK", "TGPR2T"):
            document = DOC_ROOT / f"{name}.md"
            self.assertTrue(document.is_file(), f"missing documentation: {document}")
            content = document.read_text()
            self.assertIn(f"# {name}", content)
            self.assertIn("## C++ 接口", content)
            self.assertIn("## 约束", content)
            self.assertIn("## Bundle 组成", content)
            self.assertIn("## 使用示例", content)
            self.assertIn(f"layout-and-rearrangement/layout/{name}.md", DOC_INDEX)


if __name__ == "__main__":
    unittest.main()