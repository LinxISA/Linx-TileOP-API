#!/usr/bin/env python3
"""Compile proposal declarations and execute the portable MSGB transport model.

Host checks never execute real SSRs. Set LINX_CC and LINX_SYSROOT to also
compile the backend/runtime for Linx; this still does not execute hardware.
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
                        service=False, extra_sources=()):
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
            args += [str(input_file)] + list(map(str, extra_sources))
            if run:
                args += ["-o", str(output)]
            result = subprocess.run(args, text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            if run:
                result = subprocess.run([str(output)], text=True,
                                        capture_output=True)
                self.assertEqual(result.returncode, 0,
                                 result.stdout + result.stderr)

    @unittest.skipUnless(os.environ.get("LINX_CC"), "set LINX_CC for target checks")
    def test_target_backend_and_runtime(self):
        command = shlex.split(os.environ["LINX_CC"])
        sysroot = os.environ.get("LINX_SYSROOT")
        self.assertTrue(sysroot, "LINX_SYSROOT is required with LINX_CC")
        source = """#include "backend/msgb_linx_backend.h"
#if !PTO_MSGB_COMPILER_AVAILABLE
#error MSGB builtins required
#endif
int send_msg(pto_msgb_endpoint *ep, unsigned dst, const uint64_t *w, unsigned n) {
  return pto_msgb_send(ep, dst, w, n, 0);
}
void setup(pto_msgb_endpoint *ep) {
  pto_msgb_endpoint_init(ep, pto_msgb_linx_io());
}
"""
        with tempfile.TemporaryDirectory(prefix="msgb-target-") as tmp:
            path = Path(tmp)
            probe = path / "probe.c"
            probe.write_text(source)
            for optimization in ("-O2", "-O3"):
                for input_file in (probe, PROPOSAL / "runtime/sync_runtime.c"):
                    result = subprocess.run(command + [
                        "--target=linx64v5-unknown-linux-musl", "-mcpu=janus",
                        "--sysroot=" + sysroot, optimization, "-std=c11",
                        "-I", str(PROPOSAL), "-c", str(input_file),
                        "-o", str(path / "target.o")], text=True,
                        capture_output=True)
                    self.assertEqual(result.returncode, 0,
                                     result.stdout + result.stderr)

    def test_register_projection(self):
        result = subprocess.run([os.sys.executable,
            str(ROOT / "tools/check_msgb_contract.py")],
            text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_runtime_c_and_cpp(self):
        source = (ROOT / "test/sync_msgb_runtime_test.c").read_text()
        for command, standard, language in (
            (self.cc, "c11", "c"), (self.cxx, "c++17", "c++")
        ):
            with self.subTest(standard=standard):
                self.compile_and_run(command, standard=standard,
                    language=language, source=source, run=True,
                    extra_sources=(PROPOSAL / "runtime/sync_runtime.c",))

    def test_endpoint_c_and_cpp(self):
        source = (ROOT / "test/sync_msgb_endpoint_test.c").read_text()
        for command, standard, language in (
            (self.cc, "c11", "c"), (self.cxx, "c++17", "c++")
        ):
            with self.subTest(standard=standard):
                self.compile_and_run(command, standard=standard,
                                     language=language, source=source, run=True)

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
