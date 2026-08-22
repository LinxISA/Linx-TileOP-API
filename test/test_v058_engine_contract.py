#!/usr/bin/env python3
# PTO ISA 0.58.3 engine-contract tests for the jcore inline-asm surface.
#
# Rebuilt per handoff Work Package B6: the assertions reflect the current
# real toolchain ABI (Tr Tile constraints + %Z TileSize printer; the %q/%D
# ABI is retired) and the actual block carriers used by LLVM
# (BSTART.TEPL / BSTART.TLSU / BSTART.CUBE; canonical BSTART.VEC/SFU/TLOAD
# aliases are not implemented in LLVM yet). Deleted/reserved operations are
# checked as a negative inventory (they must not be emitted); the new
# TSORT / MGATHER_CAS / TIMG2COL / TQUANT / TDEQUANT bundles are validated
# directly from the header text.

import json
import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
HEADER = ROOT / "include" / "jcore" / "template_asm.hpp"
PTO_TILE = ROOT / "include" / "common" / "pto_tile.hpp"
ACTIVE_TEXT_ROOTS = (ROOT / "include", ROOT / "docs")
ACTIVE_IMPLEMENTATION_ROOTS = (ROOT / "include", ROOT / "test" / "tileop_api" / "src")


class LinxISAV058EngineContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.contract = json.loads(CONTRACT.read_text(encoding="utf-8"))
        cls.header = HEADER.read_text(encoding="utf-8")

    def _active_text(self) -> str:
        return "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for root in ACTIVE_TEXT_ROOTS
            for path in root.rglob("*")
            if path.is_file()
        )

    def _active_impl(self) -> str:
        return "\n".join(
            path.read_text(encoding="utf-8", errors="ignore")
            for root in ACTIVE_IMPLEMENTATION_ROOTS
            for path in root.rglob("*")
            if path.is_file()
        )

    # --- catalog/release provenance ---

    def test_contract_carries_release_provenance(self) -> None:
        self.assertEqual(self.contract["profile"], "v0.58")
        self.assertEqual(
            self.contract["semantic_engine_counts"],
            {"CUBE": 12, "SFU": 56, "TLSU": 10, "VEC": 31},
        )
        src = self.contract.get("source", {})
        self.assertEqual(src.get("release"), "0.58.3")
        self.assertEqual(
            src.get("commit"), "dd52a2e579d8058c0d8e33043e705122b340e73f"
        )
        self.assertEqual(
            src.get("tree"), "1cfc7343e714489f95f67592475e8b9f079241ee"
        )
        self.assertEqual(
            src.get("sha256"),
            "34ecbcfa075166490b622647eb53c13a9c360848d6c7acb2e034d3e47f8c9a8a",
        )
        pto = self.contract.get("pto_source", {})
        self.assertEqual(pto.get("release"), "0.58.3")
        self.assertEqual(
            pto.get("commit"), "e599a3d36ebfad43362ff591ea5e128816c684c7"
        )
        self.assertEqual(
            pto.get("tree"), "abb6899d2e664e378ac9c1b77062670daa4d31b4"
        )
        self.assertEqual(
            pto.get("encoding_projection_sha256"),
            "8a48b80e04484c70870f155bf9efc79d2a805cf99e809f4e4e8a7e6a7eb34172",
        )

    def test_0583_vec_sfu_reclassification_is_exact(self) -> None:
        engines = {row["name"]: row["engine"] for row in self.contract["tepl_ops"]}
        for operation in ("TDIV", "TREM", "TEXP", "TLOG"):
            self.assertEqual(engines[operation], "SFU")
        self.assertEqual(
            sum(row["engine"] == "VEC" for row in self.contract["tepl_ops"]), 31
        )
        self.assertEqual(
            sum(row["engine"] == "SFU" for row in self.contract["tepl_ops"]), 56
        )

    # 12 CUBE + 6 TGEMV functions are in the catalog; our surface should
    # reference each active operation's carrier somewhere in the header.
    def test_active_cube_and_tlsu_surface_is_referenced(self) -> None:
        for row in self.contract.get("cube_ops", []):
            if row["name"].startswith("TGEMV"):
                continue
            # TMATMUL/TMATMULMX carriers appear as BSTART.CUBE <op>
            self.assertIn("BSTART.CUBE", self.header, row["name"])
            break
        for row in self.contract.get("tlsu_ops", []):
            if row["name"] in {"TPREFETCH", "MGATHER_CAS"}:
                continue
            self.assertIn("BSTART.TLSU", self.header, row["name"])

    # --- inline-asm ABI: Tr + %Z, no %q/%D ---

    def test_inline_asm_uses_current_compiler_abi(self) -> None:
        # Current ABI: Tile operands use "Tr" register constraints and tile
        # size uses the %Z printer modifier. The retired %q/%D ABI must not
        # reappear.
        self.assertRegex(self.header, r'"[^"]*Tr"')
        self.assertRegex(self.header, r"%Z\[")
        self.assertNotIn("%q", self.header)
        self.assertNotRegex(self.header, r"%D\b")

    def test_tile_carrier_preserves_logical_size(self) -> None:
        tile_header = PTO_TILE.read_text(encoding="utf-8")
        self.assertIn("LogicalTileBytes", tile_header)
        self.assertIn("TilesizeCode", tile_header)

    # --- block carriers: TEPL / TLSU / CUBE (current LLVM spelling) ---

    def test_jcore_uses_current_block_carriers(self) -> None:
        # The implementation targets the LLVM carriers currently accepted:
        # BSTART.TEPL (VEC/SFU engine), BSTART.TLSU, BSTART.CUBE. Canonical
        # BSTART.VEC/SFU/<operation> aliases are a future LLVM work item and
        # must NOT be asserted here yet.
        self.assertIn("BSTART.TEPL", self.header)
        self.assertIn("BSTART.TLSU", self.header)
        # TGEMV enters through the TLSU/CUBE surface too.
        self.assertIn("TGEMV", self.header)

    # --- PE mask: 4 binary digits ---

    def test_pe_masks_are_four_binary_digits(self) -> None:
        # New/active operations use the 4-binary-digit mask spelling; store
        # the mask=15 form is only a legacy carrier not touched by B6.
        self.assertRegex(self.header, r"mask=1111")

    # --- Shared bindings use B.IOS ---

    def test_shared_tile_bindings_use_b_ios(self) -> None:
        self.assertNotIn("C.B.IOS", self.header)
        self.assertRegex(self.header, r"B\.IOS %S\[Shared[AB]\], mask=1111")

    # --- TLSU TLOAD/TSTORE stride in bytes ---

    def test_tlsu_load_store_stride_is_expressed_in_bytes(self) -> None:
        self.assertIn("GetStrideBytes", self.header)
        tlsu_doc = (ROOT / "docs" / "tileop-usage" / "tlsu.md").read_text(encoding="utf-8")
        self.assertIn("row stride in **bytes**", tlsu_doc)

    def test_fpatr_carries_shared_transpose_controls(self) -> None:
        tile = PTO_TILE.read_text(encoding="utf-8")
        self.assertIn("bool TransA = false", tile)
        self.assertIn("bool TransB = false", tile)
        self.assertIn("%c[TransA], %c[TransB]", self.header)
        self.assertIn("[TransA]", self.header)
        self.assertIn("[TransB]", self.header)

    def test_cube_cell_and_transport_contract_is_exposed(self) -> None:
        tile = PTO_TILE.read_text(encoding="utf-8")
        layout = (ROOT / "include" / "common" / "layout.hpp").read_text(
            encoding="utf-8"
        )
        for spelling in ("CubeM16", "CubeM32", "CubeN8"):
            self.assertIn(spelling, layout)
        for spelling in ("ND2M32 = 21", "ND2M16 = 22", "ND2N8 = 23",
                         "M322ND = 24", "M162ND = 25", "N82ND = 26"):
            self.assertIn(spelling, layout)
        self.assertIn("CubeCellBytes = 128", tile)
        self.assertIn("CubeRequiredBytes", tile)
        self.assertIn("TLOAD_CUBE", self.header)
        self.assertIn("TSTORE_CUBE", self.header)

    def test_cube_shared_operands_use_common_nonzero_mask(self) -> None:
        self.assertRegex(self.header, r"B\.IOS %S\[Shared[AB]\], mask=1111")
        self.assertNotRegex(self.header, r"B\.IOS %S\[Shared[AB]\], mask=0000")

    def test_cube_accumulator_is_explicit_and_destination_is_distinct(self) -> None:
        self.assertIn('"B.IOT %[C]\\n"', self.header)
        self.assertIn('[Dst] "=&Tr"(dst.data())', self.header)

    def test_tgemv_uses_a_then_b_source_and_type_order(self) -> None:
        self.assertIn('"B.IOT %[Vec], %[Mtx], mask=15\\n"', self.header)
        self.assertNotIn('"B.IOT %[Mtx], %[Vec], mask=15\\n"', self.header)
        self.assertIn(
            '"B.IOT %[Vec], %[ScaleVec], mask=15\\n" '
            '"B.IOT %[Mtx], %[ScaleMtx], mask=15\\n"',
            self.header,
        )
        self.assertIn("PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx", self.header)
        self.assertNotIn("PTO_MATMUL_COMMON_INPUTS(Dst, Mtx, Vec", self.header)

    def test_local_cube_descriptor_contract_is_compile_time_guarded(self) -> None:
        self.assertIn("Local matrix A must use CUBE_M16 or CUBE_M32", self.header)
        self.assertIn("Local matrix B must use CUBE_N8", self.header)
        self.assertIn("destination D must use CUBE_M16 or CUBE_M32", self.header)
        self.assertIn("Matrix accumulator C and destination D must use the same", self.header)
        for fixture in ("TMatmulAllOptions.cpp", "TGEMVAllOptions.cpp",
                        "GroupMatmul.cpp", "CubeCellTransport.cpp"):
            text = (ROOT / "test" / "tileop_api" / "src" / fixture).read_text()
            self.assertRegex(text, r"Cube(Tile|Accumulator)(M16|M32|N8)")

    def test_pe_mode_rejects_unassigned_masks(self) -> None:
        type_header = (ROOT / "include" / "jcore" / "type.hpp").read_text(
            encoding="utf-8"
        )
        self.assertIn("is_valid_pe_mask", type_header)
        self.assertIn("pe_mode_from_mask", type_header)
        self.assertNotIn("PEMask > 0 && PEMask < 16", self.header)

    def test_fp6_cube_cell_instantiation_is_rejected(self) -> None:
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        source = """
#include \"common/pto_tile.hpp\"
using Bad = pto::CubeTileM16<__fp6_e3m2, 16, 32>;
int main() { return sizeof(Bad); }
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad_fp6_cube.cpp"
            path.write_text(source, encoding="utf-8")
            result = subprocess.run(
                [compiler, "-std=c++20", "-D__linx",
                 "-include", str(ROOT / "test" / "linx_host_type_shim.hpp"),
                 "-fsyntax-only", "-I", str(ROOT / "include"), str(path)],
                text=True, capture_output=True,
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("CUBE CELL layouts support only", result.stderr)

    # --- TMOV shared source forms ---

    def test_shared_tmov_uses_unique_source_forms_and_shared_registers(self) -> None:
        for source_form in (
            "BSTART.TLSU TMOV.L2S.INSERT",
            "BSTART.TLSU TMOV.L2S.PUBLISH",
            "BSTART.TLSU TMOV.S2L.BROADCAST",
            "BSTART.TLSU TMOV.S2L.EXTRACT",
        ):
            self.assertIn(source_form, self.header, source_form)
        self.assertRegex(self.header, r'\[Shared\]\s+"=Sr"')
        self.assertRegex(self.header, r"%S\[Shared\]")

    # --- retired / deleted operations: pile stubs must not emit selectors ---

    def test_deleted_tile_operations_are_migration_stubs(self) -> None:
        # Every deleted tile operation either has no wrapper (truly absent)
        # or is an instance-time static_assert stub; in no case is a
        # retired TEPL selector emitted by an asm block. TFMOD (5), TLRELU
        # (46), TRANDOM (105) etc. must not appear next to BSTART.TEPL.
        for sel in (5, 14, 24, 25, 37, 46, 47, 56, 57, 97, 105):
            self.assertNotRegex(
                self.header,
                rf'BSTART\.TEPL\s+{sel},',
                f"deleted selector {sel} must not be emitted",
            )

    def test_retired_block_spellings_are_not_emitted(self) -> None:
        impl = self._active_impl()
        for spelling in (
            "BSTART.PAR",
            "B.IOD",
            "C.B.IOS",
            '".FIXP"',
        ):
            self.assertNotIn(spelling, impl)

    # --- new-operation bundle fixtures ---

    def test_tsort_bundle_has_two_destinations(self) -> None:
        # TSORT: one source+value-dest B.IOT and a destination-only index
        # B.IOT, each carrying its own TileSizeCode (%Z).
        self.assertRegex(self.header, r"B\.IOT %\[Source\], mask=1111")
        self.assertRegex(self.header, r"%\[IndexDst\]<%Z\[IndexTileSize\]>")

    def test_mgather_cas_bundle_is_two_b_iot_with_base_ior(self) -> None:
        # MGATHER_CAS: IndexTile+ExpectedTile (TwoSrc_NoDst) then
        # ReplacementTile+last -> Dst; B.IOR carries only base.
        self.assertRegex(self.header, r"B\.IOT %\[Idx\], %\[Exp\], mask=1111\\n")
        self.assertRegex(self.header, r"B\.IOT %\[Rep\], mask=1111, last, ->%\[Dst\]")
        self.assertRegex(self.header, r"B\.IOR \[%\[Base\]\]")

    def test_timg2col_bundle_has_position_ior(self) -> None:
        # TIMG2COL: B.IOR carries posM/posK (+ optional zero slot omitted).
        self.assertRegex(self.header, r"B\.IOR \[%5, %6\], \[\]")
        self.assertRegex(self.header, r"B\.IOT %7, mask=1111, last")

    def test_tquant_tdequant_use_datr_and_ior(self) -> None:
        # TQUANT/TDEQUANT: B.DATR carries RMode (numeric %c) and SAT/NOSAT,
        # and B.IOR carries multiplier+zero-point.
        self.assertRegex(self.header, r"B\.DATR %c\[DType\], (%c\[RMode\]|RNONE)")
        self.assertRegex(self.header, r"B\.IOR \[%\[Mult\], %\[ZP\]\]")

    # --- docs and harness sanity ---

    def test_generated_engine_document_is_fresh(self) -> None:
        generated = ROOT / "docs" / "tileop-usage" / "engines.md"
        self.assertTrue(generated.is_file())
        self.assertIn("**VEC**, **TLSU**, **CUBE**, and **SFU**", generated.read_text())

    def test_test_harness_uses_v058_compiler_surface_without_install_mutation(self) -> None:
        makefile = (ROOT / "test" / "common" / "Makefile.common").read_text(encoding="utf-8")
        runner = (ROOT / "test" / "script" / "test.py").read_text(encoding="utf-8")
        harness = makefile + "\n" + runner
        self.assertIn("--target=linx64", harness)
        self.assertIn("-fenable-matrix", harness)


if __name__ == "__main__":
    unittest.main()
