#!/usr/bin/env python3
"""Exercise the scalar MSGB protocol with a scripted register bank."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MessageBufferTransportTest(unittest.TestCase):
    def run_host(self, compiler, language, standard, source="msgb_transport_test.c"):
        with tempfile.TemporaryDirectory(prefix="msgb-transport-") as work:
            binary = Path(work) / "msgb"
            subprocess.run([
                compiler, "-x", language, f"-std={standard}", "-Wall", "-Wextra",
                "-Werror", "-I", str(ROOT / "include"),
                str(ROOT / "test" / source), "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)

    def test_c11(self):
        self.run_host(os.environ.get("CC") or "cc", "c", "c11")

    def test_cpp17(self):
        self.run_host(os.environ.get("CXX") or "c++", "c++", "c++17")

    def test_typed_messages(self):
        self.run_host(os.environ.get("CXX") or "c++", "c++", "c++17", "pto_message_test.cpp")

    def test_rejected_message_types(self):
        compiler = os.environ.get("CXX") or "c++"
        for invalid in ("BAD_NONTRIVIAL", "BAD_OVERSIZE", "BAD_SCALAR",
                        "BAD_CONST_RECV", "BAD_VOLATILE_RECV"):
            modes = ("TEST_RECV",) if invalid.endswith("_RECV") else ("TEST_SEND", "TEST_RECV")
            for mode in modes:
                with self.subTest(invalid=invalid, mode=mode):
                    result = subprocess.run([
                        compiler, "-std=c++17", "-fsyntax-only", "-I", str(ROOT / "include"),
                        "-D" + invalid, "-D" + mode, str(ROOT / "test/pto_message_test.cpp"),
                    ], capture_output=True, text=True)
                    self.assertNotEqual(result.returncode, 0, "invalid message type compiled")


if __name__ == "__main__":
    unittest.main()
