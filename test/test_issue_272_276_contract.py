import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = (ROOT / "include/jcore/template_asm.hpp").read_text()
REGION = (ROOT / "include/common/pto_tile_region.hpp").read_text()
FIXTURES = ROOT / "test/tileop_api/src"


class Issue272To276Contract(unittest.TestCase):
    def test_tpack_supports_views_and_assemble(self):
        self.assertIn("void TPACK_ASS", HEADER)
        self.assertIn("B.SUBVIEW 0", HEADER)
        self.assertIn("B.SUBVIEW 1", HEADER)
        self.assertIn("tpack_selected_prefix_defined", HEADER)
        self.assertIn("is_tile<region::ReductionPrefixView", REGION)
        for name in ("Issue272PackPrefix.cpp", "Issue274PackViews.cpp",
                     "Issue275PackAssemble.cpp", "Issue276ReinterpretMul.cpp"):
            self.assertTrue((FIXTURES / name).is_file())
