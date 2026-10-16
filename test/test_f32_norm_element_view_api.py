#!/usr/bin/env python3
"""Public FP32 rectangular NORM element-view source contract."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
ANNOTATIONS = (
    "pto.element.view:v2;dtype=f32;layout=norm;physical_rows=16;"
    "physical_cols=32;valid_rows=16;valid_cols=32",
    "pto.element.view:v2;dtype=f32;layout=norm;physical_rows=1;"
    "physical_cols=32;valid_rows=1;valid_cols=32",
)


class F32NormElementViewAPITest(unittest.TestCase):
    def compile(self, code):
        compiler = os.environ.get("CXX") or shutil.which("c++")
        self.assertIsNotNone(compiler)
        with tempfile.TemporaryDirectory(
                prefix="f32-norm-element-view-") as directory:
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
using namespace pto;
template<class T> concept HasElements = requires(T& tile) { TPARTELEMENT(tile); };
template<class T> concept HasConstElements = requires(const T& tile) {
  TPARTELEMENT(tile);
};
using Matrix = Tile<Location::Vec,float,16,32,BLayout::RowMajor,16,32>;
using Row = Tile<Location::Vec,float,1,32,BLayout::RowMajor,1,32>;
static_assert(HasElements<Matrix>);
static_assert(HasElements<Row>);
static_assert(HasConstElements<Matrix>);
static_assert(HasConstElements<Row>);
static_assert(std::is_same_v<std::remove_cvref_t<
              decltype(TPARTELEMENT(std::declval<Matrix&>())[0])>, float>);
static_assert(std::is_same_v<std::remove_cvref_t<
              decltype(TPARTELEMENT(std::declval<Row&>())[0])>, float>);
static_assert(std::is_const_v<std::remove_reference_t<
              decltype(TPARTELEMENT(std::declval<const Matrix&>()))>>);
static_assert(std::is_const_v<std::remove_reference_t<
              decltype(TPARTELEMENT(std::declval<const Row&>()))>>);
static_assert(std::is_const_v<std::remove_reference_t<
              decltype(TPARTELEMENT(std::declval<const Matrix&>())[0])>>);
static_assert(std::is_const_v<std::remove_reference_t<
              decltype(TPARTELEMENT(std::declval<const Row&>())[0])>>);
static_assert(sizeof(typename Matrix::TileDType) == 16 * 32 * sizeof(float));
static_assert(sizeof(typename Row::TileDType) == 32 * sizeof(float));

using WrongShape = Tile<Location::Vec,float,8,64,BLayout::RowMajor,8,64>;
using WrongValidRows = Tile<Location::Vec,float,16,32,BLayout::RowMajor,15,32>;
using WrongValidCols = Tile<Location::Vec,float,16,32,BLayout::RowMajor,16,31>;
using WrongType = Tile<Location::Vec,uint32_t,16,32,BLayout::RowMajor,16,32>;
using WrongLayout = Tile<Location::Vec,float,16,32,BLayout::ColMajor,16,32>;
using WrongInnerRow = Tile<Location::Vec,float,16,32,BLayout::RowMajor,16,32,
                           SLayout::RowMajor>;
using WrongInnerCol = Tile<Location::Vec,float,16,32,BLayout::RowMajor,16,32,
                           SLayout::ColMajor>;
static_assert(!HasElements<WrongShape>);
static_assert(!HasElements<WrongValidRows>);
static_assert(!HasElements<WrongValidCols>);
static_assert(!HasElements<WrongType>);
static_assert(!HasElements<WrongLayout>);
static_assert(!HasElements<WrongInnerRow>);
static_assert(!HasElements<WrongInnerCol>);
static_assert(!HasConstElements<WrongInnerRow>);
static_assert(!HasConstElements<WrongInnerCol>);
''')

    def test_annotation_schema_is_exact(self):
        source = (ROOT / "include/common/pto_tile_region.hpp").read_text()
        for annotation in ANNOTATIONS:
            self.assertEqual(source.count(annotation), 2)


if __name__ == "__main__":
    unittest.main()
