#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SOURCE_HEADER = ROOT / "include/common/pto_tileop_api_revision.hpp"
UMBRELLA_HEADER = ROOT / "include/common/pto_tileop.hpp"


class TileOpApiVersionHeaderTest(unittest.TestCase):
    def test_source_header_exposes_version_and_feature(self):
        header = SOURCE_HEADER.read_text(encoding="utf-8")
        self.assertIn('#define PTO_TILEOP_API_VERSION "0.58.3"', header)
        self.assertIn('#define PTO_TILEOP_API_SPEC_VERSION "0.58.6"', header)
        self.assertIn('#define PTO_TILEOP_API_REVISION "source-tree"', header)
        self.assertIn('#define PTO_TILEOP_API_REVISION_IS_EXACT 0', header)
        self.assertIn('#define PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX 1', header)
        self.assertIn(
            '#include "common/pto_tileop_api_revision.hpp"',
            UMBRELLA_HEADER.read_text(encoding="utf-8"),
        )

    def test_install_records_exact_checkout_revision(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            prefix = temp / "toolchain"
            resource_dir = prefix / "lib/clang/15.0.4"
            clang = prefix / "bin/clang"
            clang.parent.mkdir(parents=True)
            resource_dir.mkdir(parents=True)
            clang.write_text(
                "#!/bin/sh\nprintf '%s\\n' '" + str(resource_dir) + "'\n",
                encoding="utf-8",
            )
            clang.chmod(0o755)

            revision = "0123456789abcdef0123456789abcdef01234567"
            subprocess.run(
                [
                    "make",
                    "install",
                    f"CLANG_PREFIX={prefix}",
                    f"TILEOP_API_REVISION={revision}",
                    "TILEOP_API_REVISION_IS_EXACT=1",
                ],
                cwd=ROOT,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )

            installed = (
                resource_dir
                / "include/tileop-api/common/pto_tileop_api_revision.hpp"
            ).read_text(encoding="utf-8")
            self.assertIn(f'#define PTO_TILEOP_API_REVISION "{revision}"', installed)
            self.assertIn('#define PTO_TILEOP_API_REVISION_IS_EXACT 1', installed)
            self.assertIn('#define PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX 1', installed)


if __name__ == "__main__":
    unittest.main()
