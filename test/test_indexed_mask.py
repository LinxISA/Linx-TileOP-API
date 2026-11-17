"""#320 carrier diagnostics, real target lowering, optional gfrun execution.

Host checks strip target asm and only validate the C++ contract. Set LINX_CXX
and LINX_SYSROOT for target checks; also set LINX_MODEL_BIN for execution.
"""
import importlib.util
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import unittest

import test_shared_nd_to_cube as native

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / "test/tileop_api/src"
NEGATIVES = {
    "DTYPE": "requires canonical U8 byte lanes",
    "SIGNED": "requires canonical U8 byte lanes",
    "LAYOUT": "mask, index and data layouts must match",
    "UNSUPPORTED_LAYOUT": "requires RowMajor, CUBE_M16 or CUBE_M32",
    "ROWS": "mask and index valid rows must match",
    "COLUMNS": "mask and index valid columns must match",
    "CAPACITY": "mask capacity must be 128 B..256 KiB",
    "ROW_CAPACITY": "mask row capacity must cover the index extent",
    "COLUMN_CAPACITY": "mask column capacity must cover the index extent",
    "SHARED": "mask must use a Local Tile",
}


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


class ContractChecks:
    def test_all_overloads_accept_independent_mask_capacity(self):
        for source in ("IndexedMask.cpp", "IndexedMaskNegatives.cpp",
                       "IndexedTLSURange.cpp"):
            result = subprocess.run(self.flags + ["-fsyntax-only", str(FIXTURES / source)],
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_all_overloads_reject_incompatible_masks(self):
        for operation in range(4):
            for case, diagnostic in NEGATIVES.items():
                with self.subTest(operation=operation, case=case):
                    result = subprocess.run(self.flags + [
                        "-fsyntax-only", "-DMASK_" + case,
                        "-DMASK_OPERATION=" + str(operation),
                        str(FIXTURES / "IndexedMaskNegatives.cpp"),
                    ], text=True, capture_output=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(diagnostic, result.stderr)


class HostContract(ContractChecks, unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.TemporaryDirectory(prefix="indexed-mask-host-")
        directory = Path(cls.work.name)
        examples = load_module("indexed_mask_examples", ROOT / "tools/check_tileop_usage_examples.py")
        examples.host_include_tree(directory / "include")
        # Older aarch64 host compilers lack _Float16. These checks only use
        # integer tiles, so a distinct two-byte placeholder suffices for half.
        shim = directory / "types.hpp"
        shim.write_text((ROOT / "test/linx_host_type_shim.hpp").read_text().replace(
            "using __half = _Float16;", "struct __half { unsigned short value; };"))
        cls.flags = [os.environ.get("CXX", "c++"), "-std=c++20", "-D__linx",
                     "-include", str(shim),
                     "-I", str(directory / "include")]

    @classmethod
    def tearDownClass(cls):
        cls.work.cleanup()

    def test_cube_contract_keeps_independent_geometry(self):
        # Contract-only checks: CUBE layout emission is tracked in #201.
        source = Path(self.work.name) / "cube.cpp"
        source.write_text('''#include <common/pto_indexed_mask.hpp>
using namespace pto;
template <BLayout L, int R> void check() {
  using D = Tile<Location::Vec, uint32_t, R, 4, L>;
  using I = Tile<Location::Vec, int32_t, R, 4, L>;
  using M = Tile<Location::Vec, uint8_t, R, 16, L, R, 4>;
  detail::validate_indexed_mask<D, I, M>();
}
template void check<BLayout::CubeM16, 16>();
template void check<BLayout::CubeM32, 32>();
''')
        result = subprocess.run(self.flags + ["-fsyntax-only", str(source)],
                                text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)


class NativeContract(ContractChecks, unittest.TestCase):
    setUpClass = classmethod(native.NativeLowering.setUpClass.__func__)
    tearDownClass = classmethod(native.NativeLowering.tearDownClass.__func__)
    compile = native.NativeLowering.compile

    def test_documentation_examples_compile(self):
        examples = load_module("indexed_mask_target_examples", ROOT / "tools/check_tileop_usage_examples.py")
        for example in examples.examples():
            if example.document.name in {"MGATHER_MASK.md", "MSCATTER_MASK.md"}:
                with self.subTest(document=example.document.name):
                    source = self.directory / (example.document.stem + ".cpp")
                    source.write_text(examples.compilation_unit(example))
                    self.compile("-c", str(source), "-o", str(source.with_suffix(".o")))

    def test_target_object_preserves_mask_operands(self):
        obj = self.directory / "indexed-mask.o"
        self.compile("-c", str(FIXTURES / "IndexedMask.cpp"), "-o", str(obj))
        listing = subprocess.check_output([
            str(Path(self.compiler).parent / "llvm-objdump"), "-d",
            "--no-show-raw-insn", "--disassembler-options=no-tile-macros", str(obj),
        ], text=True)
        self.assertEqual(len(re.findall(r"BSTART.TLSU\s+MGATHER.MASK,", listing)), 4)
        self.assertEqual(len(re.findall(r"BSTART.TLSU\s+MSCATTER.MASK,", listing)), 4)

    def check_model_case(self, operation, noncanonical=False):
        model = os.environ.get("LINX_MODEL_BIN")
        if not model:
            self.skipTest("set LINX_MODEL_BIN for real model execution")
        # Stop immediately after main returns. A successful process exit alone
        # is insufficient; verify the stop state and every output byte.
        startup = self.directory / "start.s"
        startup.write_text('''.global _start
.global indexed_mask_stop
.type _start,@function
.text
_start:
 bstart.std call main
 c.setret 2, ->ra
indexed_mask_stop:
 c.bstart.std
 bstop
''')
        elf = self.directory / f"mask-{operation}.elf"
        self.compile(*(["-DMASK_NONCANONICAL"] if noncanonical else []),
                     "-DMASK_OPERATION=" + str(operation),
                     "-mllvm", "-enable-all-vector-as-tilereg=true",
                     "-nostdlib", "-ffreestanding", "-fno-builtin", "-static",
                     str(startup), str(FIXTURES / "IndexedMaskRuntime.cpp"),
                     "-o", str(elf))
        symbols = subprocess.check_output([
            str(Path(self.compiler).parent / "llvm-nm"), str(elf)], text=True)
        addresses = {name: int(address, 16) for address, name in re.findall(
            r"^([0-9a-fA-F]+)\s+\w\s+(\w+)$", symbols, re.M)}
        output = self.directory / f"mask-{operation}.bin"
        state = self.directory / f"mask-{operation}.json"
        ran = subprocess.run([
            model, "-f", str(elf), "-s", "softcore.multiThreadNum=1",
            "--stop_pc", str(addresses["indexed_mask_stop"]),
            "--dump-run-state", str(state), "--dump-memory",
            f"0x{addresses['observed']:x}:512:{output}",
        ], cwd=str(Path(model).resolve().parent.parent),
            text=True, capture_output=True, timeout=60)
        if noncanonical:
            self.assertNotEqual(ran.returncode, 0, ran.stdout + ran.stderr)
            self.assertIn("mask predicate 0x2", ran.stdout + ran.stderr)
            self.assertIn("is not 0x00 or 0x01", ran.stdout + ran.stderr)
            return
        self.assertEqual(ran.returncode, 0, ran.stdout + ran.stderr)
        self.assertTrue(json.loads(state.read_text())["stop_pc_requested"],
                        ran.stdout + ran.stderr)
        expected = []
        for pattern in range(4):
            for lane in range(32):
                active = (False, True, lane % 2 == 1, lane >= 16)[pattern]
                expected.append(100 + lane if active else (0 if operation < 2 else 93))
        self.assertEqual(output.read_bytes(), struct.pack("<128I", *expected))

    def test_model_byte_masks(self):
        for operation in range(4):
            with self.subTest(operation=operation):
                self.check_model_case(operation)

    def test_model_rejects_noncanonical_bytes(self):
        for operation in range(4):
            with self.subTest(operation=operation):
                self.check_model_case(operation, noncanonical=True)


class DocumentationContract(unittest.TestCase):
    def test_regeneration_keeps_contract_and_examples(self):
        generator = load_module("indexed_mask_docs", ROOT / "tools/generate_tileop_usage_docs.py")
        # Minimal standards input keeps this test independent of a sibling spec checkout.
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            generator.SPEC_ROOT = directory
            generator.SPEC_DOC_ROOT = directory
            generator.ASL_ROOT = directory
            seed = directory / "mask.md"
            seed.write_text("# mask\n\n## Summary\n\nIndexed memory operation.\n")
            header = (ROOT / "include/jcore/template_asm.hpp").read_text()
            for name in ("MGATHER_MASK", "MSCATTER_MASK"):
                original = (ROOT / f"docs/tileop-usage/tlsu/memory/{name}.md").read_text()
                regenerated = generator.render(name, seed, header, original)
                for phrase in ("0x00", "0x01", "TCMP/TCMPS", "128 B..256 KiB",
                               "动态有效区域", "TLOAD(mask, enabled_gm)"):
                    self.assertIn(phrase, regenerated)
                self.assertIn("不分配输出 Tile" if name == "MSCATTER_MASK" else
                              "TmaPadValue::Zero", regenerated)
            for name in ("TCMP", "TCMPS"):
                self.assertIn("每个比较结果占一位", generator.operation_contract(name))


if __name__ == "__main__":
    unittest.main()
