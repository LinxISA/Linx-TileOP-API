#!/usr/bin/env python3
"""Exercise the scalar MSGB protocol with a scripted register bank."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MessageBufferTransportTest(unittest.TestCase):
    def run_host(self, compiler, language, standard):
        with tempfile.TemporaryDirectory(prefix="msgb-transport-") as work:
            binary = Path(work) / "msgb"
            subprocess.run([
                compiler, "-x", language, f"-std={standard}", "-Wall", "-Wextra",
                "-Werror", "-I", str(ROOT / "include"),
                str(ROOT / "test/msgb_transport_test.c"), "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)

    def test_c11(self):
        self.run_host(os.environ.get("CC") or "cc", "c", "c11")

    def test_cpp17(self):
        self.run_host(os.environ.get("CXX") or "c++", "c++", "c++17")


if __name__ == "__main__":
    unittest.main()
