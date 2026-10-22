#!/usr/bin/env python3
"""Public U32 full-row NORM element-view source contract."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
ANNOTATIONS = (
    "pto.element.view:v2;dtype=u32;layout=norm;physical_rows=1;"
    "physical_cols=128;valid_rows=1;valid_cols=128",
    "pto.element.view:v2;dtype=u32;layout=norm;physical_rows=1;"
    "physical_cols=128;valid_rows=1;valid_cols=32",
    "pto.element.view:v2;dtype=u32;layout=norm;physical_rows=1;"
    "physical_cols=2048;valid_rows=1;valid_cols=2048",
)


class NormElementViewAPITest(unittest.TestCase):
    def compile(self, code):
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(prefix="norm-element-view-") as directory:
            source = Path(directory) / "contract.cpp"
            source.write_text(
                "#include <common/pto_tile.hpp>\n"
                "#include <common/pto_tile_region.hpp>\n" + code)
            result = subprocess.run(
                [compiler, "-std=c++20", "-D__linx", "-include",
                 str(ROOT / "test/linx_host_type_shim.hpp"),
                 "-I", str(ROOT / "include"), str(source), "-fsyntax-only"],
                text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_exact_profiles_only(self):
        self.compile(r'''
#include <common/pto_tileop_api_revision.hpp>
using namespace pto;
#ifndef PTO_TILEOP_API_HAS_NORM_U32_ELEMENT_VIEW_V2
#error "NORM U32 element-view v2 feature gate is required"
#endif
template<class T> concept HasElements = requires(T& tile) { TPARTELEMENT(tile); };
using Full128 = Tile<Location::Vec,uint32_t,1,128,BLayout::RowMajor,1,128>;
using Tail32 = Tile<Location::Vec,uint32_t,1,128,BLayout::RowMajor,1,32>;
using Full2048 = Tile<Location::Vec,uint32_t,1,2048,BLayout::RowMajor,1,2048>;
static_assert(HasElements<Full128>);
static_assert(HasElements<Tail32>);
static_assert(HasElements<Full2048>);
static_assert(!is_element_tile_v<Full128>);
static_assert(!is_element_tile_v<Full2048>);
static_assert(sizeof(typename Full128::TileDType) == 128 * sizeof(uint32_t));
static_assert(sizeof(typename Full2048::TileDType) == 2048 * sizeof(uint32_t));

using Wrong64 = Tile<Location::Vec,uint32_t,1,64,BLayout::RowMajor,1,64>;
using WrongTail31 = Tile<Location::Vec,uint32_t,1,128,BLayout::RowMajor,1,31>;
using Wrong2048Tail = Tile<Location::Vec,uint32_t,1,2048,BLayout::RowMajor,1,32>;
using WrongRows = Tile<Location::Vec,uint32_t,2,64,BLayout::RowMajor,2,64>;
using WrongType = Tile<Location::Vec,int32_t,1,128,BLayout::RowMajor,1,128>;
using WrongLayout = Tile<Location::Vec,uint32_t,1,128,BLayout::ColMajor,1,128>;
static_assert(!HasElements<Wrong64>);
static_assert(!HasElements<WrongTail31>);
static_assert(!HasElements<Wrong2048Tail>);
static_assert(!HasElements<WrongRows>);
static_assert(!HasElements<WrongType>);
static_assert(!HasElements<WrongLayout>);
''')

    def test_annotation_schema_is_exact(self):
        source = (ROOT / "include/common/pto_tile_region.hpp").read_text()
        for annotation in ANNOTATIONS:
            self.assertEqual(source.count(annotation), 1)
        self.assertNotIn("layout=norm;rows=", source)


if __name__ == "__main__":
    unittest.main()
