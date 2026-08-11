#!/usr/bin/env python3

import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
HEADER = ROOT / "include" / "jcore" / "template_asm.hpp"
ACTIVE_TEXT_ROOTS = (ROOT / "include", ROOT / "docs")
ACTIVE_IMPLEMENTATION_ROOTS = (ROOT / "include", ROOT / "test" / "tileop_api" / "src")
LINXISA_V058_COMMIT = "0a12890427edc2179ed75ad26039cdcebc6b4486"
LINXISA_V058_TREE = "fef6c084b166f3fd85a1b3d1b72fc069e6050800"
LINXISA_V058_CATALOG_SHA256 = "b38864f4630be258ec62e5690d794463d0574443782c06b9a79d7d0a4362c61b"


class LinxISAV058EngineContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.contract = json.loads(CONTRACT.read_text(encoding="utf-8"))
        cls.header = HEADER.read_text(encoding="utf-8")

    def test_projection_is_exact_linxisa_v058_release(self) -> None:
        self.assertEqual(self.contract["profile"], "v0.58")
        self.assertEqual(self.contract["source"]["release"], "v0.58")
        self.assertEqual(self.contract["source"]["commit"], LINXISA_V058_COMMIT)
        self.assertEqual(self.contract["source"]["tree"], LINXISA_V058_TREE)
        self.assertEqual(self.contract["source"]["sha256"], LINXISA_V058_CATALOG_SHA256)
        self.assertEqual(
            self.contract["semantic_engine_counts"],
            {"CUBE": 12, "SFU": 52, "TLSU": 10, "VEC": 35},
        )

    def test_jcore_emits_only_canonical_vec_sfu_aliases(self) -> None:
        operations = {row["name"]: row["engine"] for row in self.contract["tepl_ops"]}
        self.assertNotIn("BSTART.TEPL", self.header)
        emitted = re.findall(r'BSTART\.(VEC|SFU)\s+([A-Z][A-Z0-9_.]+),', self.header)
        self.assertEqual(len(emitted), 87)
        self.assertEqual({name for _, name in emitted}, set(operations))
        for engine, name in emitted:
            self.assertEqual(engine, operations[name], name)

    def test_jcore_uses_named_tlsu_and_cube_block_starts(self) -> None:
        self.assertNotIn("BSTART.TLSU", self.header)
        self.assertNotIn("BSTART.CUBE", self.header)
        for row in self.contract["tlsu_ops"]:
            if row["name"] in {"TPREFETCH", "MGATHER_CAS"}:
                continue
            self.assertIn(row["mnemonic"], self.header, row["name"])
        for row in self.contract["cube_ops"]:
            if row["name"].startswith("TGEMV"):
                continue
            self.assertIn(row["mnemonic"].removeprefix("BSTART."), self.header, row["name"])

    def test_shared_tile_bindings_use_b_ios(self) -> None:
        self.assertNotIn("C.B.IOS", self.header)
        self.assertRegex(self.header, r"B\.IOS %S\[Shared[AB]\], mask=1111")
        self.assertRegex(self.header, r'"B\.IOS mask=" PTO_PE_MASK_ASM')

    def test_pe_masks_are_four_binary_digits(self) -> None:
        active_text = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for root in ACTIVE_TEXT_ROOTS
            for path in root.rglob("*")
            if path.is_file()
        )
        self.assertNotRegex(active_text, r"mask=(?![01]{4}(?=[^0-9]|$))[0-9]+")
        self.assertIn("%c[PEMask3]%c[PEMask2]%c[PEMask1]%c[PEMask0]", self.header)
        iot_records = re.findall(r'"B\.IOT ([^"]*)\\n"', self.header)
        self.assertTrue(iot_records)
        for record in iot_records:
            self.assertIn("mask=", record, record)

    def test_retired_tile_operations_are_not_exposed(self) -> None:
        retired = set(self.contract["deleted_tile_names"])
        active_text = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for root in ACTIVE_TEXT_ROOTS
            for path in root.rglob("*")
            if path.is_file()
        )
        for name in retired:
            self.assertIsNone(re.search(rf"\b{re.escape(name)}\b", active_text), name)

    def test_retired_block_spellings_are_absent_from_implementation(self) -> None:
        active_implementation = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for root in ACTIVE_IMPLEMENTATION_ROOTS
            for path in root.rglob("*")
            if path.is_file()
        )
        for spelling in (
            "BSTART.PAR",
            "B.IOD",
            "C.B.IOS",
            "B.FPATR",
            ".FIXP\"",
        ):
            self.assertNotIn(spelling, active_implementation)

    def test_inline_asm_uses_current_compiler_contract(self) -> None:
        self.assertNotRegex(self.header, r'"[=+&]*(?:Tr|vr)"')
        self.assertNotIn("%Z", self.header)
        self.assertRegex(self.header, r"B\.DATR[^\n]*%D")
        self.assertIn("->%q", self.header)

    def test_tile_carrier_preserves_logical_size(self) -> None:
        tile_header = (ROOT / "include" / "common" / "pto_tile.hpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("ext_vector_type(1024)", tile_header)
        self.assertIn("LogicalTileBytes", tile_header)
        self.assertIn("TilesizeCode", tile_header)

    def test_active_surface_uses_tlsu_and_per_pe_tsize(self) -> None:
        active_text = "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for root in ACTIVE_TEXT_ROOTS
            for path in root.rglob("*")
            if path.is_file()
        )
        self.assertIsNone(re.search(r"\bTMA\b|\bTma[A-Za-z_]*", active_text))
        self.assertNotRegex(active_text, r"512 B(?:\.\.|–)32 KB")
        self.assertIn("128 B..8 KB", active_text)

    def test_tlsu_stride_is_expressed_in_logical_elements(self) -> None:
        self.assertNotRegex(
            self.header,
            r"(?:RowStride|ColStride)\s*\*\s*sizeof",
        )
        tlsu_doc = (ROOT / "docs" / "tileop-usage" / "tlsu.md").read_text(encoding="utf-8")
        self.assertIn("logical elements", tlsu_doc)
        self.assertNotIn("after multiplying", tlsu_doc)

    def test_shared_tmov_uses_unique_source_forms_and_shared_registers(self) -> None:
        for source_form in (
            "BSTART.TMOV.L2S.INSERT",
            "BSTART.TMOV.L2S.PUBLISH",
            "BSTART.TMOV.S2L.BROADCAST",
            "BSTART.TMOV.S2L.EXTRACT",
        ):
            self.assertIn(source_form, self.header)
        self.assertNotIn("shared_tmov_source_form_is_unique", self.header)
        self.assertRegex(self.header, r'\[Shared\]\s+"=S"')
        self.assertRegex(self.header, r'\[Shared\]\s+"S"')
        self.assertNotRegex(
            self.header,
            r'\[Shared[A-Za-z]*\]\s+"r"\([^\n]*handle\(\)',
        )

    def test_generated_engine_document_is_fresh(self) -> None:
        generated = ROOT / "docs" / "tileop-usage" / "engines.md"
        self.assertTrue(generated.is_file())
        self.assertIn("**VEC**, **TLSU**, **CUBE**, and **SFU**", generated.read_text())

    def test_first_build_does_not_require_preexisting_output_directory(self) -> None:
        makefile = (ROOT / "test" / "common" / "Makefile.common").read_text(encoding="utf-8")
        self.assertIn("OBJ_ROOT := $(abspath $(TEST_ROOT)/../output)", makefile)
        self.assertNotIn("realpath $(TEST_ROOT)/../output", makefile)

    def test_test_harness_uses_v058_compiler_surface_without_install_mutation(self) -> None:
        makefile = (ROOT / "test" / "common" / "Makefile.common").read_text(encoding="utf-8")
        runner = (ROOT / "test" / "script" / "test.py").read_text(encoding="utf-8")
        harness = makefile + "\n" + runner
        self.assertIn("--target=linx64", harness)
        self.assertIn("-fenable-matrix", harness)
        self.assertNotIn("-mlxbc", harness)
        self.assertNotIn("enable-all-vector-as-tilereg", harness)
        self.assertNotIn("cp_hpp_to_llvmlib", runner)
        self.assertNotIn("lib/clang/15.0.4/include/tileop-api", runner)


if __name__ == "__main__":
    unittest.main()
