#!/usr/bin/env python3
"""Compile proposal declarations and execute the portable MSGB transport model.

No Linx compiler, real SSR accesses, or hardware behavior is exercised here.
"""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
PROPOSAL = ROOT / "docs/proposals/sync-intrinsics"


def compiler(variable, candidates):
    configured = os.environ.get(variable)
    if configured:
        return shlex.split(configured)
    for name in candidates:
        path = shutil.which(name)
        if path:
            return [path]
    raise RuntimeError(f"No compiler found for {variable}; set it explicitly")


class SyncIntrinsicsProposalTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = compiler("CC", ("clang", "cc", "gcc"))
        cls.cxx = compiler("CXX", ("clang++", "c++", "g++"))

    def compile_and_run(self, command, *, standard, language, source, run=False,
                        service=False):
        with tempfile.TemporaryDirectory(prefix="sync-proposal-") as tmp:
            path = Path(tmp)
            input_file = path / ("check.c" if language == "c" else "check.cpp")
            input_file.write_text(source, encoding="utf-8")
            output = path / "reference-test"
            args = command + [
                f"-std={standard}", "-Wall", "-Wextra", "-Werror", "-pedantic",
                "-I", str(PROPOSAL), "-x", language,
            ]
            if service:
                args += ["-DWSE_SYNC_SERVICE_API"]
            if not run:
                args += ["-fsyntax-only"]
            args += [str(input_file)]
            if run:
                args += ["-o", str(output)]
            result = subprocess.run(args, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            if run:
                result = subprocess.run([str(output)], text=True,
                                        capture_output=True)
                self.assertEqual(result.returncode, 0,
                                 result.stdout + result.stderr)

    def test_declarations_c_and_cpp(self):
        for command, standard, language in (
            (self.cc, "c11", "c"), (self.cxx, "c++17", "c++")
        ):
            for service in (False, True):
                with self.subTest(standard=standard, service=service):
                    self.compile_and_run(
                        command, standard=standard, language=language,
                        source='#include "sync_intrinsics.h"\n', service=service)

    def test_transport_reference_c_and_cpp(self):
        source = (ROOT / "test/sync_msgb_reference_test.c").read_text(
            encoding="utf-8")
        for command, standard, language in (
            (self.cc, "c11", "c"), (self.cxx, "c++17", "c++")
        ):
            with self.subTest(standard=standard):
                self.compile_and_run(command, standard=standard,
                                     language=language, source=source, run=True)

    def test_ack_register_state_machine_c_and_cpp(self):
        source = (ROOT / "test/sync_msgb_ack_test.c").read_text(encoding="utf-8")
        for command, standard, language in (
            (self.cc, "c11", "c"), (self.cxx, "c++17", "c++")
        ):
            with self.subTest(standard=standard):
                self.compile_and_run(command, standard=standard,
                                     language=language, source=source, run=True)


if __name__ == "__main__":
    unittest.main()
