#!/usr/bin/env python3
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
LAYOUT = (ROOT / "include/common/layout.hpp").read_text(encoding="utf-8")
TILE = (ROOT / "include/common/pto_tile.hpp").read_text(encoding="utf-8")
ASM = (ROOT / "include/jcore/template_asm.hpp").read_text(encoding="utf-8")
FIXTURE = (ROOT / "test/tileop_api/src/SharedTLoad.cpp").read_text(
    encoding="utf-8"
)


class WeightTLOADContractTest(unittest.TestCase):
    def test_only_assigned_nk_layout_codes_are_public(self):
        self.assertIn("OHWI2NK = 10", LAYOUT)
        self.assertIn("OIHW2NK = 11", LAYOUT)
        self.assertNotIn("OHWI2KN", LAYOUT)
        self.assertNotIn("OIHW2KN", LAYOUT)

    def test_packed_parameter_and_generation_metadata_fields(self):
        for expression in (
            "static_cast<uint64_t>(cout) << 16",
            "static_cast<uint64_t>(kernel_h) << 32",
            "static_cast<uint64_t>(kernel_w) << 40",
            "static_cast<uint64_t>(k_start) << 32",
            "static_cast<uint64_t>(metadata.valid_k) & 0xffff) << 10",
            "static_cast<uint64_t>(metadata.valid_n) & 0xffff) << 26",
            "static_cast<uint64_t>(metadata.total_k) & 0xffff) << 42",
            "static_cast<uint64_t>(metadata.size_code) & 0xf) << 58",
        ):
            self.assertIn(expression, TILE)

    def test_weight_bundle_uses_shared_nk_contract(self):
        start = ASM.index("// Weight-mode TLOAD:")
        end = ASM.index("// TLOAD_ASS: GM -> an already-associated Shared Tile", start)
        block = ASM[start:end]
        for command in (
            '"BSTART.TLSU TLOAD, %D[SrcType]\\n"',
            '"B.DATR layout%c[WeightLayout], DTYPE_NONE, Zero\\n"',
            '"B.DIM %[ValidK], 0, ->lb0\\n"',
            '"B.DIM %[ValidN], 0, ->lb1\\n"',
            '"B.DIM zero, %c[TotalK], ->lb2\\n"',
            '"B.IOR [%[Base],%[Shape],%[Start]], []\\n"',
        ):
            self.assertIn(command, block)
        self.assertIn('PTO_PE_MASK_ASM("B.IOS mask="', block)
        self.assertIn('[Shared] "=Sr"(dst.handle_ref())', block)
        self.assertIn("without an Assemble carrier supports one PE", block)

    def test_compile_fixture_exercises_weight_overload(self):
        self.assertIn("TLOAD<OHWI2NK>", FIXTURE)
        self.assertIn("make_weight_tload_params(16, 16, 1, 1", FIXTURE)


if __name__ == "__main__":
    unittest.main()