#!/usr/bin/env python3
"""Local M32 writer contracts, lifecycle guards and real TLSU lowering."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = ROOT / 'test/tileop_api/src'
NEGATIVES = {
    'CAPACITY': 'coverage exceeds parent capacity',
    'PARENT_LIMIT': 'parent capacity of 128B..64KiB',
    'DTYPE': 'writer dtype, location and layout must match',
    'LAYOUT': 'writer dtype, location and layout must match',
    'ROWS': '32 physical rows',
    'VALID_ROWS': 'common valid rows',
    'ENVELOPE': 'exact power-of-two CELL envelope',
    'OVERSIZED_ENVELOPE': 'exact power-of-two CELL envelope',
    'TAIL': 'common valid rows|exact power-of-two CELL envelope',
    'DYNAMIC': 'static valid writer shapes',
    'WRITER_LIMIT': 'requires 1..16 writers',
    'INDEX': 'writer index out of range',
    'INIT_ASS': 'TLOAD_ASS requires continuation',
    'CONT_LOAD': 'TLOAD requires INIT',
    'GM_DTYPE': 'requires matching GM dtype',
    'GM_SHAPE': 'GM region covering the writer',
    'STORE_SHAPE': 'GM region covering the final shape',
    'LEGACY': 'requires explicit writer geometry',
    'OPEN_SOURCE': 'no matching function',
    'SLOT_SOURCE': 'no matching function',
}


class HostLifecycle(unittest.TestCase):
    def test_order_guards_remain_active_in_release(self):
        # No producer arithmetic is simulated. Complete slots to test only the
        # C++ lifecycle guards, including stale retained carrier references.
        prefix = '''
#include <common/pto_local_cube_assembly.hpp>
using namespace pto;
using Access = detail::LocalCubeAssemblyAccess;
using W = VecTileM32<float,32,2>;
using A = LocalCubeAssembly<VecTileM32<float,32,16>,W,W,W,W,W,W,W>;
static_assert(A::Result::PhysicalCols == 14);
static_assert(A::Result::PhysicalBytes == 1792);
static_assert(A::Result::CapacityBytes == 2048);
static_assert(A::offset<6>() == 12);
int main() { A a;
'''
        cases = {
            'valid': ('auto s=a.writer<0>(); Access::complete(s);', True),
            'hole': ('auto s=a.writer<1>(); (void)s;', False),
            'early_finish': ('auto p=a.finish(); (void)p;', False),
            'repeat': ('auto s=a.writer<0>(); Access::complete(s); Access::complete(s);', False),
            'stale': ('auto s=a.writer<0>(); Access::complete(s); (void)Access::parent_data(s);', False),
            'closed': ('''auto complete = [&]<std::size_t I>() {
                auto s = a.writer<I>();
                if constexpr (I == 0) {
                  for (auto &v : Access::parent_data(s)) v = 0;
                }
                Access::complete(s);
              };
              [&]<std::size_t... I>(std::index_sequence<I...>) {
                (complete.template operator()<I>(), ...);
              }(std::make_index_sequence<7>{});
              auto p = a.finish(); (void)p;''', True),
            'double_finish': ('''auto complete = [&]<std::size_t I>() {
                auto s = a.writer<I>();
                if constexpr (I == 0) {
                  for (auto &v : Access::parent_data(s)) v = 0;
                }
                Access::complete(s);
              };
              [&]<std::size_t... I>(std::index_sequence<I...>) {
                (complete.template operator()<I>(), ...);
              }(std::make_index_sequence<7>{});
              auto p = a.finish(); (void)p; auto q = a.finish(); (void)q;''', False),
        }
        with tempfile.TemporaryDirectory(prefix='m32-assembly-host-') as work:
            work = Path(work)
            for name, (body, succeeds) in cases.items():
                source, output = work / (name + '.cpp'), work / name
                source.write_text(prefix + body + '\n}\n')
                result = subprocess.run([
                    os.environ.get('CXX', 'c++'), '-std=c++20', '-DNDEBUG',
                    '-D__linx', '-include', str(ROOT / 'test/linx_host_type_shim.hpp'),
                    '-I', str(ROOT / 'include'), str(source), '-o', str(output),
                ], text=True, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                ran = subprocess.run([str(output)], capture_output=True)
                self.assertEqual(ran.returncode == 0, succeeds, name)


class NativeLowering(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.compiler = os.environ.get('LINX_CXX')
        if not cls.compiler and os.environ.get('COMPILER_DIR'):
            cls.compiler = str(Path(os.environ['COMPILER_DIR']) / 'clang++')
        if not cls.compiler:
            raise unittest.SkipTest('set LINX_CXX or COMPILER_DIR for native M32 assembly checks')
        cls.work = tempfile.TemporaryDirectory(prefix='m32-assembly-native-')
        cls.directory = Path(cls.work.name)
        resource = Path(subprocess.check_output(
            [cls.compiler, '-print-resource-dir'], text=True).strip())
        overlay = cls.directory / 'resource'
        # Copy the real resource tree while excluding installed TileOP. Clang
        # puts its resource TileOP ahead of ordinary -I search paths.
        shutil.copytree(resource, overlay, ignore=lambda directory, names:
                        ['tileop-api'] if Path(directory) == resource / 'include' else [])
        shutil.copytree(ROOT / 'include', overlay / 'include/tileop-api')
        cls.flags = [cls.compiler, '-std=c++20', '-O2', '-mlxbc',
                     '-fenable-matrix', '-D__linx', '-DENABLE_TENSOR_INSTR',
                     '-resource-dir', str(overlay),
                     '-mllvm', '-enable-all-vector-as-tilereg=true']
        if os.environ.get('LINX_SYSROOT'):
            cls.flags += ['--sysroot=' + os.environ['LINX_SYSROOT']]
        cls.objdump = os.environ.get('LINX_OBJDUMP',
                                     str(Path(cls.compiler).parent / 'llvm-objdump'))

    @classmethod
    def tearDownClass(cls):
        cls.work.cleanup()

    def compile(self, *args):
        result = subprocess.run(self.flags + list(args), text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_negative_diagnostics(self):
        source = str(FIXTURES / 'Issue301LocalM32Negatives.cpp')
        self.compile('-fsyntax-only', source)
        for case, diagnostic in NEGATIVES.items():
            with self.subTest(case=case):
                result = subprocess.run(self.flags + [
                    '-fsyntax-only', '-DSHOULD_FAIL_301_' + case, source,
                ], text=True, capture_output=True)
                self.assertNotEqual(result.returncode, 0, case + ' compiled')
                self.assertRegex(result.stderr, diagnostic)

    def test_physical_sequences(self):
        source = str(FIXTURES / 'Issue301LocalM32TLoadAssembly.cpp')
        obj = self.directory / 'assembly.o'
        self.compile('-c', source, '-o', str(obj))
        ir = self.directory / 'assembly.ll'
        self.compile('-S', '-emit-llvm', source, '-o', str(ir))
        calls = [line for line in ir.read_text().splitlines()
                 if 'asm sideeffect' in line and 'B.ASSEMBLE' in line]
        self.assertEqual(len(calls), 22)
        init_calls = [line for line in calls if '->${0' in line or '->$0' in line]
        continuation_calls = [line for line in calls if 'call void asm' in line]
        self.assertEqual(len(init_calls), 8)
        self.assertEqual(len(continuation_calls), 14)
        for line in continuation_calls:
            self.assertRegex(line, r'B\.IOT \$0, mask=1111, last')
            self.assertNotIn('->', line.split('B.IOT')[1].split('B.ASSEMBLE')[0])
            self.assertNotRegex(line, r'"[=+]')
        dumped = subprocess.run([
            self.objdump, '-d', '--disassembler-options=no-tile-macros', str(obj),
        ], text=True, capture_output=True)
        # Older installed objdump lacks macro control and prints continuation
        # modifiers as unknown. Validate their raw instruction fields below.
        if dumped.returncode and 'Unrecognized disassembler option' in dumped.stderr:
            dumped = subprocess.run([self.objdump, '-d', str(obj)],
                                    text=True, capture_output=True)
        self.assertEqual(dumped.returncode, 0, dumped.stderr)
        sections = dict(re.findall(
            r'Disassembly of section \.text\.(issue301_\w+):\n([\s\S]*?)(?=Disassembly of section|\Z)',
            dumped.stdout))
        cases = {
            'two': (2, '512B', [2, 2], [0, 2], 4, 16),
            'three': (3, '1KB', [2, 2, 2], [0, 2, 4], 6, 24),
            'seven': (7, '2KB', [2] * 7, list(range(0, 14, 2)), 14, 56),
            'mixed': (3, '1KB', [2, 3, 1], [0, 2, 6], 7, 28),
            'single': (1, '1KB', [2], [0], 2, 8),
            'f16_tail': (2, '1KB', [2, 2], [0, 2], 7, 14),
            'i64': (2, '512B', [2, 2], [0, 2], 2, 16),
        }
        for name, (count, capacity, sizes, offsets, cols, stride) in cases.items():
            with self.subTest(case=name):
                text = sections['issue301_' + name]
                self.assertEqual(len(re.findall(r'BSTART\.TLSU\s+TLOAD,', text)), count)
                self.assertEqual(len(re.findall(r'BSTART\.TLSU\s+TSTORE,', text)), 1)
                self.assertEqual(len(re.findall(r'->t<', text)), 1)
                self.assertIn('->t<' + capacity + '>', text)
                self.assertEqual(len(re.findall(r'ND2M32\.normal', text)), count)
                self.assertNotIn('->lb2', text)
                self.assertNotRegex(text, r'TMOV|BSTART\.TEPL')
                modifiers = []
                for raw in re.findall(r'^\s*[0-9a-f]+:\s+([0-9a-f]{8})\s', text, re.M):
                    word = int(raw, 16)
                    if word & 0x707f == 0x1053:
                        modifiers.append((word >> 31, (word >> 11) & 1,
                                          (word >> 15) & 31, (word >> 20) & 2047,
                                          (word >> 7) & 15))
                self.assertEqual(modifiers, [
                    (int(i == 0), int(i == count - 1), 0, offsets[i], sizes[i])
                    for i in range(count)])
                self.assertEqual(len(re.findall(r'B\.IOT\s+t#1, mask=1111, last', text)), count)
                store = re.split(r'BSTART\.TLSU\s+TSTORE,', text)[1]
                self.assertRegex(store, rf'(?:C\.B\.DIMI\s+{cols},|B\.DIM\s+zero, {cols},)\s*->lb0')
                # The constant is materialized just before TSTORE, and its
                # register is the stride field of B.IOR.
                self.assertRegex(text, rf'(?:addi\s+zero, {stride},|c\.movi\s+{stride},)\s*->\w+')
        interleaved = sections['issue301_interleaved']
        # Allocators may put the temporary in a different Local queue. Compute
        # the parent's backward reference from the actual allocation history.
        bindings = re.findall(r'->([tu])<(\w+)>', interleaved)
        self.assertEqual([size for _, size in bindings], ['512B', '256B'])
        parent_queue, temporary_queue = (queue for queue, _ in bindings)
        parent_depth = 1 + int(temporary_queue == parent_queue)
        references = re.findall(r'B\.IOT\s+([tu]#\d+), mask=1111, last', interleaved)
        self.assertEqual(references, [f'{parent_queue}#{parent_depth}',
                                      f'{parent_queue}#{parent_depth}',
                                      f'{temporary_queue}#1'])


if __name__ == '__main__':
    unittest.main()
