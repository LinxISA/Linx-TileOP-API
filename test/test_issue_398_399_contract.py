#!/usr/bin/env python3
import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
TILE = (ROOT / "include/common/pto_tile.hpp").read_text()
ASM = (ROOT / "include/jcore/template_asm.hpp").read_text()


class Issue398399ContractTest(unittest.TestCase):
    def test_cube_geometry_is_exact_capacity_derived(self):
        self.assertIn("static constexpr int CubePhysicalRows", TILE)
        self.assertIn("static constexpr int CubePhysicalCols", TILE)
        self.assertIn(
            "(StorageBytes * 8) / (CubePhysicalRows * CubeElementBits)", TILE
        )
        self.assertIn("CubeRequiredBytes <= StorageBytes", TILE)
        fixture = (ROOT / "test/tileop_api/src/TStoreAtomic.cpp").read_text()
        self.assertIn("SlackM32::PhysicalRows == 32", fixture)
        self.assertIn("SlackM32::PhysicalCols == 4", fixture)
        self.assertIn("SlackM32::CubeRequiredCellCount == 3", fixture)
        self.assertIn("SlackM32::CubeCellCount == 4", fixture)

    def test_atomic_tstore_aliases_use_function_one_fields(self):
        names = ("ADD", "MIN", "MAX", "AND", "OR", "XOR", "INC", "DEC")
        for mode, name in enumerate(names):
            self.assertIn(
                f"PTO_DEFINE_TSTORE_ATOMIC_API(TSTORE_ATOMIC_{name}, {mode})",
                ASM,
            )
        self.assertIn('"BSTART.TLSU TSTORE, %D[DataType]\\n"', ASM)
        self.assertIn("NORM.canon", ASM)
        self.assertIn("M322ND.canon", ASM)
        self.assertIn("M162ND.canon", ASM)
        self.assertIn('RNONE, nosat, 0, 0', ASM)
        self.assertNotRegex(ASM, r'BSTART\.TLSU\s+TSTORE\.ATOMIC_')

    def test_cube_to_shared_nd_uses_canonical_tmov(self):
        self.assertIn("TMOV_CUBE_TO_SHARED_ND", ASM)
        self.assertIn('"BSTART.TMOV DTYPE_NONE\\n"', ASM)
        self.assertIn("B.DATR M322ND, DTYPE_NONE, Zero", ASM)
        self.assertIn("B.DATR M162ND, DTYPE_NONE, Zero", ASM)
        self.assertIn('B.ASSEMBLE %c[Init], %c[Last]', ASM)
        self.assertIn('[Init] "i"(1)', ASM)
        self.assertIn('[Init] "i"(0)', ASM)
        self.assertIn('PTO_PE_MASK_ASM("B.IOS %S[Shared], mask=", "\\n")', ASM)

    def test_negative_compile_cases_are_registered(self):
        driver = (ROOT / "test/tileop_api/run_negatives.sh").read_text()
        fixture = (ROOT / "test/tileop_api/src/Issue398399Negatives.cpp").read_text()
        for case in (
            "atomic_add_dtype",
            "atomic_inc_dtype",
            "atomic_cube_n8",
            "cube_shared_dtype",
            "cube_shared_cols",
            "cube_shared_capacity",
            "cube_shared_writer_size",
        ):
            self.assertIn(case, driver)
            self.assertIn(f"SHOULD_FAIL_{case.upper()}", fixture)


if __name__ == "__main__":
    unittest.main()
