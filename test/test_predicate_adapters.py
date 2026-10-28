import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CXX = Path(os.environ.get("LINX_CXX", "/home/zhuwei/linx-llvm/build/bin/clang++"))
SYSROOT = Path(os.environ.get(
    "LINX_SYSROOT",
    "/home/zhuwei/linx-toolchain-build-online-main/output/linx_blockisa_llvm_musl/sysroot",
))


class PredicateAdaptersTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not CXX.is_file() or not SYSROOT.is_dir():
            raise unittest.SkipTest("Linx compiler/sysroot unavailable")
        cls.temporary = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.output = Path(cls.temporary.name)
        resource = cls.output / "resource"
        original = subprocess.check_output([str(CXX), "-print-resource-dir"], text=True)
        shutil.copytree(original.strip(), resource)
        shutil.rmtree(resource / "include/tileop-api", ignore_errors=True)
        shutil.copytree(ROOT / "include", resource / "include/tileop-api")
        cls.flags = [
            str(CXX), "-resource-dir", str(resource),
            "--target=linx64v5-unknown-linux-musl", "-mlxbc", "-fenable-matrix",
            "-O2", "-std=c++20", "-D__linx", "-DENABLE_TENSOR_INSTR",
            "--sysroot", str(SYSROOT), "-nostdinc++",
            "-isystem", str(SYSROOT / "usr/include/c++/v1"),
        ]

    def compile(self, body, *arguments, dtype="uint32_t", columns=4):
        source = self.output / "predicate.cpp"
        source.write_text(
            '#include <common/pto_tileop.hpp>\nusing namespace pto;\n'
            f'using Values = VecTileM32<{dtype}, 32, {columns}>;\n'
            f'using Offsets = VecTileM32<uint64_t, 32, {columns}>;\n'
            f'using Global = global_tensor<{dtype}, RowMajor<32, {columns}>>;\n'
            f'void test(Values &values, Offsets &offsets, Global &destination, '
            f'{dtype} threshold) {{\n{body}\n}}\n'
        )
        return subprocess.run(self.flags + list(arguments) + [str(source)],
                              text=True, capture_output=True)

    def test_object_and_execution_mask(self):
        body = '''auto active = predicate::compare_scalar<CmpMode::GE>(values, threshold);
predicate::scatter(destination, offsets, values, active);
predicate::atomic_add(destination, offsets, values, active);
predicate::atomic_add<true>(destination, offsets, values, active);'''
        objdump = CXX.parent / "llvm-objdump"
        for dtype in ("uint32_t", "int32_t", "float"):
            for columns in (4, 8):
                with self.subTest(dtype=dtype, columns=columns):
                    obj = self.output / "predicate.o"
                    result = self.compile(body, "-c", "-o", str(obj),
                                          dtype=dtype, columns=columns)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    text = subprocess.check_output([
                        str(objdump), "-d", "--no-show-raw-insn",
                        "--disassembler-options=no-tile-macros", str(obj),
                    ], text=True)
                    self.assertEqual(text.count("ExecMaskPresent"), 2, text)
                    self.assertEqual(text.count("MSCATTER.ADD"), 2, text)
                    self.assertEqual(text.count("MSCATTER.MASK"), 1, text)
                    self.assertNotIn("TCVT", text)
                    self.assertNotIn("TLOAD, S64", text)
                    self.assertIn("ND2M32", text)
                    self.assertIn("nosat, 1, 0", text)

    def test_no_uninitialized_predicate(self):
        result = self.compile("predicate::Predicate<Values> active;", "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("private constructor", result.stderr)

    def test_numeric_tile_is_not_predicate(self):
        result = self.compile(
            "predicate::scatter(destination, offsets, values, values);", "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no matching function", result.stderr)

    def test_shape_mismatch(self):
        result = self.compile('''auto active = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
VecTileM32<uint64_t, 32, 8> wrong;
predicate::scatter(destination, wrong, values, active);''', "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("valid shape must match", result.stderr)

    def test_builtin_wrong_arity(self):
        result = self.compile("ew_mscatter_add_predicate(1);", "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("10", result.stderr)


if __name__ == "__main__":
    unittest.main()
