#!/usr/bin/env python3
"""PR #423 API geometry, diagnostics and actual Linx frontend/lowering checks.

Host checks exercise range/shape validation only; they do not simulate TMOV.
Native object checks require LLVM's PR #423 selector gate update.
"""
import os
import importlib.util
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "test/tileop_api/src"
NEGATIVES = {
    "DTYPE": "preserves the backing dtype",
    "COLUMNS": "matching physical columns",
    "PARENT": "fragment must equal destination capacity",
    "N8": "requires Local CUBE_M32 or CUBE_M16",
    "LAYOUT": "source must be RowMajor Shared ND",
    "RCPE": "rejects RCPE6M2 backing",
    "M16_B64": "64-bit CUBE_M16 layouts are not supported",
    "OFFSET": "subview must start at a complete row",
    "RANGE": "range must fit the parent capacity",
    "LENGTH": "fragment must equal destination capacity",
    "SIZE_CODE": "length must be 128B..256KiB power of two",
    "MASK": "requires an encodable PE mask",
    "ASSEMBLE": "no matching function",
    "SLOT": "requires a compiler-managed SharedTile",
    "OLD_SUBVIEW": "no matching function",
    "TEMPORARY": "deleted function",
}


class HostGeometry(unittest.TestCase):
    def test_complete_rows_tails_and_overflow(self):
        source = r'''
#include <common/pto_shared_nd.hpp>
#include <cassert>
#include <limits>
using namespace pto;
using Parent = SharedTile<Tile<Location::Vec, uint8_t, 128, 128>>;
using Cube = VecTileM32<uint8_t, 32, 128, DYNAMIC, DYNAMIC>;
using C = detail::SharedNDToCubeContract<Cube, Parent, true, 4096, 0, true>;
using P16 = SharedTile<Tile<Location::Left, uint16_t, 64, 64>>;
using M16 = CubeTileM16<uint16_t, 16, 64>;
using C16 = detail::SharedNDToCubeContract<M16, P16, true, 2048, 0, true>;
using PartialParent = SharedTile<Tile<Location::Vec, uint8_t, 64, 256>>;
using Wide = VecTileM32<uint8_t, 32, 256>;
using WideC = detail::SharedNDToCubeContract<Wide, PartialParent, true, 8192, 0, true>;
using OffsetC = detail::SharedNDToCubeContract<Wide, PartialParent, true, 8192, 2, true>;
using B64 = VecTileM32<uint64_t, 32, 1>;
using S64 = SharedTile<Tile<Location::Vec, uint64_t, 32, 1>>;
using C64 = detail::SharedNDToCubeContract<B64, S64, false, 256>;
using Packed = VecTileM16<__uint4x2, 16, 32, 11, 17>;
using PackedS = SharedTile<Tile<Location::Vec, __uint4x2, 16, 32>>;
using PackedC = detail::SharedNDToCubeContract<Packed, PackedS, false, 256>;
int main() {
  // Permuted PE ranges are independent of PE identity and arrival order.
  for (uintptr_t offset : {64u, 0u, 96u, 32u}) {
    auto s = C::shape(offset, 128, 128);
    assert(s.valid && s.rows == 32 && s.columns == 128);
  }
  auto tail = C::shape(96, 111, 117);
  assert(tail.valid && tail.rows == 15 && tail.columns == 117);
  // Row-aligned starts need not be multiples of M.
  assert(C::shape(1, 128, 128).valid);
  assert(!C::shape(96, 96, 128).valid);
  assert(!C::shape(97, 128, 128).valid);
  assert(!C::shape(0, 0, 128).valid);
  assert(!C::shape(0, 128, 0).valid);
  assert(!C::shape(0, 129, 128).valid);
  assert(!C::shape(0, 128, 129).valid);
  assert(!C::shape(2048, 128, 128).valid);
  assert(!C::shape(std::numeric_limits<uintptr_t>::max(), 128, 128).valid);
  assert(!WideC::shape(1, 64, 256).valid); // half-row start
  assert(WideC::shape(2, 64, 256).valid);
  assert(OffsetC::shape(0, 64, 256).valid);
  assert(!OffsetC::shape(2046, 64, 256).valid);
  for (uintptr_t offset : {0u, 16u, 32u, 48u})
    assert(C16::shape(offset, 64, 64).valid);
  auto tail16 = C16::shape(48, 59, 61);
  assert(tail16.valid && tail16.rows == 11 && tail16.columns == 61);
  assert(C64::shape(0, 32, 1).valid);
  auto packed = PackedC::shape(0, 11, 17);
  assert(packed.valid && packed.rows == 11 && packed.columns == 17);
  Parent parent;
  auto static_view = range::shared_nd_subview<32, 64>(parent);
  auto dynamic_view = range::shared_nd_subview<32>(parent, 96);
  static_assert(decltype(static_view)::SubviewSizeCode == 6);
  static_assert(!is_tile<decltype(static_view)>::value);
  static_assert(!range::is_legal_subview_parent_v<Parent>);
  assert(&static_view.parent() == &parent && static_view.GetRangeBase() == 0);
  assert(dynamic_view.GetRangeBase() == 96);
}
'''
        # initializer_list is explicit so this header remains self-contained.
        source = '#include <initializer_list>\n' + source
        with tempfile.TemporaryDirectory(prefix="shared-nd-host-") as temp:
            path = Path(temp)
            (path / "geometry.cpp").write_text(source)
            result = subprocess.run([
                os.environ.get("CXX", "c++"), "-std=c++20", "-D__linx",
                "-include", str(ROOT / "test/linx_host_type_shim.hpp"),
                "-I", str(ROOT / "include"), str(path / "geometry.cpp"),
                "-o", str(path / "geometry"),
            ], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            ran = subprocess.run([str(path / "geometry")], capture_output=True)
            self.assertEqual(ran.returncode, 0, ran.stderr)

    def test_runtime_guards_and_metadata_in_release(self):
        # Strip target asm only; execute the real API's validation, PE gating
        # and metadata updates. This is not payload execution evidence.
        spec = importlib.util.spec_from_file_location(
            "doc_examples", ROOT / "tools/check_tileop_usage_examples.py")
        module = importlib.util.module_from_spec(spec)
        import sys
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        prefix = r'''
#include <common/pto_tileop.hpp>
using namespace pto;
using ND = Tile<Location::Vec, uint8_t, 64, 256, BLayout::RowMajor, DYNAMIC, DYNAMIC>;
using Shared = SharedTile<ND>;
using DynamicCube = VecTileM32<uint8_t, 32, 256, DYNAMIC, DYNAMIC>;
using StaticCube = VecTileM32<uint8_t, 32, 256>;
struct ValidShape {
  int rows, columns;
  int GetValidRow() const { return rows; }
  int GetValidCol() const { return columns; }
};
int main() {
  Shared shared;
  shared.SetValidShape(ValidShape{51, 233});
  DynamicCube dst;
  dst.RowMaskInternal = 7;
  dst.ColMaskInternal = 9;
'''
        cases = {
            "tail": (r'''
  auto view = range::shared_nd_subview<64>(shared, 64);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
  return dst.GetValidRow() != 19 || dst.GetValidCol() != 233;
''', True),
            "static_current_metadata": (r'''
  using Initial = Tile<Location::Vec, uint8_t, 32, 256>;
  SharedTile<Initial> current;
  current.SetValidShape(ValidShape{17, 201});
  TMOV_SHARED_ND_TO_CUBE(dst, current);
  return dst.GetValidRow() != 17 || dst.GetValidCol() != 201;
''', True),
            "half_row": (r'''
  auto view = range::shared_nd_subview<64>(shared, 1);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
''', False),
            "out_of_bounds": (r'''
  auto view = range::shared_nd_subview<64>(shared, 66);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
''', False),
            "overflow": (r'''
  auto view = range::shared_nd_subview<64>(shared, ~uintptr_t{0});
  TMOV_SHARED_ND_TO_CUBE(dst, view);
''', False),
            "empty_valid_fragment": (r'''
  shared.SetValidShape(ValidShape{32, 233});
  auto view = range::shared_nd_subview<64>(shared, 64);
  TMOV_SHARED_ND_TO_CUBE(dst, view);
''', False),
            "static_destination_mismatch": (r'''
  StaticCube fixed;
  auto view = range::shared_nd_subview<64>(shared, 64);
  TMOV_SHARED_ND_TO_CUBE(fixed, view);
''', False),
            "zero": (r'''
  shared.SetValidShape(ValidShape{-1, 233});
  auto view = range::shared_nd_subview<64>(shared, ~uintptr_t{0});
  TMOV_SHARED_ND_TO_CUBE<0>(dst, view);
  return dst.GetValidRow() != 7 || dst.GetValidCol() != 9;
''', True),
            "nonparticipant": (r'''
  shared.SetValidShape(ValidShape{-1, 233});
  auto view = range::shared_nd_subview<64>(shared, ~uintptr_t{0});
  // Host shim is PE0 (mask bit 8); mask=0001 selects PE3.
  TMOV_SHARED_ND_TO_CUBE<1>(dst, view);
  return dst.GetValidRow() != 7 || dst.GetValidCol() != 9;
''', True),
        }
        with tempfile.TemporaryDirectory(prefix="shared-nd-guards-") as temp:
            path = Path(temp)
            module.host_include_tree(path / "include")
            for name, (body, succeeds) in cases.items():
                with self.subTest(case=name):
                    source, output = path / (name + ".cpp"), path / name
                    source.write_text(prefix + body + "\n}\n")
                    result = subprocess.run([
                        os.environ.get("CXX", "c++"), "-std=c++20", "-DNDEBUG",
                        "-D__linx", "-include", str(ROOT / "test/linx_host_type_shim.hpp"),
                        "-I", str(path / "include"), str(source), "-o", str(output),
                    ], text=True, capture_output=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    ran = subprocess.run([str(output)], capture_output=True)
                    self.assertEqual(ran.returncode == 0, succeeds, name)


class NativeLowering(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = os.environ.get("LINX_CXX")
        if not cls.compiler and os.environ.get("COMPILER_DIR"):
            cls.compiler = str(Path(os.environ["COMPILER_DIR"]) / "clang++")
        if not cls.compiler:
            raise unittest.SkipTest("set LINX_CXX or COMPILER_DIR for Linx checks")
        cls.work = tempfile.TemporaryDirectory(prefix="shared-nd-native-")
        cls.directory = Path(cls.work.name)
        resource = Path(subprocess.check_output(
            [cls.compiler, "-print-resource-dir"], text=True).strip())
        overlay = cls.directory / "resource"
        shutil.copytree(resource, overlay, ignore=lambda directory, names:
                        ["tileop-api"] if Path(directory) == resource / "include" else [])
        shutil.copytree(ROOT / "include", overlay / "include/tileop-api")
        cls.flags = [cls.compiler, "--target=linx64v5-unknown-linux-musl",
                     "-std=c++20", "-O2", "-mlxbc", "-fenable-matrix",
                     "-D__linx", "-DENABLE_TENSOR_INSTR",
                     "-resource-dir", str(overlay)]
        if os.environ.get("LINX_SYSROOT"):
            sysroot = Path(os.environ["LINX_SYSROOT"])
            cls.flags += ["--sysroot=" + str(sysroot), "-nostdinc++",
                          "-isystem", str(sysroot / "usr/include/c++/v1")]

    @classmethod
    def tearDownClass(cls):
        cls.work.cleanup()

    def compile(self, *args):
        result = subprocess.run(self.flags + list(args), text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_negative_diagnostics(self):
        source = str(FIXTURES / "SharedNDToCubeNegatives.cpp")
        self.compile("-fsyntax-only", source)
        self.compile("-fsyntax-only", str(FIXTURES / "SharedNDToCube.cpp"))
        for case, diagnostic in NEGATIVES.items():
            with self.subTest(case=case):
                result = subprocess.run(self.flags + [
                    "-fsyntax-only", "-DSHOULD_FAIL_423_" + case, source,
                ], text=True, capture_output=True)
                self.assertNotEqual(result.returncode, 0, case)
                self.assertIn(diagnostic, result.stderr)

    def test_ir_binders_geometry_and_noop(self):
        output = self.directory / "reverse.ll"
        self.compile("-S", "-emit-llvm", str(FIXTURES / "SharedNDToCube.cpp"),
                     "-o", str(output))
        sections = dict(re.findall(
            r"^define[^\n]*@(shared_nd_\w+)\([^\n]*\).*?\{\n(.*?)^\}",
            output.read_text(), re.M | re.S))
        self.assertEqual(len(sections), 9)
        for name, body in sections.items():
            if name == "shared_nd_zero":
                self.assertNotRegex(body, r"asm |load |store |llvm\.trap|call ")
                continue
            reverse = [line for line in body.splitlines()
                       if "asm sideeffect" in line and "B.DATR ND2M32" in line]
            self.assertEqual(len(reverse), 1, name)
            line = reverse[0]
            self.assertIn("BSTART.TMOV DTYPE_NONE", line)
            self.assertNotIn("->lb2", line)
            self.assertNotIn("B.ASSEMBLE", line)
            self.assertIn("=@2Tr,@2Sr", line)
            self.assertLess(line.index("B.IOS"), line.index("B.IOT"))
            if name in ("shared_nd_four_pe_m32", "shared_nd_four_pe_m16",
                        "shared_nd_static_offset", "shared_nd_dynamic_tail"):
                self.assertLess(line.index("B.IOS"), line.index("B.SUBVIEW"))
                self.assertLess(line.index("B.SUBVIEW"), line.index("B.IOT"))
            else:
                self.assertNotIn("B.SUBVIEW", line)
        for name, values in {
            "shared_nd_singleton_m32": (8, 21, 6, 128, 32),
            "shared_nd_singleton_m16": (8, 22, 5, 64, 16),
            "shared_nd_dynamic_tail": (8, 21, 3, 3, 19),
            "shared_nd_packed_tail": (8, 22, 2, 17, 11),
            "shared_nd_b64": (8, 21, 2, 1, 32),
        }.items():
            tail = ", ".join("i32 " + str(v) for v in values)
            reverse = next(line for line in sections[name].splitlines()
                           if "asm sideeffect" in line and "B.DATR ND2M32" in line)
            self.assertIn(tail, reverse, name)
        self.assertIn("store i32 19", sections["shared_nd_dynamic_tail"])
        self.assertIn("store i32 3", sections["shared_nd_dynamic_tail"])
        self.assertIn("@llvm.trap", sections["shared_nd_four_pe_m32"])

    def test_assembly_register_binding(self):
        # Exercise register allocation even with a compiler whose integrated
        # assembler has not yet enabled the two reverse TMOV selectors.
        output = self.directory / "reverse.s"
        self.compile("-S", "-fno-integrated-as",
                     str(FIXTURES / "SharedNDToCube.cpp"), "-o", str(output))
        text = output.read_text()
        self.assertRegex(text, r"B\.IOS S\d+, mask=1111")
        self.assertRegex(text, r"B\.SUBVIEW 0, \w+, 0, 6")
        self.assertIn("B.SUBVIEW 0, zero, 64, 6", text)
        self.assertIn("B.DATR ND2M32, DTYPE_NONE, Zero", text)
        self.assertIn("B.DATR ND2M16, DTYPE_NONE, Zero", text)

    def test_object_with_reverse_capable_compiler(self):
        mc = Path(self.compiler).parent / "llvm-mc"
        probe = subprocess.run([str(mc), "-triple=linx64v5", "-filetype=obj",
                                "-o", os.devnull], input=
                               "BSTART.TMOV DTYPE_NONE\n"
                               "B.DATR ND2M32, DTYPE_NONE, Zero\n"
                               "BSTART.TMOV DTYPE_NONE\n"
                               "B.DATR ND2M16, DTYPE_NONE, Zero\n",
                               text=True, capture_output=True)
        if probe.returncode != 0:
            if os.environ.get("PTO_REQUIRE_SHARED_ND_OBJECT") == "1":
                self.fail(probe.stderr)
            self.assertIn("Match Instruction", probe.stderr)
            self.skipTest("LLVM must enable TMOV Layout 21/22 (PTO PR #423); "
                          "object code has not been validated")
        output = self.directory / "reverse.o"
        self.compile("-c", str(FIXTURES / "SharedNDToCube.cpp"), "-o", str(output))
        objdump = Path(self.compiler).parent / "llvm-objdump"
        result = subprocess.run([str(objdump), "-d", "--disassembler-options=no-tile-macros",
                                 str(output)], text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("ND2M32.normal", result.stdout)
        self.assertIn("ND2M16.normal", result.stdout)


if __name__ == "__main__":
    unittest.main()
