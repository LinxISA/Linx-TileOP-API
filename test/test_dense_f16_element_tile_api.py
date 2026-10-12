#!/usr/bin/env python3
"""Dense F16/U16 logical64 ElementTile contracts."""

import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class DenseF16ElementTileAPITest(unittest.TestCase):
    def compile(self, code, *, run=False, accepts=True, release=False):
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(prefix="dense-f16-element-") as directory:
            source = Path(directory) / "contract.cpp"
            output = Path(directory) / "contract"
            source.write_text(
                "#include <common/pto_tile.hpp>\n"
                "#include <common/pto_tile_region.hpp>\n" + code
            )
            command = [
                compiler,
                "-std=c++20",
                "-D__linx",
                "-include",
                str(ROOT / "test/linx_host_type_shim.hpp"),
                "-I",
                str(ROOT / "include"),
                str(source),
            ]
            if release:
                command.append("-DNDEBUG")
            command += ["-o", str(output)] if run else ["-fsyntax-only"]
            result = subprocess.run(command, text=True, capture_output=True)
            if accepts:
                self.assertEqual(result.returncode, 0, result.stderr)
                if run:
                    executed = subprocess.run([str(output)], capture_output=True)
                    self.assertEqual(executed.returncode, 0, executed.stderr)
            else:
                self.assertNotEqual(result.returncode, 0, "invalid profile compiled")

    def test_exhaustive_dense_partition_mapping(self):
        self.compile(
            r'''
#include <cassert>
#include <common/pto_tileop_api_revision.hpp>
using namespace pto;
#ifndef PTO_TILEOP_API_HAS_DENSE_F16_U16_ELEMENT_TILE
#error "dense F16/U16 ElementTile capability gate is required"
#endif

static_assert(ElementTile<__half,64>::Rows == 32);
static_assert(ElementTile<__half,64>::Cols == 2);
static_assert(ElementTile<uint16_t,64>::Rows == 32);
static_assert(ElementTile<uint16_t,64>::Cols == 2);
static_assert(sizeof(typename ElementTile<__half,64>::TileDType) == 128);
static_assert(sizeof(typename ElementTile<uint16_t,64>::TileDType) == 128);
static_assert(sizeof(typename ElementTile<uint32_t,64>::TileDType) == 256);
static_assert(sizeof(typename ElementTile<int32_t,64>::TileDType) == 256);

template<class T, unsigned N>
void check_dense_parent() {
  static_assert(N == 64 || N == 128 || N == 512 || N == 2048);
  ElementTile<T,N> parent;
  T output[N]{};
  constexpr unsigned columns=N/32;
  constexpr unsigned part_count=N/64;
  using Parts=decltype(TPARTVIEW<64>(parent,N));
  static_assert(Parts::is_logical64_partition);
  static_assert(std::is_same_v<typename Parts::SubTileType,ElementTile<T,64>>);
  if constexpr (part_count <= 16) {
    static_assert(std::is_same_v<
        typename TileArray<ElementTile<T,64>,1,part_count>::ParentCarrier,
        typename ElementTile<T,N>::TileDType>);
  }

  for (unsigned valid=0; valid<=N; ++valid) {
    auto parts=TPARTVIEW<64>(parent,valid);
    assert(parts.size()==part_count);
    unsigned total=0;
    for (unsigned part=0; part<part_count; ++part) {
      unsigned expected=0;
      for (unsigned element=0; element<64; ++element) {
        const unsigned global=(element/2)*columns+2*part+(element%2);
        if (global<valid) {
          assert(parts.logical_index(part,element)==global);
          ++expected;
        }
      }
      assert(parts.valid_size(part)==expected);
      const uint64_t low=expected==0 ? 0 :
          ((uint64_t{1} << ((expected+1)/2))-1);
      const uint64_t high=expected<2 ? 0 :
          ((uint64_t{1} << (expected/2))-1);
      assert(parts.store_predicate(expected)==(low|(high<<32)));
      auto memory=parts.logical_region(output,part);
      assert(memory.data()==output+2*part);
      assert(memory.GetStrideBytes(3)==columns*sizeof(T));
      total+=expected;
    }
    assert(total==valid);
  }

  if constexpr (part_count <= 16) {
    TileArray<ElementTile<T,64>,1,part_count> fragments;
    auto assembled=TASSEMBLY<ElementTile<T,N>>(std::move(fragments));
    static_assert(std::is_same_v<decltype(assembled),ElementTile<T,N>>);
  }
}

template<class T>
void check_part_view() {
  ElementTile<T,64> part;
  auto& elements=TPARTELEMENT(part);
  static_assert(std::is_same_v<
      std::remove_reference_t<decltype(elements[0])>,T>);
  assert(&elements==&part.data());
}

int main() {
  check_dense_parent<__half,64>();
  check_dense_parent<__half,128>();
  check_dense_parent<__half,512>();
  check_dense_parent<__half,2048>();
  check_dense_parent<uint16_t,64>();
  check_dense_parent<uint16_t,128>();
  check_dense_parent<uint16_t,512>();
  check_dense_parent<uint16_t,2048>();
  check_dense_parent<uint32_t,64>();
  check_dense_parent<uint32_t,128>();
  check_dense_parent<uint32_t,512>();
  check_dense_parent<int32_t,64>();
  check_dense_parent<int32_t,128>();
  check_dense_parent<int32_t,512>();
  check_part_view<__half>();
  check_part_view<uint16_t>();
  check_part_view<uint32_t>();
  check_part_view<int32_t>();
}
''',
            run=True,
        )

    def test_tail_predicate_boundaries(self):
        self.compile(
            r'''
#include <cassert>
using namespace pto;
int main() {
  ElementTile<uint16_t,64> parent;
  auto parts=TPARTVIEW<64>(parent,64);
  struct Case { unsigned count; uint64_t mask; };
  const Case cases[] = {
    {0,0}, {1,1}, {2,uint64_t{1}|(uint64_t{1}<<32)},
    {31,0xffffu|(uint64_t{0x7fff}<<32)},
    {32,0xffffu|(uint64_t{0xffff}<<32)},
    {33,0x1ffffu|(uint64_t{0xffff}<<32)},
    {63,0xffffffffu|(uint64_t{0x7fffffff}<<32)},
    {64,~uint64_t{0}},
  };
  for (auto item:cases)
    assert(parts.store_predicate(item.count)==item.mask);
}
''',
            run=True,
        )

    def test_release_rejects_bad_dense_ranges(self):
        operations = (
            "auto parts=pto::TPARTVIEW<64>(parent,65); (void)parts;",
            "auto parts=pto::TPARTVIEW<64>(parent,64); (void)parts.part(1);",
            "auto parts=pto::TPARTVIEW<64>(parent,64); (void)parts.valid_size(1);",
            "auto parts=pto::TPARTVIEW<64>(parent,64); (void)parts.logical_index(0,64);",
            "auto parts=pto::TPARTVIEW<64>(parent,1); (void)parts.logical_index(0,1);",
            "auto parts=pto::TPARTVIEW<64>(parent,64); (void)parts.store_predicate(65);",
        )
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        for operation in operations:
            with self.subTest(operation=operation):
                with tempfile.TemporaryDirectory(prefix="dense-f16-reject-") as directory:
                    source = Path(directory) / "reject.cpp"
                    output = Path(directory) / "reject"
                    source.write_text(
                        "#include <common/pto_tile.hpp>\n"
                        "#include <common/pto_tile_region.hpp>\n"
                        "int main(){ pto::ElementTile<uint16_t,64> parent; "
                        + operation
                        + " return 0; }"
                    )
                    built = subprocess.run(
                        [
                            compiler,
                            "-std=c++20",
                            "-DNDEBUG",
                            "-D__linx",
                            "-include",
                            str(ROOT / "test/linx_host_type_shim.hpp"),
                            "-I",
                            str(ROOT / "include"),
                            str(source),
                            "-o",
                            str(output),
                        ],
                        text=True,
                        capture_output=True,
                    )
                    self.assertEqual(built.returncode, 0, built.stderr)
                    executed = subprocess.run([str(output)], capture_output=True)
                    self.assertNotEqual(executed.returncode, 0, "invalid range accepted")

    def test_rejects_invalid_dense_profiles(self):
        for code in (
            "pto::ElementTile<__half,32> tile;",
            "pto::ElementTile<__half,256> tile;",
            "pto::ElementTile<uint16_t,32> tile;",
            "pto::ElementTile<uint16_t,256> tile;",
            "pto::ElementTile<int16_t,64> tile;",
            "pto::TileArray<pto::ElementTile<uint16_t,64>,1,32> invalid;",
            "void bad(pto::ElementTile<__half,128>& p){ auto x=pto::TPARTVIEW<32>(p,128); }",
            "void bad(pto::ElementTile<float,128>& p){ auto x=pto::TPARTVIEW<64>(p,128); }",
            "void bad(pto::ElementTile<__half,128>& p){ auto& x=pto::TPARTELEMENT(p); }",
        ):
            with self.subTest(code=code):
                self.compile(code, accepts=False)

    def test_native_counted_store_ir_and_object(self):
        compiler = os.environ.get("LINX_CXX")
        resource = os.environ.get("LINX_RESOURCE_DIR")
        sysroot = os.environ.get("LINX_SYSROOT")
        if not (compiler and resource and sysroot):
            self.skipTest("set LINX_CXX/LINX_RESOURCE_DIR/LINX_SYSROOT")
        source = ROOT / "test/tileop_api/src/ElementDenseF16.cpp"
        with tempfile.TemporaryDirectory(prefix="native-dense-f16-") as directory:
            common = [
                compiler,
                "-std=c++20",
                "--target=linx64v5-unknown-linux-musl",
                "-D__linx",
                "-mlxbc",
                "-fenable-matrix",
                "-mllvm",
                "-enable-all-vector-as-tilereg=true",
                "-resource-dir=" + resource,
                "--sysroot=" + sysroot,
                "-I",
                str(ROOT / "include"),
            ]
            ir = Path(directory) / "dense.ll"
            emitted_ir = subprocess.run(
                common + ["-O0", "-S", "-emit-llvm", str(source), "-o", str(ir)],
                text=True,
                capture_output=True,
            )
            self.assertEqual(emitted_ir.returncode, 0, emitted_ir.stderr)
            ir_text = ir.read_text()
            self.assertIn("B.DIM zero, 2, ->lb0", ir_text)
            self.assertIn("ExecMaskPresent", ir_text)

            optimized_ir = Path(directory) / "dense-optimized.ll"
            emitted_optimized = subprocess.run(
                common + ["-O2", "-S", "-emit-llvm", str(source), "-o", str(optimized_ir)],
                text=True, capture_output=True,
            )
            self.assertEqual(emitted_optimized.returncode, 0, emitted_optimized.stderr)
            optimized_text = optimized_ir.read_text()
            zero_body = next(body for body in re.split(r"(?=^define )", optimized_text,
                                                      flags=re.MULTILINE)
                             if body.startswith("define ") and "zero_count_store" in body)
            zero_body = zero_body.split("\n}", 1)[0]
            self.assertIn("ret void", zero_body)
            self.assertNotRegex(zero_body, r"\b(?:load|store|getelementptr|call)\b")
            bodies = [body.split("\n}", 1)[0] for body in
                      re.split(r"(?=^define )", optimized_text, flags=re.MULTILINE)
                      if body.startswith("define ") and "copy_dense_partI" in body]
            self.assertEqual(len(bodies), 4)
            for body in bodies:
                self.assertIn("BSTART.TLSU TMOV", body)
                self.assertIn("B.DATR CUBE_M32, Null", body)
                self.assertIn("B.SUBVIEW", body)
                self.assertIn("B.DIM zero, ${6:c}, ->lb2", body)
                self.assertRegex(body, r"i32 2, i32 32, i32 2,")

            direct = [body.split("\n}", 1)[0] for body in
                      re.split(r"(?=^define )", optimized_text, flags=re.MULTILINE)
                      if body.startswith("define ") and "copy_dense_tileI" in body]
            self.assertEqual(len(direct), 4)
            for body in direct:
                self.assertIn("BSTART.TLSU TMOV", body)
                self.assertIn("B.DATR CUBE_M32, Null", body)
                self.assertIn("B.DIM zero, ${6:c}, ->lb2", body)
                self.assertRegex(body, r"i32 2, i32 32, i32 2, i32 29\)")

            obj = Path(directory) / "dense.o"
            emitted_obj = subprocess.run(
                common + ["-O2", "-c", str(source), "-o", str(obj)],
                text=True,
                capture_output=True,
            )
            self.assertEqual(emitted_obj.returncode, 0, emitted_obj.stderr)
            self.assertGreater(obj.stat().st_size, 0)
            dumped = subprocess.run(
                [str(Path(compiler).parent / "llvm-objdump"), "-d",
                 "--disassembler-options=no-tile-macros", str(obj)],
                text=True, capture_output=True,
            )
            self.assertEqual(dumped.returncode, 0, dumped.stderr)
            move_blocks = re.findall(r"BSTART\.TLSU\s+TMOV,[\s\S]*?(?=BSTART|$)", dumped.stdout)
            self.assertEqual(len(move_blocks), 8)
            for block in move_blocks:
                self.assertRegex(block, r"B\.DATR\s+CUBE_M32\.normal, Null")
                self.assertRegex(block, r"C\.B\.DIMI\s+2,\s*->lb2")

            for invalid in ("N8", "BOXED", "COL_MAJOR"):
                rejected = subprocess.run(
                    common + ["-O2", "-fsyntax-only", "-DPTO_DENSE_REJECT_" + invalid + "_TMOV",
                              str(source)], text=True, capture_output=True,
                )
                self.assertNotEqual(rejected.returncode, 0, invalid)
                self.assertIn("no matching function for call to 'TMOV'", rejected.stderr)



if __name__ == "__main__":
    unittest.main()
