#!/usr/bin/env python3
"""Public closed hierarchical ElementTile storage contracts."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class HierarchicalElementStorageAPITest(unittest.TestCase):
    def compile(self, code, *, run=False, accepts=True):
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(prefix="hierarchical-element-") as directory:
            source = Path(directory) / "contract.cpp"
            output = Path(directory) / "contract"
            source.write_text(
                "#include <common/pto_tile.hpp>\n"
                "#include <common/pto_tile_region.hpp>\n" + code
            )
            command = [
                compiler, "-std=c++20", "-D__linx", "-include",
                str(ROOT / "test/linx_host_type_shim.hpp"),
                "-I", str(ROOT / "include"), str(source),
            ]
            command += ["-o", str(output)] if run else ["-fsyntax-only"]
            result = subprocess.run(command, text=True, capture_output=True)
            if accepts:
                self.assertEqual(result.returncode, 0, result.stderr)
                if run:
                    executed = subprocess.run([str(output)], capture_output=True)
                    self.assertEqual(executed.returncode, 0, executed.stderr)
            else:
                self.assertNotEqual(result.returncode, 0, "invalid hierarchy compiled")

    def test_storage_profiles_and_logical_mapping(self):
        self.compile(r'''
#include <cassert>
#include <common/pto_tileop_api_revision.hpp>
using namespace pto;
#ifndef PTO_TILEOP_API_HAS_HIERARCHICAL_ELEMENT_STORAGE
#error "hierarchical ElementTile storage capability gate is required"
#endif

int main() {
  static_assert(sizeof(typename ElementTile<float,2048>::TileDType) == 8192);
  static_assert(sizeof(typename ElementTile<float,16384>::TileDType) == 65536);
  static_assert(std::is_same_v<
      typename TileArray<ElementTile<float,128>,1,16>::ParentCarrier,
      typename ElementTile<float,2048>::TileDType>);
  static_assert(std::is_same_v<
      typename TileArray<ElementTile<float,2048>,1,8>::ParentCarrier,
      typename ElementTile<float,16384>::TileDType>);
  static_assert(std::is_same_v<
      typename TileArray<ElementTile<__half,128>,1,16>::ParentCarrier,
      typename ElementTile<__half,2048>::TileDType>);
  static_assert(is_element_assembly_fragment_v<ElementTile<__half,128>>);
  static_assert(is_element_assembly_fragment_v<ElementTile<float,128>>);
  static_assert(is_element_assembly_fragment_v<ElementTile<float,2048>>);
  static_assert(is_element_assembly_fragment_v<ElementTile<uint32_t,128>>);
  static_assert(!is_element_assembly_fragment_v<int>);
  using OpenView = SubTileView<ElementTile<float,128>,ElementTile<float,32>>;
  static_assert(!is_element_assembly_fragment_v<OpenView>);

  ElementTile<float,16384> f32_parent;
  auto f32_parts = TPARTVIEW<32>(f32_parent, 777);
  static_assert(std::is_same_v<
      decltype(f32_parts.logical_index(std::size_t{}, uint32_t{})),
      uint32_t>);
  assert(f32_parts.size() == 512);
  for (unsigned part = 0; part < f32_parts.size(); ++part) {
    unsigned expected = part < 777 ? (777 - part + 511) / 512 : 0;
    assert(f32_parts.valid_size(part) == expected);
    for (unsigned element = 0; element < expected; ++element)
      assert(f32_parts.logical_index(part, element) == element * 512 + part);
  }

  ElementTile<__half,2048> f16_parent;
  auto f16_parts = TPARTVIEW<64>(f16_parent, 777);
  static_assert(std::is_same_v<
      decltype(f16_parts.logical_index(std::size_t{}, uint32_t{})),
      uint32_t>);
  assert(f16_parts.size() == 32);
  for (unsigned part = 0; part < f16_parts.size(); ++part) {
    for (unsigned element = 0; element < f16_parts.valid_size(part); ++element)
      assert(f16_parts.logical_index(part, element) ==
             (element / 2) * 64 + 2 * part + element % 2);
  }

  TileArray<ElementTile<float,32>,1,4> f32_level1;
  auto f32_closed128 =
      TASSEMBLY<ElementTile<float,128>>(std::move(f32_level1));
  TileArray<ElementTile<float,128>,1,16> f32_level2;
  auto f32_closed2048 =
      TASSEMBLY<ElementTile<float,2048>>(std::move(f32_level2));
  TileArray<ElementTile<float,2048>,1,8> f32_level3;
  auto f32_closed16384 =
      TASSEMBLY<ElementTile<float,16384>>(std::move(f32_level3));
  (void)f32_closed128;
  (void)f32_closed2048;
  (void)f32_closed16384;
}
''', run=True)

    def test_rejects_unsupported_hierarchies(self):
        for code in (
            "pto::ElementTile<__half,16384> tile;",
            "pto::ElementTile<uint32_t,4096> tile;",
            "pto::ElementTile<float,4096> tile;",
            "pto::TileArray<pto::ElementTile<float,32>,1,17> tile;",
            "void bad(){ pto::TileArray<pto::ElementTile<float,128>,1,16> a; "
            "auto x=pto::TASSEMBLY<pto::ElementTile<float,16384>>(std::move(a)); }",
            "void bad(){ pto::ElementTile<float,2048> p; "
            "auto x=pto::TPARTVIEW<64>(p,2048); (void)x; }",
        ):
            with self.subTest(code=code):
                self.compile(code, accepts=False)

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
            "--sysroot=" + sysroot, "-I", str(ROOT / "include"),
        ]

    def test_native_hierarchical_transport(self):
        compiler, common = self.native_common()
        source = ROOT / "test/tileop_api/src/ElementHierarchicalAssembly.cpp"
        with tempfile.TemporaryDirectory(prefix="native-element-hierarchy-") as directory:
            ir = Path(directory) / "hierarchy.ll"
            built = subprocess.run(
                common + ["-S", "-emit-llvm", str(source), "-o", str(ir)],
                text=True, capture_output=True,
            )
            self.assertEqual(built.returncode, 0, built.stderr)
            text = ir.read_text()
            self.assertEqual(text.count("#pragma pto element for"), 0)
            self.assertIn("B.ASSEMBLE", text)
            self.assertNotIn("TCVT", text)

            obj = Path(directory) / "hierarchy.o"
            built = subprocess.run(
                common + ["-c", str(source), "-o", str(obj)],
                text=True, capture_output=True,
            )
            self.assertEqual(built.returncode, 0, built.stderr)
            self.assertGreater(obj.stat().st_size, 0)
            dumped = subprocess.run(
                [str(Path(compiler).parent / "llvm-objdump"), "-d",
                 "--disassembler-options=no-tile-macros", str(obj)],
                text=True, capture_output=True,
            )
            self.assertEqual(dumped.returncode, 0, dumped.stderr)
            stores = re.findall(r"BSTART\.TLSU\s+TSTORE", dumped.stdout)
            self.assertEqual(len(stores), 2)
            self.assertRegex(dumped.stdout, r"B\.ASSEMBLE")

    def test_native_hierarchical_element_for(self):
        compiler, common = self.native_common()
        pragma_source = (
            ROOT / "test/tileop_api/src/ElementHierarchicalPragmaAssembly.cpp"
        )
        self.assertEqual(pragma_source.read_text().count("#pragma pto element for"), 2)
        with tempfile.TemporaryDirectory(
            prefix="native-element-pragma-hierarchy-"
        ) as directory:
            pragma_ir = Path(directory) / "pragma-hierarchy.ll"
            pragma_common = common + [
                "-fno-vectorize", "-fno-slp-vectorize", "-mllvm",
                "-linxv5-enable-generic-element=true",
            ]
            built = subprocess.run(
                pragma_common + ["-S", "-emit-llvm", str(pragma_source),
                                 "-o", str(pragma_ir)],
                text=True, capture_output=True,
            )
            self.assertEqual(built.returncode, 0, built.stderr)
            self.assertGreater(pragma_obj.stat().st_size, 0)
            dumped = subprocess.run(
                [str(Path(compiler).parent / "llvm-objdump"), "-d",
                 "--disassembler-options=no-tile-macros", str(pragma_obj)],
                text=True, capture_output=True,
            )
            self.assertEqual(dumped.returncode, 0, dumped.stderr)
            stores = re.findall(r"BSTART\.TLSU\s+TSTORE", dumped.stdout)
            self.assertEqual(len(stores), 2)
            self.assertRegex(dumped.stdout, r"B\.ASSEMBLE")
            pragma_obj = Path(directory) / "pragma-hierarchy.o"
            built = subprocess.run(
                pragma_common + ["-c", str(pragma_source),
                                 "-o", str(pragma_obj)],
                text=True, capture_output=True,
            )
            self.assertEqual(built.returncode, 0, built.stderr)


if __name__ == "__main__":
    unittest.main()
