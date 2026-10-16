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

    def test_tpack_emits_distinct_plain_init_and_continuation_forms(self):
        start = HEADER.index("void tpack_emit")
        end = HEADER.index("\ntemplate <is_tile_data_v D", start)
        body = HEADER[start:end]

        associated_start = body.index("if constexpr (Associated)")
        init_start = body.index("} else if constexpr (is_assemble_v<D>)")
        plain_start = body.index("} else {", init_start)
        associated = body[associated_start:init_start]
        init = body[init_start:plain_start]
        plain = body[plain_start:]

        # The backend inspects inline-asm templates before assembler .if
        # folding, so the ordinary allocating form must contain no inactive
        # assemble modifier at all.
        self.assertNotIn('"B.ASSEMBLE', plain)
        self.assertNotIn(".if %c[Assemble]", body)

        # INIT owns an allocating destination. MIDDLE/LAST instead consumes
        # the already-open parent as the final source-form slot.
        self.assertIn('->%[Dst]<%Z[Size]>', init)
        self.assertIn('B.ASSEMBLE %c[Init]', init)
        self.assertEqual(init.count('"B.IOT '), 1)
        self.assertNotIn('->%[Dst]<%Z[Size]>', associated)
        self.assertIn('B.IOT %[ParentRef], mask=1111, last', associated)
        self.assertIn('[ParentRef] "Tr"(dst.data())', associated)
        self.assertIn('B.ASSEMBLE %c[Init]', associated)
        self.assertEqual(associated.count('"B.IOT '), 2)
        self.assertEqual(plain.count('"B.IOT '), 1)

        # Each source view retains its own base, offset, and extent.
        for branch in (plain, init, associated):
            self.assertIn('%[Src0Base], %c[Src0Off], %c[Src0Size]', branch)
            self.assertIn('%[Src1Base], %c[Src1Off], %c[Src1Size]', branch)
