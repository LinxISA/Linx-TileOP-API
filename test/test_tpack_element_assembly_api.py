#!/usr/bin/env python3
"""Native compiler contracts for TPACK plain and B.ASSEMBLE forms."""

import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "test/tileop_api/src"


class TPackElementAssemblyAPI(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = os.environ.get("LINX_CXX")
        cls.resource = os.environ.get("LINX_RESOURCE_DIR")
        cls.sysroot = os.environ.get("LINX_SYSROOT")
        if not (cls.compiler and cls.resource and cls.sysroot):
            raise unittest.SkipTest(
                "set LINX_CXX/LINX_RESOURCE_DIR/LINX_SYSROOT for native TPACK"
            )
        cls.common = [
            cls.compiler,
            "--target=linx64v5-unknown-linux-musl",
            "--sysroot=" + cls.sysroot,
            "-resource-dir=" + cls.resource,
            "-mlxbc",
            "-fenable-matrix",
            "-O2",
            "-std=c++20",
            "-D__linx",
            "-DENABLE_TENSOR_INSTR",
        ]
        cls.simt = [
            "-mllvm", "-enable-all-vector-as-tilereg=true",
            "-mllvm", "-linxv5-enable-HL-Inst-Opt=true",
            "-mllvm", "-linxv5-enable-dim-opt=true",
            "-mllvm", "-linxv5-enable-ldst-bridge=false",
            "-mllvm", "-linxv5-enable-continuous-mem-opt=true",
            "-mllvm", "-linxv5-enable-tile-clock-hand=false",
            "-mllvm", "-linxv5-enable-simt-clock-hand=true",
            "-mllvm", "-enable-misched=false",
        ]

    def run_compiler(self, args):
        result = subprocess.run(
            self.common + args, text=True, capture_output=True
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_four_issue_fixtures_compile_in_default_and_simt_lanes(self):
        cases = (
            "Issue272PackPrefix",
            "Issue274PackViews",
            "Issue275PackAssemble",
            "Issue276ReinterpretMul",
        )
        with tempfile.TemporaryDirectory(prefix="native-tpack-objects-") as work:
            work = Path(work)
            for case in cases:
                source = FIXTURES / (case + ".cpp")
                for lane, extra in (("default", []), ("simt", self.simt)):
                    output = work / f"{case}.{lane}.o"
                    self.run_compiler(extra + ["-c", str(source), "-o", str(output)])
                    self.assertGreater(output.stat().st_size, 0)

    def test_plain_has_no_assembly_modifier_and_continuation_is_source_form(self):
        with tempfile.TemporaryDirectory(prefix="native-tpack-contract-") as work:
            work = Path(work)
            plain_ir = work / "plain.ll"
            self.run_compiler([
                "-S", "-emit-llvm", str(FIXTURES / "Issue272PackPrefix.cpp"),
                "-o", str(plain_ir),
            ])
            plain_text = plain_ir.read_text()
            self.assertIn("BSTART.TEPL 119", plain_text)
            self.assertNotIn("B.ASSEMBLE", plain_text)
            self.assertRegex(
                plain_text,
                r"B\.IOT \$1, \$2, mask=1111, last, ->\$0<\$\{10:Z\}>",
            )

            assemble_ir = work / "assemble.ll"
            self.run_compiler([
                "-S", "-emit-llvm", str(FIXTURES / "Issue275PackAssemble.cpp"),
                "-o", str(assemble_ir),
            ])
            assembly_calls = [
                line for line in assemble_ir.read_text().splitlines()
                if "asm sideeffect" in line and "B.ASSEMBLE" in line
            ]
            self.assertEqual(len(assembly_calls), 2)
            init = next(line for line in assembly_calls if "call <" in line)
            continuation = next(line for line in assembly_calls if "call void asm" in line)
            self.assertRegex(
                init,
                r"B\.IOT \$1, \$2, mask=1111, last, ->\$0<\$\{10:Z\}>",
            )
            self.assertIn("B.ASSEMBLE ${17:c}, ${18:c}", init)
            self.assertRegex(continuation, r"B\.IOT \$2, mask=1111, last")
            self.assertNotIn("B.IOT $2, mask=1111, last, ->", continuation)
            self.assertRegex(continuation, r'"@2Tr,@2Tr,@2Tr,')

            obj = work / "assemble.o"
            self.run_compiler([
                "-c", str(FIXTURES / "Issue275PackAssemble.cpp"),
                "-o", str(obj),
            ])
            objdump = str(Path(self.compiler).parent / "llvm-objdump")
            dumped = subprocess.run(
                [objdump, "-d", "--disassembler-options=no-tile-macros", str(obj)],
                text=True,
                capture_output=True,
            )
            self.assertEqual(dumped.returncode, 0, dumped.stderr)
            pack_blocks = re.findall(
                r"BSTART\.TEPL\s+119,[\s\S]*?(?=BSTART|$)", dumped.stdout
            )
            self.assertEqual(len(pack_blocks), 2)
            self.assertRegex(
                pack_blocks[0],
                r"B\.IOT\s+[tumn]#\d+, [tumn]#\d+, mask=1111, last,\s+->t<512B>",
            )
            self.assertRegex(pack_blocks[0], r"B\.ASSEMBLE\s+1, 0, zero, 0, 1")
            self.assertRegex(pack_blocks[1], r"B\.IOT\s+t#1, mask=1111, last")
            self.assertNotRegex(pack_blocks[1], r"B\.IOT[^\n]*->")
            self.assertRegex(pack_blocks[1], r"B\.ASSEMBLE\s+0, 1, zero, 3, 1")


if __name__ == "__main__":
    unittest.main()
