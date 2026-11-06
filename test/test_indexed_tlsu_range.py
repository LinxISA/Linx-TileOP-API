"""Compile #424 public APIs and inspect real target records (no ISA simulation)."""
import re
import subprocess
import unittest
import test_shared_nd_to_cube as native
FIXTURES = native.FIXTURES


class IndexedTLSURange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        native.NativeLowering.setUpClass.__func__(cls)

    @classmethod
    def tearDownClass(cls):
        cls.work.cleanup()

    compile = native.NativeLowering.compile

    def disassemble(self, path):
        from pathlib import Path
        return subprocess.check_output([
            str(Path(self.compiler).parent / 'llvm-objdump'), '-d',
            '--no-show-raw-insn', '--disassembler-options=no-tile-macros',
            str(path)], text=True)

    def test_all_23_wrappers_and_zero_presence(self):
        source = str(FIXTURES / 'IndexedTLSURange.cpp')
        # Out-of-line Tile wrappers already fail frame lowering at -O0.
        # Check unoptimized IR separately; inspect native records at -O2.
        obj = self.directory / 'range.o'
        self.compile('-O2', '-c', source, '-o', str(obj))
        text = self.disassemble(obj)
        operations = set(re.findall(r'BSTART.TLSU\s+(M(?:GATHER|SCATTER)(?:\.[A-Z]+)?),', text))
        self.assertEqual(len(operations), 23, operations)
        self.assertNotIn('ExecMaskPresent', text)
        # Every actual indexed request has an explicit bound selector.
        bundles = re.findall(r'BSTART.TLSU\s+M(?:GATHER|SCATTER)[, .].*?(?=BSTART|$)', text, re.S)
        ranges = [operands for bundle in bundles for operands in
                  re.findall(r'B.IOR\s+\[([^]]+)\],\s*\[\]', bundle)]
        self.assertGreaterEqual(len(ranges), 23)
        for operands in ranges:
            slots = [slot.strip() for slot in operands.split(',')]
            self.assertGreaterEqual(len(slots), 2, operands)
            self.assertNotEqual(slots[1:], ['zero', 'zero'], operands)

    def test_unoptimized_ir_keeps_full_width_bounds(self):
        output = self.directory / 'range-o0.ll'
        self.compile('-O0', '-S', '-emit-llvm',
                     str(FIXTURES / 'IndexedTLSURange.cpp'), '-o', str(output))
        ir = output.read_text()
        bounded = [line for line in ir.splitlines()
                   if 'asm sideeffect' in line and 'BSTART.TLSU M' in line
                   and re.search(r'(?:^|,)R,r,', line.split('", "')[-1])]
        operations = {re.search(r'BSTART.TLSU ([^,]+),', line)[1]
                      for line in bounded}
        self.assertEqual(len(operations), 23, operations)
        for line in bounded:
            self.assertIn('i64 ', line)

    def test_predicate_range_uses_one_unflagged_record(self):
        source = self.directory / 'predicate-range.cpp'
        source.write_text('''#include <common/pto_tileop.hpp>
using namespace pto;
using V = VecTileM32<uint32_t, 32, 4>;
using I = VecTileM32<uint64_t, 32, 4>;
void bounded(V &v, I &ix, global_tensor<uint32_t, RowMajor<32, 4>> &gm,
             int64_t min, int64_t max) {
  auto active = predicate::compare_scalar<CmpMode::GE>(v, uint32_t(1));
  predicate::scatter(gm, ix, v, active, IndexedByteRange{min, max});
  predicate::atomic_add(gm, ix, v, active, IndexedByteRange{min, max});
}
''')
        obj = self.directory / 'predicate-range.o'
        self.compile('-c', str(source), '-o', str(obj))
        text = self.disassemble(obj)
        # Check default macro folding too: physical disassembly alone missed
        # the absent PredicateTile catalog form during the initial review.
        assembly = self.directory / 'predicate-bundles.s'
        # Remove addresses from the physical listing before replaying it.
        commands = []
        active = False
        for line in text.splitlines():
            match = re.match(r'^\s*[0-9a-f]+:\s+(.*)$', line)
            if not match:
                continue
            command = match[1]
            if 'BSTART' in command:
                active = bool(re.search(r'BSTART.TLSU\s+M(?:GATHER|SCATTER)', command))
            if active:
                commands.append(command)
        assembly.write_text('\n'.join(commands) + '\n')
        from pathlib import Path
        indexed_obj = self.directory / 'predicate-bundles.o'
        subprocess.run([str(Path(self.compiler).parent / 'llvm-mc'),
                        '-triple=linx64v5', '-filetype=obj', str(assembly),
                        '-o', str(indexed_obj)], check=True, capture_output=True)
        folded = subprocess.check_output([
            str(Path(self.compiler).parent / 'llvm-objdump'), '-d',
            '--no-show-raw-insn', str(indexed_obj)], text=True)
        self.assertIn('MSCATTER_ADD', folded)
        self.assertNotIn('ExecMaskPresent', folded)
        for operation in ('MSCATTER.MASK', 'MSCATTER.ADD'):
            match = re.search(r'BSTART.TLSU\s+' + re.escape(operation) + r',.*?(?=BSTART|$)', text, re.S)
            self.assertIsNotNone(match, text)
            self.assertEqual(match.group().count('B.IOR'), 1)
            self.assertNotIn('ExecMaskPresent', match.group())


if __name__ == '__main__':
    unittest.main()
