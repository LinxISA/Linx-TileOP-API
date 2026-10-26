import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "test/tileop_api/src"
CXX = os.environ.get("LINX_CXX", "/home/zhuwei/linx-llvm/build/bin/clang++")
SYSROOT = Path(os.environ.get(
    "LINX_SYSROOT",
    "/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/sysroot",
))


class LLVM138NarrowTCMPTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not Path(CXX).is_file() or not SYSROOT.is_dir():
            if "LINX_CXX" in os.environ or "LINX_SYSROOT" in os.environ:
                raise RuntimeError("configured Linx compiler/sysroot unavailable")
            raise unittest.SkipTest("Linx target compiler/sysroot unavailable")
        cls.temporary = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.output = Path(cls.temporary.name)
        resource = cls.output / "resource"
        original = subprocess.check_output([CXX, "-print-resource-dir"], text=True)
        shutil.copytree(original.strip(), resource)
        shutil.rmtree(resource / "include/tileop-api", ignore_errors=True)
        shutil.copytree(ROOT / "include", resource / "include/tileop-api")
        cls.flags = [
            CXX, "-resource-dir", str(resource),
            "--target=linx64v5-unknown-linux-musl", "-mlxbc",
            "--sysroot", str(SYSROOT), "-nostdinc++",
            "-isystem", str(SYSROOT / "usr/include/c++/v1"),
            "-fenable-matrix", "-O2", "-std=c++20", "-D__linx",
            "-DENABLE_TENSOR_INSTR",
        ]

    def compile(self, *arguments):
        return subprocess.run(
            self.flags + list(arguments), text=True,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        )

    def test_constructor_geometry_and_comparison_modes(self):
        result = self.compile(
            "-S", "-emit-llvm", str(SOURCES / "LLVM138NarrowTCMP.cpp"), "-o", "-",
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        bodies = dict(re.findall(
            r"^define [^\n]*@(\w+)\([^\n]*\{\n(.*?)^\}",
            result.stdout, re.MULTILINE | re.DOTALL,
        ))
        for rows, size_code in ((32, 1), (64, 2)):
            for constructor in ("dynamic", "static"):
                with self.subTest(rows=rows, constructor=constructor):
                    body = bodies[f"{constructor}{rows}"]
                    operations = [line for line in body.splitlines()
                                  if "BSTART.TEPL" in line]
                    self.assertEqual(len(operations), 3, body)
                    self.assertEqual(sum("BSTART.TEPL 13" in operation
                                         for operation in operations), 1, body)
                    for operation in operations:
                        self.assertNotIn("undef", operation)
                        self.assertRegex(operation, r'B\.DIM zero, \$\{4:c\}, ->lb2')
                        if "BSTART.TEPL 13" in operation:
                            self.assertIn(f"i32 1, <{rows} x i32>", operation)
                            self.assertRegex(operation, rf'i32 {size_code}\)')
                        else:
                            self.assertIn(f"i32 1, i32 {size_code},", operation)
        self.assertIn("BSTART.TEPL 13", bodies["cross_type32"])
        for mode in ("EQ", "NE", "LT", "GT", "LE", "GE"):
            self.assertIn(f"B.DATR Zero, {mode}", bodies["static_modes"])
        self.assertNotIn("llvm.trap", result.stdout)

    def test_target_object_compiles(self):
        result = self.compile(
            "-c", str(SOURCES / "LLVM138NarrowTCMP.cpp"),
            "-o", str(self.output / "narrow.o"),
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertGreater((self.output / "narrow.o").stat().st_size, 0)

    def test_final_assembly_keeps_required_lb0(self):
        result = self.compile(
            "-S", str(SOURCES / "LLVM138NarrowTCMP.cpp"), "-o", "-",
        )
        self.assertEqual(result.returncode, 0, result.stdout)
        for rows in (32, 64):
            for constructor in ("dynamic", "static"):
                with self.subTest(rows=rows, constructor=constructor):
                    name = f"{constructor}{rows}"
                    match = re.search(
                        rf"^{name}:.*?^\s*\.size\s+{name},",
                        result.stdout, re.MULTILINE | re.DOTALL,
                    )
                    self.assertIsNotNone(match, result.stdout)
                    dimensions = re.findall(
                        r"(?:C\.B\.DIMI|B\.DIM)\s+([^\n]*?),\s*->lb0", match.group(),
                    )
                    self.assertEqual(len(dimensions), 3, match.group())
                    for dimension in dimensions:
                        self.assertRegex(dimension, r"^(?:zero,\s*)?1$")

    def test_incompatible_carriers_are_rejected(self):
        for rows in (32, 64):
            for case, diagnostic in (
                ("PHYSICAL_SHAPE", "TCMP source physical shapes and layouts must match"),
                ("LAYOUT", "TCMP source physical shapes and layouts must match"),
                ("VALID_SHAPE", "TCMP source valid shapes must match"),
            ):
                with self.subTest(rows=rows, case=case):
                    result = self.compile(
                        "-fsyntax-only", f"-DPHYSICAL_ROWS={rows}",
                        f"-DSHOULD_FAIL_{case}",
                        str(SOURCES / "LLVM138NarrowTCMPNegatives.cpp"),
                    )
                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn(diagnostic, result.stdout)


if __name__ == "__main__":
    unittest.main()
