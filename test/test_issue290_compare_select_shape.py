import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "include/jcore/template_asm.hpp"
SYSROOT = Path(os.environ.get(
    "LINX_SYSROOT",
    "/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/sysroot",
))
CXX = os.environ.get("LINX_CXX", "/home/zhuwei/linx-llvm/build/bin/clang++")


SOURCE = r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using WideF = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor, 16, 1>;
using NarrowF = Tile<Location::Vec, float, 16, 1, BLayout::RowMajor, 16, 1>;
using WideP = Tile<Location::Vec, int32_t, 16, 16, BLayout::RowMajor, 16, 1>;
void bad_cmp(WideP &d, WideF &a, NarrowF &b) {
  TCMP<CmpMode::GT>(d, a, b);
}
'''


class Issue290CompareSelectShapeTest(unittest.TestCase):
    def test_header_has_physical_shape_contracts(self):
        text = HEADER.read_text(encoding="utf-8")
        self.assertIn("TCMP source physical shapes and layouts must match", text)
        self.assertIn("TSEL physical shapes and layouts must match the destination", text)

    def test_mismatched_physical_shape_is_compile_error(self):
        if not Path(CXX).is_file() or not SYSROOT.is_dir():
            self.skipTest("Linx target compiler/sysroot unavailable")
        with tempfile.TemporaryDirectory() as temporary:
            temporary_path = Path(temporary)
            resource_dir = temporary_path / "clang-resource"
            shutil.copytree(
                Path(subprocess.check_output(
                    [CXX, "-print-resource-dir"], text=True).strip(),
                ),
                resource_dir,
            )
            shutil.rmtree(resource_dir / "include/tileop-api", ignore_errors=True)
            shutil.copytree(ROOT / "include", resource_dir / "include/tileop-api")
            source = temporary_path / "negative.cpp"
            source.write_text(SOURCE, encoding="utf-8")
            result = subprocess.run(
                [
                    CXX, "-resource-dir", str(resource_dir),
                    "--target=linx64v5-unknown-linux-musl", "-mlxbc",
                    "--sysroot", str(SYSROOT), "-nostdinc++",
                    "-isystem", str(SYSROOT / "usr/include/c++/v1"),
                    "-fenable-matrix", "-O2", "-std=c++20", "-D__linx",
                    "-DENABLE_TENSOR_INSTR", "-fsyntax-only", str(source),
                ],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("TCMP source physical shapes and layouts must match", result.stdout)


if __name__ == "__main__":
    unittest.main()
