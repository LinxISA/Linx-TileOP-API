#!/usr/bin/env python3
"""Compile-time and native contracts for TileArray participant masks."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "test/tileop_api/src/TileArrayParticipantMask.cpp"


class TileArrayParticipantMaskTest(unittest.TestCase):
    def host_compile(self, declaration, accepts):
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(prefix="tile-array-mask-host-") as work:
            source = Path(work) / "contract.cpp"
            source.write_text(
                "#include <common/pto_tile.hpp>\n"
                "#include <common/pto_tile_region.hpp>\n"
                "using namespace pto;\n" + declaration
            )
            result = subprocess.run([
                compiler, "-std=c++20", "-D__linx", "-include",
                str(ROOT / "test/linx_host_type_shim.hpp"), "-I",
                str(ROOT / "include"), "-fsyntax-only", str(source),
            ], text=True, capture_output=True)
            self.assertEqual(result.returncode == 0, accepts, result.stderr)

    def test_public_type_and_invalid_masks(self):
        self.host_compile(
            "using A=TileArray<ElementTile<unsigned,32>,1,4,8>;"
            "static_assert(A::participant_mask==8);"
            "using R=decltype(std::declval<A&>()[0][0]);"
            "static_assert(R::participant_mask==8);",
            True,
        )
        for mask in (1, 2, 4, 8, 12, 14, 15):
            with self.subTest(valid_mask=mask):
                self.host_compile(
                    f"TileArray<ElementTile<unsigned,32>,1,4,{mask}> ok;",
                    True,
                )
        for mask in (0, 3, 5, 16):
            with self.subTest(mask=mask):
                self.host_compile(
                    f"TileArray<ElementTile<unsigned,32>,1,4,{mask}> bad;",
                    False,
                )

    def test_nondefault_mask_rejects_unadapted_writer(self):
        self.host_compile(
            "using Leaf=ElementTile<unsigned,32>;"
            "void default_writer(TileArrayOutputRef<Leaf>);"
            "void bad(){"
            "TileArray<ElementTile<unsigned,32>,1,4,8> a;"
            "default_writer(a[0][0]);}",
            False,
        )
        self.host_compile(
            "PTO_PE_DOMAIN(8) void owned(){} int main(){ owned(); }",
            False,
        )

    def native_common(self):
        compiler = os.environ.get("LINX_CXX")
        resource = os.environ.get("LINX_RESOURCE_DIR")
        sysroot = os.environ.get("LINX_SYSROOT")
        if not (compiler and resource and sysroot):
            self.skipTest("set LINX_CXX/LINX_RESOURCE_DIR/LINX_SYSROOT")
        return compiler, [
            compiler, "-std=c++20", "--target=linx64v5-unknown-linux-musl",
            "-D__linx", "-mlxbc", "-fenable-matrix", "-O2", "-mllvm",
            "-enable-all-vector-as-tilereg=true", "-resource-dir=" + resource,
            "-mllvm", "-linxv5-enable-generic-element=true",
            "-fno-vectorize", "-fno-slp-vectorize", "-Werror=unknown-pragmas",
            "--sysroot=" + sysroot, "-I", str(ROOT / "include"),
        ]

    def test_native_nested_pe0_and_default_masks(self):
        compiler, common = self.native_common()
        with tempfile.TemporaryDirectory(prefix="tile-array-mask-native-") as work:
            work = Path(work)
            ir = work / "mask.ll"
            result = subprocess.run(
                common + ["-S", "-emit-llvm", str(FIXTURE), "-o", str(ir)],
                text=True, capture_output=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            text = ir.read_text()
            pe0 = text[text.index("define dso_local void @assemble_pe0"):]
            pe0 = pe0[:pe0.index("define dso_local void @assemble_default")]
            self.assertIn("B.ASSEMBLE", pe0)
            self.assertIn("== 8\\0A", pe0)
            self.assertRegex(pe0, r"i1 false, i32 8, i32 29")

            obj = work / "mask.o"
            result = subprocess.run(
                common + ["-c", str(FIXTURE), "-o", str(obj)],
                text=True, capture_output=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            dumped = subprocess.run([
                str(Path(compiler).parent / "llvm-objdump"), "-d",
                "--disassembler-options=no-tile-macros", str(obj),
            ], text=True, capture_output=True)
            self.assertEqual(dumped.returncode, 0, dumped.stderr)
            pe0_asm = dumped.stdout[
                dumped.stdout.index("<assemble_pe0>"):
                dumped.stdout.index("<assemble_default>")
            ]
            assembly_blocks = [
                block for block in re.findall(
                    r"BSTART(?:\.TLSU\s+|\.)TMOV\b[\s\S]*?(?=BSTART|$)", pe0_asm
                ) if "B.ASSEMBLE" in block
            ]
            self.assertGreater(len(assembly_blocks), 4)
            for block in assembly_blocks:
                self.assertRegex(block, r"B\.IOT[^\n]*mask=1000")
                self.assertNotRegex(block, r"B\.IOT[^\n]*mask=1111")
            self.assertRegex(pe0_asm, r"B\.ASSEMBLE\s+1, 0, [^,]+, 0, 1")
            self.assertRegex(pe0_asm, r"B\.ASSEMBLE\s+1, 0, [^,]+, 0, 3")
            self.assertRegex(pe0_asm, r"BSTART\.TEPL\s+TXOR")
            self.assertEqual(
                len(re.findall(r"BSTART(?:\.TLSU\s+|\.)TSTORE\b", pe0_asm)), 2
            )
            default_asm = dumped.stdout[dumped.stdout.index("<assemble_default"):]
            self.assertRegex(default_asm, r"B\.IOT[^\n]*mask=1111")

    def test_native_nondefault_mask_rejects_unadapted_tcvt(self):
        _, common = self.native_common()
        with tempfile.TemporaryDirectory(prefix="tile-array-mask-negative-") as work:
            source = Path(work) / "bad.cpp"
            source.write_text(
                "#include <common/pto_tileop.hpp>\n"
                "using namespace pto;\n"
                "void bad(){ TileArray<ElementTile<unsigned,32>,1,4,8> a; "
                "ElementTile<unsigned,32> x; TCVT(a[0][0], x); }\n"
            )
            result = subprocess.run(
                common + ["-fsyntax-only", str(source)],
                text=True, capture_output=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("no matching function", result.stderr)

    def test_native_function_domain_annotation(self):
        _, common = self.native_common()
        with tempfile.TemporaryDirectory(prefix="pe-domain-annotation-") as work:
            source = Path(work) / "domain.cpp"
            ir = Path(work) / "domain.ll"
            source.write_text(
                "#include <common/pto_tile_region.hpp>\n"
                "PTO_PE_DOMAIN(8) void owned(unsigned *p){ *p = 8; }\n"
            )
            result = subprocess.run(
                common + ["-S", "-emit-llvm", str(source), "-o", str(ir)],
                text=True, capture_output=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            text = ir.read_text()
            self.assertIn('c"pto.pe_domain\\00"', text)
            self.assertRegex(text, r"@\.args = .*\{ i32 8 \}")

    def test_native_rejects_tile_reference_domain_boundary(self):
        _, common = self.native_common()
        with tempfile.TemporaryDirectory(prefix="pe-domain-tile-boundary-") as work:
            source = Path(work) / "bad.cpp"
            source.write_text(
                "#include <common/pto_tileop.hpp>\n"
                "using namespace pto;\n"
                "using Tile128=ElementTile<unsigned,128>;\n"
                "__attribute__((noinline)) static void consume("
                "Tile128 &input, unsigned *output){Tile128 result;"
                "TADDS(result,input,1u); TSTORE(output,result,128);}\n"
                "PTO_PE_DOMAIN(8) void bad(Tile128 &input, unsigned *output){"
                "consume(input,output);}\n"
            )
            result = subprocess.run(
                common + ["-c", str(source), "-o", str(Path(work) / "bad.o")],
                text=True, capture_output=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "Tile-reference arguments may not cross a function PE domain boundary",
                result.stderr,
            )


if __name__ == "__main__":
    unittest.main()
