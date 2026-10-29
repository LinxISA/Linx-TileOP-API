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

    def test_atomic_tstore_execution_and_pe_masks(self):
        body = '''auto active = predicate::compare_scalar<CmpMode::GE>(values, threshold);
TSTORE_ATOMIC_ADD<1>(destination, values, active);
TSTORE_ATOMIC_MAX<4, true>(destination, values, active);
TSTORE_ATOMIC_DEC<0>(destination, values, active);'''
        obj = self.output / "atomic-tstore.o"
        result = self.compile(body, "-c", "-o", str(obj))
        self.assertEqual(result.returncode, 0, result.stderr)
        text = subprocess.check_output([
            str(CXX.parent / "llvm-objdump"), "-d",
            "--disassembler-options=no-tile-macros", str(obj)], text=True)
        self.assertEqual(text.count("BSTART.TLSU\tTSTORE"), 3, text)
        self.assertIn("mask=0001", text)
        self.assertIn("mask=0100", text)
        self.assertIn("mask=0000", text)
        self.assertIn("nosat, 1, 0", text)
        self.assertNotIn("nosat, 1, 1", text)

    def test_cooperative_cube_shared_publishes_valid_shape(self):
        source = self.output / "cube-shared-valid.cpp"
        source.write_text('''#include <common/pto_tileop.hpp>
using namespace pto;
using Cube = VecTileM32<uint32_t, 32, 3>;
using ParentLocal = Tile<Location::Left, uint32_t, 64, 4, BLayout::RowMajor>;
using Parent = SharedTile<ParentLocal>;
using Global = global_tensor<uint32_t, RowMajor<64, 4>>;
void test(Global &destination, Cube &source) {
  Parent parent;
  auto init = range::assemble<4, 0>(parent);
  TMOV_CUBE_TO_SHARED_ND<1>(init, source);
  auto last = range::assemble_last<4, 4>(parent);
  TMOV_CUBE_TO_SHARED_ND<1>(last, source);
  TSTORE_ATOMIC_ADD<1>(destination, parent);
}
''')
        obj = self.output / "cube-shared-valid.o"
        result = subprocess.run(self.flags + ["-c", str(source), "-o", str(obj)],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        text = subprocess.check_output([
            str(CXX.parent / "llvm-objdump"), "-d",
            "--disassembler-options=no-tile-macros", str(obj)], text=True)
        self.assertRegex(text, r"C\.B\.DIMI\s+3,\s+->lb0")
        self.assertRegex(text, r"C\.B\.DIMI\s+64,\s+->lb1")
        self.assertRegex(text, r"C\.B\.DIMI\s+4,\s+->lb2")

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

    def test_logical_selection_and_memory(self):
        body = '''auto first = predicate::compare_scalar<CmpMode::GE>(values, threshold);
auto second = predicate::compare_scalar<CmpMode::NE>(values, threshold);
auto both = predicate::logical_and(first, second);
auto either = predicate::logical_or(first, second);
auto different = predicate::logical_xor(both, either);
static_assert(std::is_same_v<typename decltype(both)::BasisTile::DType, uint8_t>);
predicate::select(values, different, values, values);
predicate::select_scalar(values, both, values, threshold);
predicate::scatter(destination, offsets, values, either);
predicate::atomic_add(destination, offsets, values, different);'''
        obj = self.output / "logical.o"
        result = self.compile(body, "-c", "-o", str(obj))
        self.assertEqual(result.returncode, 0, result.stderr)
        text = subprocess.check_output([
            str(CXX.parent / "llvm-objdump"), "-d",
            "--disassembler-options=no-tile-macros", str(obj)], text=True)
        for operation in ("TAND", "TOR", "TXOR", "TSEL", "TSELS",
                          "MSCATTER.MASK", "MSCATTER.ADD"):
            self.assertIn(operation, text)
        self.assertNotIn("TCVT", text)

    def test_cross_basis_and_typed_views(self):
        obj = self.output / "views.o"
        result = self.compile('''VecTileM32<float, 32, 8> floats;
auto first = predicate::compare_scalar<CmpMode::GE>(values, threshold);
auto second = predicate::compare_scalar<CmpMode::LT>(floats, 1.0f);
auto combined = predicate::logical_and(first, second);
auto low = predicate::subview<0, 4>(combined);
auto high = predicate::subview<1, 4>(combined);
auto whole = predicate::concatenate(low, high);
predicate::scatter(destination, offsets, values, whole);''',
            "-c", "-o", str(obj), columns=8)
        self.assertEqual(result.returncode, 0, result.stderr)
        text = subprocess.check_output([
            str(CXX.parent / "llvm-objdump"), "-d",
            "--disassembler-options=no-tile-macros", str(obj)], text=True)
        self.assertEqual(text.count("B.SUBVIEW"), 2, text)
        self.assertEqual(text.count("B.ASSEMBLE"), 2, text)
        source = self.output / "predicate.cpp"
        assembly = self.output / "views.s"
        result = subprocess.run(self.flags + ["-S", str(source), "-o", str(assembly)],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        result = subprocess.run([
            str(CXX.parent / "llvm-mc"), "-triple=linx64v5", "-filetype=obj",
            str(assembly), "-o", str(self.output / "views.roundtrip.o")],
            text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_numeric_is_not_logical_predicate(self):
        result = self.compile('''auto active = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
auto invalid = predicate::logical_and(active, values);''', "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no matching function", result.stderr)

    def test_subview_bounds(self):
        result = self.compile('''auto active = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
auto invalid = predicate::subview<1, 4>(active);''', "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must fit the source valid region", result.stderr)

    def test_mixed_result_is_numeric(self):
        result = self.compile('''auto active = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
VecTileM32<int8_t, 32, 4> numeric;
auto result = predicate::logical_numeric<6>(active, numeric);
static_assert(std::is_same_v<typename decltype(result)::DType, uint8_t>);
predicate::scatter(destination, offsets, values, result);''', "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("no matching function", result.stderr)

    def test_mixed_numeric_object(self):
        result = self.compile('''auto active = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
VecTileM32<int8_t, 32, 4> numeric;
auto result = predicate::logical_numeric<8>(active, numeric);
global_tensor<uint8_t, RowMajor<32, 4>> bytes(reinterpret_cast<uint8_t *>(destination.data()));
TSTORE(bytes, result);''', "-c", "-o", str(self.output / "mixed.o"))
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_logical_shape_mismatch(self):
        result = self.compile('''auto first = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
VecTileM32<uint32_t, 32, 8> wider;
auto second = predicate::compare_scalar<CmpMode::EQ>(wider, threshold);
auto invalid = predicate::logical_or(first, second);''', "-fsyntax-only")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("valid shapes must match", result.stderr)

    def test_cross_basis_selection(self):
        result = self.compile('''auto active = predicate::compare_scalar<CmpMode::EQ>(values, threshold);
VecTileM32<float, 32, 4> selected;
predicate::select(selected, active, selected, selected);
predicate::select_scalar(selected, active, selected, 0.0f);''',
            "-c", "-o", str(self.output / "cross-basis.o"))
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_m16_memory_is_rejected(self):
        source = self.output / "m16-memory.cpp"
        source.write_text('''#include <common/pto_tileop.hpp>
using namespace pto;
using Values = Tile<Location::Vec, uint32_t, 16, 8, BLayout::CubeM16>;
using Offsets = VecTileM32<uint64_t, 32, 8>;
using Global = global_tensor<uint32_t, RowMajor<16, 8>>;
void test(Values &values, Offsets &offsets, Global &destination) {
auto active = predicate::compare_scalar<CmpMode::EQ>(values, 0u);
predicate::scatter(destination, offsets, values, active);
}
''')
        result = subprocess.run(self.flags + ["-fsyntax-only", str(source)],
                                text=True, capture_output=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("indexed memory currently requires CUBE_M32", result.stderr)

    def test_m16_consumers(self):
        source = self.output / "m16.cpp"
        source.write_text('''#include <common/pto_tileop.hpp>
using namespace pto;
using Values = Tile<Location::Vec, uint32_t, 16, 8, BLayout::CubeM16>;
using Global = global_tensor<uint32_t, RowMajor<16, 8>>;
void test(Values &values, Global &destination, uint32_t threshold) {
auto first = predicate::compare_scalar<CmpMode::GE>(values, threshold);
auto combined = predicate::logical_xor(first, first);
predicate::select(values, combined, values, values);
predicate::select_scalar(values, combined, values, threshold);
auto view = predicate::subview<0, 8>(combined);
auto whole = predicate::concatenate(view, view);
}
''')
        result = subprocess.run(self.flags + ["-c", str(source), "-o",
                                str(self.output / "m16.o")],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
