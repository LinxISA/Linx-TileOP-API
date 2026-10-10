#!/usr/bin/env python3
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TILE = (ROOT / "include/common/pto_tile.hpp").read_text(encoding="utf-8")
ASM = (ROOT / "include/jcore/template_asm.hpp").read_text(encoding="utf-8")
FIXTURE = (ROOT / "test/tileop_api/src/Issue111SharedSlot.cpp").read_text(
    encoding="utf-8"
)


class SharedSlotContractTest(unittest.TestCase):
    def test_slot_carrier_is_architectural_metadata_only(self):
        block = TILE[TILE.index("class SharedTileSlot"):TILE.index("template <typename T> struct is_shared_tile_slot")]
        self.assertIn("static_assert(Slot_ < 64", block)
        self.assertIn("using LocalTileType = LocalTile", block)
        self.assertNotIn("Handle", block)
        self.assertNotIn("handle_ref", block)
        self.assertNotIn("unsigned long", block)

    def test_slot_producers_and_consumer_encode_same_slot(self):
        for name in ("TLOAD_SLOT", "TIMG2COL_SPART_SLOT", "TMATMUL_SLOT"):
            self.assertIn(name, ASM)
        self.assertIn("->S%c[Slot]", ASM)
        self.assertIn("S%c[SharedA]", ASM)
        self.assertIn("S%c[SharedB]", ASM)
        self.assertIn("TIMG2COL_SPART_SLOT<DN2ND, SlotA::Slot", FIXTURE)
        self.assertIn("TLOAD_SLOT<OIHW2NK, SlotB::Slot", FIXTURE)
        self.assertIn("TMATMUL_SLOT<Acc, SlotA::Slot, SlotA, SlotB::Slot, SlotB>", FIXTURE)

    def test_slot_api_does_not_use_shared_cxx_constraints(self):
        start = ASM.index("// Slot form for Shared-producing weight loads")
        end = ASM.index("// TLOAD_ASS:", start)
        tload_slot = ASM[start:end]
        start = ASM.index("template <is_tile_data_v Dst, unsigned SlotA")
        end = ASM.index("#define PTO_DEFINE_MATMUL_3SRC_HELPER", start)
        matmul_slot = ASM[start:end]
        start = ASM.index("template <LayoutCvtEnum SourceOrder = NORM, unsigned Slot")
        end = ASM.index("// TFILLPAD:", start)
        timg_slot = ASM[start:end]
        for block in (tload_slot, matmul_slot, timg_slot):
            self.assertNotIn('"Sr"', block)
            self.assertNotIn("handle_ref", block)
            self.assertNotRegex(block, r"\[Shared\]")

    def test_fixture_only_uses_explicit_slots(self):
        self.assertEqual(len(re.findall(r"SharedTile<", FIXTURE)), 0)
        self.assertIn("SharedTileSlot<0", FIXTURE)
        self.assertIn("SharedTileSlot<1", FIXTURE)


if __name__ == "__main__":
    unittest.main()
