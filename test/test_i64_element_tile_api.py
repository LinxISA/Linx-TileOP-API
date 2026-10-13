#!/usr/bin/env python3
"""PTO 8f589b7 native S64/U64 CUBE_M32 leaf contracts."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC_PIN = "8f589b70085a37c70f210898b168aae634da5ac1"


class I64ElementTileAPITest(unittest.TestCase):
    def compile(self, code, *, accepts=True, run=False):
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(prefix="i64-element-tile-") as directory:
            source = Path(directory) / "contract.cpp"
            output = Path(directory) / "contract"
            source.write_text(
                "#include <common/pto_tile.hpp>\n"
                "#include <common/pto_tile_region.hpp>\n" + code)
            command = [compiler, "-std=c++20", "-D__linx", "-include",
                       str(ROOT / "test/linx_host_type_shim.hpp"), "-I",
                       str(ROOT / "include"), str(source)]
            command += ["-o", str(output)] if run else ["-fsyntax-only"]
            result = subprocess.run(command, text=True, capture_output=True)
            if accepts:
                self.assertEqual(result.returncode, 0, result.stderr)
                if run:
                    executed = subprocess.run([str(output)], capture_output=True)
                    self.assertEqual(executed.returncode, 0, executed.stderr)
            else:
                self.assertNotEqual(result.returncode, 0,
                                    "invalid native64 profile compiled")

    def test_leaf_geometry_and_raw_plane_reference(self):
        self.compile(r'''
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <limits>
#include <common/pto_tileop_api_revision.hpp>
#ifndef PTO_TILEOP_API_HAS_I64_ELEMENT_TILE_LEAF32
#error "native64 leaf32 feature gate is required"
#endif
using namespace pto;
using U64Leaf=ElementTile<uint64_t,32>;
using S64Leaf=ElementTile<int64_t,32>;
static_assert(U64Leaf::Rows==32 && U64Leaf::Cols==1);
static_assert(U64Leaf::ValidRow==32 && U64Leaf::ValidCol==1);
static_assert(U64Leaf::LogicalTileBytes==256);
static_assert(U64Leaf::TilesizeCode==2);
static_assert(U64Leaf::IsCubeM32B64);
static_assert(U64Leaf::CubeCellRows==32);
static_assert(U64Leaf::CubeCellCols==1);
static_assert(U64Leaf::CubeCellCount==2);
static_assert(U64Leaf::CubePhysicalCellsPerLogicalColumn==2);
static_assert(U64Leaf::CubeStorageIndex(0,0)==0);
static_assert(U64Leaf::CubeStorageIndex(31,0)==31);
static_assert(U64Leaf::CubePhysicalCellIndex(0,false)==0);
static_assert(U64Leaf::CubePhysicalCellIndex(0,true)==1);
static_assert(U64Leaf::CubePhysicalPlaneByteOffset(7,0,false)==28);
static_assert(U64Leaf::CubePhysicalPlaneByteOffset(7,0,true)==156);
static_assert(S64Leaf::CubePhysicalPlaneByteOffset(7,0,false)==28);
static_assert(S64Leaf::CubePhysicalPlaneByteOffset(7,0,true)==156);
static_assert(S64Leaf::CubePhysicalPlaneWord(
                  UINT64_C(0x1122334455667788),false)==0x55667788);
static_assert(S64Leaf::CubePhysicalPlaneWord(
                  UINT64_C(0x1122334455667788),true)==0x11223344);
static_assert(U64Leaf::CubePhysicalCellRangeComplete(0,2));
static_assert(!U64Leaf::CubePhysicalCellRangeComplete(0,1));
static_assert(!U64Leaf::CubePhysicalCellRangeComplete(1,2));
static_assert(is_element_tile_v<U64Leaf> && is_element_tile_v<S64Leaf>);
template<class T> concept HasElements=requires(T& tile){TPARTELEMENT(tile);};
static_assert(HasElements<U64Leaf> && HasElements<S64Leaf>);

using ExistingN8=CubeTileN8<uint64_t,2,8>;
static_assert(ExistingN8::CubeCellRows==2);
static_assert(ExistingN8::CubeCellCols==8);
static_assert(ExistingN8::CubeCellCount==1);
static_assert(ExistingN8::LogicalTileBytes==128);
using ShortValidM32=VecTileM32<uint64_t,32,1,16,1>;
static_assert(ShortValidM32::Rows==32 && ShortValidM32::ValidRow==16);
static_assert(ShortValidM32::LogicalTileBytes==256);

constexpr std::array<uint64_t,8> edge={
  0, UINT64_C(0x8000000000000000), UINT64_MAX,
  UINT64_C(0x1122334455667788), UINT64_C(0x5566778811223344),
  UINT64_C(0xffffffff00000000), UINT64_C(0x00000000ffffffff), 1};

int main(){
  std::array<uint32_t,64> physical{};
  for(unsigned row=0;row<32;++row){
    uint64_t value=edge[row%edge.size()];
    auto low=U64Leaf::CubePhysicalPlaneWord(value,false);
    auto high=U64Leaf::CubePhysicalPlaneWord(value,true);
    assert(low==static_cast<uint32_t>(value));
    assert(high==static_cast<uint32_t>(value>>32));
    physical[U64Leaf::CubePhysicalPlaneByteOffset(row,0,false)/4]=low;
    physical[U64Leaf::CubePhysicalPlaneByteOffset(row,0,true)/4]=high;
  }
  for(unsigned row=0;row<32;++row){
    uint64_t value=edge[row%edge.size()];
    assert(physical[row]==static_cast<uint32_t>(value));
    assert(physical[32+row]==static_cast<uint32_t>(value>>32));
  }
  U64Leaf u;
  S64Leaf s;
  auto &uv=TPARTELEMENT(u);
  auto &sv=TPARTELEMENT(s);
  uv[0]=edge[1];
  sv[0]=std::bit_cast<int64_t>(edge[1]);
  assert(uv[0]==edge[1]);
  assert(std::bit_cast<uint64_t>(sv[0])==edge[1]);
}
''', run=True)

    def test_integer_parent_geometry_and_partitions(self):
        self.compile(r'''
#include <cassert>
using namespace pto;
template<class T, int N> void check() {
  using Parent = ElementTile<T,N>;
  static_assert(Parent::LogicalTileBytes == N * sizeof(T));
  Parent parent;
  for (unsigned valid : {0u,1u,31u,32u,unsigned(N-1),unsigned(N)}) {
    auto parts = TPARTVIEW<32>(parent,valid);
    unsigned total = 0;
    for (unsigned p = 0; p < N/32; ++p) {
      auto part = parts.part(p);
      assert(part.GetRangeBase() == p * sizeof(T)/4);
      for(unsigned e = 0; e < parts.valid_size(p); ++e) {
        assert(parts.logical_index(p,e) == e*(N/32)+p);
        assert(parts.logical_index(p,e) < valid);
        ++total;
      }
    }
    assert(total == valid);
  }
}
template<class T> void all() {
  check<T,64>(); check<T,128>(); check<T,256>();
  check<T,512>(); check<T,1024>();
}
int main() { all<int64_t>(); all<uint64_t>(); all<int32_t>(); all<uint32_t>(); }
''', run=True)

    def test_direct_32_writer_parent_rejected(self):
        self.compile(r'''
pto::TileArray<pto::ElementTile<uint64_t,32>,1,32> rejected;
''', accepts=False)

    def test_parent_cannot_be_used_as_leaf_arithmetic(self):
        self.compile(r'''
int main() {
  pto::ElementTile<uint64_t,1024> parent;
  auto &bad = pto::TPARTELEMENT(parent);
  (void)bad;
}
''', accepts=False)

    def test_exact_view_annotations(self):
        header = (ROOT / "include/common/pto_tile_region.hpp").read_text()
        for marker in (
            "pto.element.view:v1;dtype=s64;rows=32;cols=1;layout=cube_m32",
            "pto.element.view:v1;dtype=u64;rows=32;cols=1;layout=cube_m32",
        ):
            self.assertIn(f'annotate("{marker}")', header)

    def test_fp64_element_profile_rejected(self):
        self.compile("pto::ElementTile<double,32> value;", accepts=False)

    def test_unsupported_i64_parent_rejected(self):
        self.compile("pto::ElementTile<uint64_t,2048> value;", accepts=False)

    def test_m16_i64_rejected(self):
        self.compile("pto::CubeTileM16<uint64_t,16,1> value;", accepts=False)

    def test_short_physical_m32_i64_rejected(self):
        self.compile("pto::VecTileM32<uint64_t,16,1> value;", accepts=False)

    def test_incomplete_pair_subview_rejected(self):
        self.compile(r'''
int main(){
  pto::ElementTile<uint64_t,32> parent;
  auto bad=pto::range::subview<1>(parent);
  (void)bad;
}
''', accepts=False)

    def test_incomplete_pair_assemble_rejected(self):
        self.compile(r'''
int main(){
  pto::ElementTile<int64_t,32> parent;
  auto bad=pto::range::assemble_init_last<1,1>(parent);
  (void)bad;
}
''', accepts=False)

    def test_spec_pin_is_exact(self):
        self.assertEqual(SPEC_PIN,
                         "8f589b70085a37c70f210898b168aae634da5ac1")


if __name__ == "__main__":
    unittest.main()
