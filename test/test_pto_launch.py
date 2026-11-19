#!/usr/bin/env python3
"""Compile and inspect the device-side PTO launch context accessors."""

import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PROBE = r"""
#include <common/pto_launch.hpp>

extern "C" uint32_t read_block_num() { return pto::GetBlockNum(); }
extern "C" uint32_t read_block_idx() { return pto::GetBlockIdx(); }
"""


class PTOLaunchTest(unittest.TestCase):
    def test_public_header_owns_the_architectural_registers(self):
        header = (ROOT / "include/common/pto_launch.hpp").read_text()
        self.assertIn("namespace pto", header)
        self.assertIn("GetBlockNum", header)
        self.assertIn("GetBlockIdx", header)
        self.assertIn("SSRGET 0x0050", header)
        self.assertIn("SSRGET 0x0051", header)

    def test_target_object_reads_blocknum_and_blockid(self):
        compiler = os.environ.get("LINX_CXX")
        if not compiler:
            self.skipTest("set LINX_CXX to a Linx target clang++")

        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            source = work / "probe.cpp"
            obj = work / "probe.o"
            source.write_text(PROBE)
            subprocess.run([
                compiler,
                "--target=linx64v5-unknown-linux-musl",
                "-std=c++17",
                "-ffreestanding",
                "-O2",
                "-I",
                str(ROOT / "include"),
                "-c",
                str(source),
                "-o",
                str(obj),
            ], check=True)

            objdump = Path(compiler).parent / "llvm-objdump"
            listing = subprocess.check_output([str(objdump), "-d", str(obj)], text=True)

        self.check_accessor(listing, "read_block_num", 0x50)
        self.check_accessor(listing, "read_block_idx", 0x51)

    def check_accessor(self, listing, symbol, expected_address):
        body = re.search(
            rf"^[0-9a-f]+ <{symbol}>:\n(?P<body>.*?)(?=^[0-9a-f]+ <|\Z)",
            listing,
            re.MULTILINE | re.DOTALL,
        )
        self.assertIsNotNone(body, f"missing {symbol} in target disassembly")
        words = [int(word, 16) for word in re.findall(
            r"^\s*[0-9a-f]+:\s+([0-9a-f]+)\s+", body.group("body"), re.MULTILINE)]
        reads = [(index, word >> 20) for index, word in enumerate(words)
                 if word & 0x000ff07f == 0x0000003b]
        self.assertEqual([address for _, address in reads], [expected_address])
        index, _ = reads[0]
        self.assertGreater(index, 0)
        self.assertLess(index + 1, len(words))
        self.assertEqual(words[index - 1], 0x0840)
        self.assertEqual(words[index + 1], 0x0800)


if __name__ == "__main__":
    unittest.main()
