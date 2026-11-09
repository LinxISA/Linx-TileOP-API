#!/usr/bin/env python3
"""A public header must link across translation units without symbol rewriting."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LayoutHeaderLinkTest(unittest.TestCase):
    def test_inline_layout_name_has_one_address_across_translation_units(self):
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory)
            for name in ("left", "right"):
                (out / f"{name}.cpp").write_text(
                    '#include "common/layout.hpp"\n'
                    'using Function = const char *(*)(pto::LayoutEnum);\n'
                    f'Function {name}() {{ return pto::layout_type_to_str; }}\n')
            (out / "main.cpp").write_text(
                '#include "common/layout.hpp"\n#include <cstring>\n'
                'using Function = const char *(*)(pto::LayoutEnum);\n'
                'Function left(); Function right();\n'
                'int main() {\n'
                '  if (left() != right()) return 1;\n'
                '  if (std::strcmp(left()(pto::LayoutEnum::kRowMajor), "RowMajor")) return 2;\n'
                '  if (std::strcmp(right()(pto::LayoutEnum::kColMajor), "ColMajor")) return 3;\n'
                '  return 0;\n}\n')
            command = shlex.split(os.environ.get("CXX", "c++"))
            command += ["-std=c++20", "-O0", "-I", str(ROOT / "include")]
            command += [str(out / f"{name}.cpp") for name in ("left", "right", "main")]
            command += ["-o", str(out / "layout-link")]
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            subprocess.run([str(out / "layout-link")], check=True)


if __name__ == "__main__":
    unittest.main()
