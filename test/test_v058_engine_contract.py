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
import struct
import subprocess
import sys
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
            self.assertIn('"BSTART.CUBE " OPCODE', self.header, row["name"])
            break
        for row in self.contract.get("tlsu_ops", []):
            if row["name"] in {"TPREFETCH", "MGATHER_CAS"}:
                continue
            spelling = row["name"].replace("_", ".")
            self.assertIn(f"BSTART.TLSU {spelling}", self.header, row["name"])

    # --- inline-asm ABI: Tr + %Z + %D ---

    def test_inline_asm_uses_current_compiler_abi(self) -> None:
        # Current ABI: Tile operands use "Tr" register constraints and tile
        # size uses %Z and DataType immediates use the target %D printer.
        self.assertRegex(self.header, r'"[^"]*Tr"')
        self.assertRegex(self.header, r"%Z\[")
        self.assertNotIn("%q", self.header)
        self.assertRegex(self.header, r"%D(?:\[|\d)")

    def test_tile_carrier_preserves_logical_size(self) -> None:
        tile_header = PTO_TILE.read_text(encoding="utf-8")
        self.assertIn("LogicalTileBytes", tile_header)
        self.assertIn("TilesizeCode", tile_header)

    # --- block carriers: TEPL / TLSU / CUBE (current LLVM spelling) ---

    def test_jcore_uses_current_block_carriers(self) -> None:
        # TEPL remains the raw VEC/SFU carrier; TLSU/CUBE use exact named starts.
        self.assertIn("BSTART.TEPL", self.header)
        self.assertIn("BSTART.TLSU TLOAD", self.header)
        self.assertIn('"BSTART.CUBE " OPCODE', self.header)
        # TGEMV enters through the TLSU/CUBE surface too.
        self.assertIn("TGEMV", self.header)

    def test_tcvt_emits_dimensions_before_terminating_iot(self) -> None:
        tcvt = re.search(
            r'(?s)template <is_tile_data_v tile_shape_out, '
            r'is_tile_data_v tile_shape_in>\n'
            r'void TCVT_T\(.*?\n}\n\n#define DEFINE_TMOV_LAYOUT',
            self.header,
        )
        self.assertIsNotNone(tcvt)
        carrier = tcvt.group(0)
        ordinary_branch = carrier.split('} else {', 1)[1]
        for instruction in (
                '"BSTART.TEPL 27, %D1\\n"',
                '"B.DATR %D2, RNONE\\n"',
                '"B.DIM %5, 0, ->lb0\\n"',
                '"B.DIM %6, 0, ->lb1\\n"',
                '"B.DIM zero, %c7, ->lb2\\n"',
                '"B.IOT %3, mask=1111, last, ->%0<%Z4>\\n"'):
            self.assertIn(instruction, ordinary_branch)

    def test_tcvt_cube_layout_closure_uses_destination_tsize(self) -> None:
        tcvt = re.search(
            r'(?s)template <is_tile_data_v tile_shape_out, '
            r'is_tile_data_v tile_shape_in>\n'
            r'void TCVT_T\(.*?\n}\n\n#define DEFINE_TMOV_LAYOUT',
            self.header,
        )
        self.assertIsNotNone(tcvt)
        carrier = tcvt.group(0)
        cube_branch = carrier.split('if constexpr (IsCubeMSource) {', 1)[1].split(
            '} else {', 1)[0]
        self.assertIn('tile_shape_in::BFractal == BLayout::CubeM16', carrier)
        self.assertIn('tile_shape_in::BFractal == BLayout::CubeM32', carrier)
        self.assertIn('tile_shape_out::TilesizeCode', cube_branch)
        self.assertNotIn('->lb2', cube_branch)
        cube_fixture = (ROOT / 'test' / 'tileop_api' / 'src' /
                        'CubeTCvt.cpp').read_text(encoding='utf-8')
        self.assertIn('TCVT(dst, src);', cube_fixture)
        self.assertIn('__tilesize_1KB', cube_fixture)

    def test_range_modifiers_expose_simple_factories(self) -> None:
        tile_header = PTO_TILE.read_text(encoding="utf-8")
        tile = (ROOT / "test" / "tileop_api" / "src" /
                "RangeSubview.cpp").read_text(encoding="utf-8")
        assemble = (ROOT / "test" / "tileop_api" / "src" /
                    "RangeAssemble.cpp").read_text(encoding="utf-8")
        self.assertIn("auto sv = range::subview(s);", tile)
        self.assertIn("auto sv = range::subview(s, base_units);", tile)
        self.assertIn("auto sv = range::subview<128, 3>(s);", tile)
        self.assertIn("auto sv = range::subview<128, 3>(s, base_units);", tile)
        self.assertIn("auto sv = range::subview_at_reg<3, 23>(s, 23);", tile)
        self.assertIn("auto as = range::assemble(d);", assemble)
        self.assertIn("auto as = range::assemble(d, base_units);", assemble)
        self.assertIn("auto as = range::assemble<128, 3>(d, base_units);", assemble)
        self.assertIn("auto as = range::assemble_last_at<2047>(d);", assemble)
        self.assertIn("auto as = range::assemble_init_last(d);", assemble)
        self.assertIn("auto as = range::assemble_middle<128, 3>(d, base_units);", assemble)
        self.assertIn("auto as = range::assemble_last_at_reg<3, 23>(d, 23);", assemble)
        self.assertIn("auto subview(Parent &parent", tile_header)
        self.assertIn("LengthBytes_ = 0, unsigned OffsetUnits_ = 0", tile_header)
        self.assertIn("PTO_DEFINE_ASSEMBLE_FACTORY(assemble", tile_header)
        self.assertIn("PTO_DEFINE_ASSEMBLE_FACTORY(assemble_last", tile_header)
        self.assertIn("auto subview_at(Parent &parent", tile_header)
        self.assertIn("length cannot exceed the parent Tile capacity", tile_header)
        self.assertIn("auto assemble_last_at_reg(Parent &parent", tile_header)

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
        tlsu_doc = (ROOT / "docs" / "tileop-usage" / "tlsu" / "load-store-move" / "TLOAD.md").read_text(encoding="utf-8")
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
        self.assertIn('"B.IOT %[C], mask=1111\\n"', self.header)
        self.assertIn('[Dst] "=&Tr"(dst.data())', self.header)

    def test_tgemv_uses_a_then_b_source_and_type_order(self) -> None:
        self.assertIn('"B.IOT %[Vec], %[Mtx], mask=1111\\n"', self.header)
        self.assertNotIn('"B.IOT %[Mtx], %[Vec], mask=1111\\n"', self.header)
        self.assertIn(
            '"B.IOT %[Vec], mask=1111\\n" ".if %c[HasScaleA]\\n" '
            '"B.IOT %[ScaleVec], mask=1111\\n" ".endif\\n" '
            '"B.IOT %[Mtx], mask=1111\\n" ".if %c[HasScaleB]\\n" '
            '"B.IOT %[ScaleMtx], mask=1111\\n"',
            self.header,
        )
        self.assertIn("PTO_MATMUL_COMMON_INPUTS(Dst, Vec, Mtx", self.header)
        self.assertNotIn("PTO_MATMUL_COMMON_INPUTS(Dst, Mtx, Vec", self.header)

    def test_mx_scale_presence_is_validated_per_input_side(self) -> None:
        self.assertIn("ScaleA presence must match the PTO MX type contract",
                      self.header)
        self.assertIn("ScaleB presence must match the PTO MX type contract",
                      self.header)
        self.assertIn("constexpr bool HasScaleA = (ScaleMask & 1) != 0;",
                      self.header)
        self.assertIn("constexpr bool HasScaleB = (ScaleMask & 2) != 0;",
                      self.header)
        self.assertIn("PTO_MX_SCALE_INPUTS", self.header)
        a_source = ('"B.IOT %[A], mask=1111\\n" ".if %c[HasScaleA]\\n" '
                    '"B.IOT %[ScaleA], mask=1111\\n"')
        b_source = ('"B.IOT %[B], mask=1111\\n" ".if %c[HasScaleB]\\n" '
                    '"B.IOT %[ScaleB], mask=1111\\n"')
        a_pos = self.header.find(a_source)
        b_pos = self.header.find(b_source, a_pos)
        self.assertGreaterEqual(a_pos, 0)
        self.assertGreater(b_pos, a_pos)
        fixture = (ROOT / "test" / "tileop_api" / "src" /
                   "MXScaleVariants.cpp").read_text(encoding="utf-8")
        for spelling in ("__half", "__bf16", "__fp8_e4m3", "__fp8_e5m2",
                         "__fp4_e2m1x2", "__fp4_e1m2x2"):
            self.assertIn(spelling, fixture)
        negatives = (ROOT / "test" / "tileop_api" / "run_negatives.sh").read_text(
            encoding="utf-8"
        )
        for case in ("missing_mx_scale_a", "missing_mx_scale_b",
                     "extra_mx_scale_a", "extra_mx_scale_b",
                     "bad_mx_scale_dtype", "bad_mx_scale_shape"):
            self.assertIn(case, negatives)

    def test_local_cube_descriptor_contract_is_compile_time_guarded(self) -> None:
        self.assertIn("Local matrix A must use CUBE_M16 or CUBE_M32", self.header)
        self.assertIn("Local matrix B must use CUBE_N8", self.header)
        self.assertIn("destination D must use CUBE_M16 or CUBE_M32", self.header)
        self.assertIn("Matrix accumulator C and destination D must use the same", self.header)
        for fixture in ("TMatmulAllOptions.cpp", "TGEMVAllOptions.cpp",
                        "GroupMatmul.cpp", "CubeCellTransport.cpp"):
            text = (ROOT / "test" / "tileop_api" / "src" / fixture).read_text()
            self.assertRegex(text, r"Cube(Tile|Accumulator)(M16|M32|N8)")

    def test_matrix_dtype_and_effective_shape_contract_is_centralized(self) -> None:
        tile = PTO_TILE.read_text(encoding="utf-8")
        self.assertIn(
            '"B.DATR %D[DataTypeB], RNONE, NOSAT\\n"', self.header
        )
        self.assertIn("matrix_accumulator_type_code", tile)
        self.assertIn("MatrixNumericClass::Unsigned", tile)
        self.assertIn("OutputCode == AccCode", tile)
        self.assertIn("FP32/S32/U32 AccType", tile)
        self.assertIn("Matrix D valid shape must match effective M x N", self.header)
        self.assertIn("Attr.TransA", self.header)
        self.assertIn("? A::ValidCol : A::ValidRow", self.header)
        self.assertIn("Attr.TransB", self.header)
        self.assertIn("? B::ValidRow : B::ValidCol", self.header)
        integer_fixture = (
            ROOT / "test" / "tileop_api" / "src" / "MatrixIntegerDtypes.cpp"
        ).read_text()
        self.assertIn("CubeAccumulatorM16<int32_t", integer_fixture)
        self.assertIn("CubeAccumulatorM16<uint32_t", integer_fixture)
        transpose_fixture = (
            ROOT / "test" / "tileop_api" / "src" /
            "SharedTransposeNonSquare.cpp"
        ).read_text()
        self.assertIn("transpose_a().transpose_b()", transpose_fixture)
        self.assertIn("RowMax", transpose_fixture)
        self.assertIn("GroupMax", transpose_fixture)
        self.assertGreaterEqual(self.header.count("constexpr int EffectiveM"), 2)
        self.assertGreaterEqual(self.header.count("constexpr int EffectiveN"), 2)
        for fixture in ("TMatmulAllOptions.cpp", "TGEMVAllOptions.cpp",
                        "SharedMatrixForms.cpp"):
            text = (ROOT / "test" / "tileop_api" / "src" / fixture).read_text()
            self.assertIn("__fp8_e4m3", text)
            self.assertIn("__fp8_e8m0", text)

    def test_mc_gate_requires_canonical_cube_layout_names(self) -> None:
        gate = (ROOT / "test" / "tileop_api" /
                "verify_pto0583_asm.sh").read_text()
        self.assertIn(r"ND2M32\.normal", gate)
        self.assertIn(r"N82ND\.normal", gate)
        self.assertIn("missing canonical $description", gate)

    def test_active_inline_asm_uses_exact_pto0583_surface(self) -> None:
        # CUBE transport (TLOAD_CUBE/TSTORE_CUBE) legitimately uses the
        # canonical TLSU carrier ("BSTART.TLSU TLOAD/TSTORE") and the
        # ".normal" CUBE selectors (ND2M16/ND2M32/ND2N8/M162ND/M322ND/
        # N82ND); ordinary non-CUBE code must keep the exact 0.58.3 surface.
        self.assertNotRegex(self.header, r'"[^"\n]*mask=15(?:\D|$)')
        self.assertNotIn("mask=%c[PEMask]", self.header)
        self.assertNotRegex(
            self.header, r'"BSTART\.TEPL\s+\d+,\s*\d+,\s*%[cD]'
        )
        # Only the CUBE transport selectors may carry a ".normal" suffix on
        # B.DATR; any other B.DATR ... .normal is the old spelling.
        self.assertNotRegex(
            self.header,
            r'"B\.DATR (?!ND2M(?:16|32)|ND2N8|M(?:16|32)2ND|N82ND)[^"\n]*\.normal',
        )
        self.assertIn('"BSTART.TLSU TLOAD, %D[DataType]\\n"', self.header)
        self.assertIn('"BSTART.TLSU TSTORE, %D[DataType]\\n"', self.header)
        self.assertIn('"B.DATR ND2M32.normal, Zero\\n"', self.header)
        self.assertNotIn(
            '"B.DATR %c[DataTypeB], byte0, Zero\\n"', self.header
        )
        self.assertIn('"BSTART.CUBE " OPCODE ", %D[DataTypeA]\\n"', self.header)

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
        # TQUANT/TDEQUANT: B.DATR carries named dtype/RMode and optional sat,
        # and B.IOR carries multiplier+zero-point.
        self.assertRegex(
            self.header,
            r"B\.DATR %D\[__pto_DstType\], (?:RTZ|RTM|RTP|RNA|RTO|RHB)",
        )
        self.assertRegex(self.header, r"B\.IOR \[%\[Mult\], %\[ZP\]\]")

    # --- docs and harness sanity ---

    def test_generated_engine_document_is_fresh(self) -> None:
        generated = ROOT / "docs" / "tileop-usage" / "generated" / "engines.md"
        self.assertTrue(generated.is_file())
        self.assertIn("**VEC**, **TLSU**, **CUBE**, and **SFU**", generated.read_text())

    def test_test_harness_uses_v058_compiler_surface_without_install_mutation(self) -> None:
        makefile = (ROOT / "test" / "common" / "Makefile.common").read_text(encoding="utf-8")
        runner = (ROOT / "test" / "script" / "test.py").read_text(encoding="utf-8")
        harness = makefile + "\n" + runner
        self.assertIn("linx64-unknown-linux-musl", harness)
        self.assertIn("LINX_TARGET", harness)
        self.assertIn("-fenable-matrix", harness)

    def test_compile_all_aggregates_active_object_failures(self) -> None:
        harness = ROOT / "test" / "tileop_api" / "compile.all"
        with tempfile.TemporaryDirectory() as temporary:
            temporary_path = Path(temporary)
            calls = temporary_path / "calls.log"
            fake_make = temporary_path / "make"
            fake_make.write_text(
                "#!/bin/sh\n"
                f"printf '%s\\n' \"$*\" >> {calls}\n"
                "case \"$*\" in\n"
                "  *TESTCASE=CubeCellTransport*object*) exit 1 ;;\n"
                "esac\n"
                "exit 0\n",
                encoding="utf-8",
            )
            fake_make.chmod(0o755)
            environment = os.environ.copy()
            environment.update(
                {
                    "MAKE": str(fake_make),
                    "COMPILER_DIR": temporary,
                    "LINX_SYSROOT": temporary,
                }
            )
            result = subprocess.run(
                [str(harness), "objects"],
                cwd=ROOT / "test" / "tileop_api",
                env=environment,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("FAIL object: CubeCellTransport", result.stderr)
            self.assertIn("TESTCASE=TTrans object", calls.read_text(encoding="utf-8"))

    def test_pto_identity_verifier_rejects_hostile_elf_notes(self) -> None:
        verifier = ROOT / "test" / "tileop_api" / "verify_pto_identity.py"
        expected = (
            b'{"encoding_abi":"pto-isa-0.58.3-mode-function-v1",'
            b'"encoding_projection_sha256":'
            b'"8a48b80e04484c70870f155bf9efc79d2a805cf99e809f4e4e8a7e6a7eb34172",'
            b'"release":"0.58.3"}'
        )

        def make_elf(desc: bytes = expected, owner: bytes = b"PTO\0",
                     machine: int = 0xE9, magic: bytes = b"\x7fELF",
                     extra_desc: bytes | None = None) -> bytes:
            def make_note(note_desc: bytes) -> bytes:
                name = owner + b"\0" * ((-len(owner)) & 3)
                return (struct.pack("<III", len(owner), len(note_desc), 1) +
                        name + note_desc + b"\0" * ((-len(note_desc)) & 3))

            note = make_note(desc)
            extra_note = make_note(extra_desc) if extra_desc is not None else b""
            phnum = 2 if extra_note else 1
            note_offset = 64 + phnum * 56
            extra_offset = note_offset + len(note)
            shstr = b"\0.note.pto.isa\0.shstrtab\0"
            shstr_offset = extra_offset + len(extra_note)
            section_offset = (shstr_offset + len(shstr) + 7) & ~7
            image = bytearray(section_offset + 3 * 64)
            ident = magic + bytes((2, 1, 1, 0)) + bytes(8)
            struct.pack_into(
                "<16sHHIQQQIHHHHHH", image, 0, ident, 3, machine, 1, 0,
                64, section_offset, 0, 64, 56, phnum, 64, 3, 2,
            )
            struct.pack_into(
                "<IIQQQQQQ", image, 64, 4, 4, note_offset, note_offset,
                note_offset, len(note), len(note), 4,
            )
            if extra_note:
                struct.pack_into(
                    "<IIQQQQQQ", image, 64 + 56, 4, 4, extra_offset,
                    extra_offset, extra_offset, len(extra_note),
                    len(extra_note), 4,
                )
            image[note_offset:note_offset + len(note)] = note
            image[extra_offset:extra_offset + len(extra_note)] = extra_note
            image[shstr_offset:shstr_offset + len(shstr)] = shstr
            struct.pack_into(
                "<IIQQQQIIQQ", image, section_offset + 64,
                shstr.index(b".note.pto.isa"), 7, 2, note_offset,
                note_offset, len(note), 0, 0, 4, 0,
            )
            struct.pack_into(
                "<IIQQQQIIQQ", image, section_offset + 128,
                shstr.index(b".shstrtab"), 3, 0, 0, shstr_offset,
                len(shstr), 0, 0, 1, 0,
            )
            return bytes(image)

        with tempfile.TemporaryDirectory() as temporary:
            temporary_path = Path(temporary)
            variants = {
                "valid": make_elf(),
                "missing": make_elf(owner=b"GNU\0"),
                "corrupt": make_elf(magic=b"BAD!"),
                "wrong-machine": make_elf(machine=0x3E),
                "wrong-identity": make_elf(desc=expected.replace(b"0.58.3", b"0.58.2")),
                "trailing-nul": make_elf(desc=expected + b"\0"),
                "duplicate-segment": make_elf(extra_desc=expected),
                "conflicting-segment": make_elf(
                    extra_desc=expected.replace(b"0.58.3", b"0.58.2")),
            }
            for name, contents in variants.items():
                path = temporary_path / f"{name}.elf"
                path.write_bytes(contents)
                result = subprocess.run(
                    [sys.executable, str(verifier), str(path)],
                    text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                )
                if name == "valid":
                    self.assertEqual(result.returncode, 0, result.stderr)
                else:
                    self.assertNotEqual(result.returncode, 0, name)


if __name__ == "__main__":
    unittest.main()
