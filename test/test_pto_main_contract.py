"""Pinned main/#424 provenance and ordinary indexed B.IOR regression coverage."""
import copy
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import test_shared_nd_to_cube as native

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('catalog_check', ROOT / 'tools/check_pto_catalog_alignment.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


class MainSourceContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.work = tempfile.TemporaryDirectory(prefix='tileop-source-contract-')
        cls.addClassCleanup(cls.work.cleanup)
        cls.root = Path(cls.work.name)
        cls.repo = cls.root / 'spec'
        cls.repo.mkdir()
        (cls.root / 'contracts').mkdir()
        cls.lock = json.loads((ROOT / 'contracts/pto-isa-0.59.0-sources.json').read_text())
        def git(*args):
            return subprocess.check_output(['git', '-C', str(cls.repo), *args],
                env=dict(os.environ, GIT_AUTHOR_NAME='Contract test', GIT_AUTHOR_EMAIL='test@example.invalid',
                         GIT_COMMITTER_NAME='Contract test', GIT_COMMITTER_EMAIL='test@example.invalid'),
                stderr=subprocess.PIPE).decode().strip()
        git('init', '-q')
        (cls.repo / 'spec/catalog').mkdir(parents=True)
        (cls.repo / 'asl').mkdir()
        (cls.repo / 'asl/contract.asl').write_text('RegSrc1 and RegSrc2 must be zero\n')
        for kind, pin in checker.source_pins(cls.lock):
            data = (ROOT / 'contracts' / pin['catalog']).read_bytes()
            (cls.root / 'contracts' / pin['catalog']).write_bytes(data)
            (cls.repo / 'spec/catalog/tile-operations.json').write_bytes(data)
            git('add', '.')
            git('commit', '-qm', kind)
            pin['commit'] = git('rev-parse', 'HEAD')
            if kind.startswith('extension:'):
                pin['normative_trees'] = {path: git('rev-parse', 'HEAD:' + path) for path in ('asl', 'spec')}
        cls.lock['active']['normative_trees'] = {
            path: git('rev-parse', cls.lock['active']['commit'] + ':' + path) for path in ('asl', 'spec')}
        (cls.repo / 'asl/contract.asl').write_text('RegSrc1 and RegSrc2 encode bounds\n')
        git('add', '.')
        git('commit', '-qm', 'same catalog with incompatible normative contract')
        cls.divergent = git('rev-parse', 'HEAD')

    def verify(self, lock=None, ref=None):
        with patch.object(checker, 'ROOT', self.root):
            return checker.verify_sources(lock or self.lock, self.repo, ref)

    def test_pinned_git_objects_ignore_checkout(self):
        self.assertEqual(self.verify(), [])

    def test_same_catalog_does_not_hide_normative_drift(self):
        failures = self.verify(ref=self.divergent)
        self.assertTrue(any('review and repin' in item for item in failures), failures)
        self.assertTrue(any('normative tree differs: asl' in item for item in failures), failures)

    def test_corrupt_catalog_hash_is_rejected(self):
        lock = copy.deepcopy(self.lock)
        lock['active']['catalog_sha256'] = '0' * 64
        self.assertTrue(any('active catalog' in item for item in self.verify(lock)))

    def test_extension_hash_and_normative_drift_are_rejected(self):
        lock = copy.deepcopy(self.lock)
        lock['extensions']['indexed_tlsu_range']['catalog_sha256'] = '0' * 64
        self.assertTrue(any('extension:indexed_tlsu_range catalog' in item for item in self.verify(lock)))
        lock = copy.deepcopy(self.lock)
        lock['extensions']['indexed_tlsu_range']['normative_trees']['asl'] = '0' * 40
        self.assertTrue(any('extension normative tree differs' in item for item in self.verify(lock)))

    def test_indexed_docs_use_main_units_and_atomic_types(self):
        pages = list((ROOT / 'docs/tileop-usage/tlsu/memory').glob('M*.md'))
        pages = [p for p in pages if p.stem.startswith(('MGATHER', 'MSCATTER'))]
        self.assertEqual(len(pages), 23)
        for page in pages:
            content = page.read_text()
            self.assertIn('地址为 `Base + displacement`', content)
            self.assertIn('IndexedByteRange', content)
            self.assertIn('#424', content)
            self.assertNotIn('StrideGPR', content)
            self.assertNotIn('逻辑线性元素下标', content)
        for name, expected in [('MGATHER_CAS', {'U16', 'U32', 'U64'}),
                               ('MGATHER_ADD', {'FP16', 'BF16', 'FP32', 'FP64', 'S32', 'U32', 'U64'}),
                               ('MSCATTER_INC', {'U32'}), ('MSCATTER_MAX', {'S32', 'S64', 'U32', 'U64'})]:
            content = (ROOT / 'docs/tileop-usage/tlsu/memory' / (name + '.md')).read_text()
            data = re.search(r'^\| 数据 Tile \| (.*) \|$', content, re.M)[1]
            self.assertEqual(set(re.findall(r'(?:BF|FP|S|U)\d+', data)), expected, name)

    def test_indexed_schema_rejects_surplus_selectors(self):
        header = (ROOT / 'include/jcore/template_asm.hpp').read_text()
        self.assertEqual(checker.verify_indexed_contract(header), [])
        changed = header.replace('B.IOR [%[base]], []', 'B.IOR [%[base],%[min],%[max]], []')
        self.assertNotEqual(changed, header)
        self.assertTrue(checker.verify_indexed_contract(changed))
        changed = header.replace('B.IOR [%[base]], []', 'B.IOR [%[base]], [%[result]]')
        self.assertTrue(checker.verify_indexed_contract(changed))
        changed = header.replace('%[base],%[RangeMin],%[RangeMax]', '%[base],%[RangeMax],%[RangeMin]')
        self.assertNotEqual(changed, header)
        self.assertTrue(checker.verify_indexed_contract(changed))


class MainIndexedNative(unittest.TestCase):
    setUpClass = classmethod(native.NativeLowering.setUpClass.__func__)
    tearDownClass = classmethod(native.NativeLowering.tearDownClass.__func__)
    compile = native.NativeLowering.compile

    def test_all_23_indexed_objects_bind_only_base(self):
        obj = self.directory / 'indexed-main.o'
        self.compile('-c', str(native.FIXTURES / 'IndexedTLSUMain.cpp'), '-o', str(obj))
        assembly = subprocess.check_output([str(Path(self.compiler).parent / 'llvm-objdump'),
            '-d', '--no-show-raw-insn', '--disassembler-options=no-tile-macros', str(obj)], text=True)
        bundles = re.findall(r'BSTART.TLSU\s+(M(?:GATHER|SCATTER)(?:\.[A-Z]+)?),'
                             r'(.*?)(?=BSTART|$)', assembly, re.S)
        self.assertEqual(len({op for op, _ in bundles}), 23, assembly)
        for op, bundle in bundles:
            with self.subTest(op=op):
                records = re.findall(r'B.IOR\s+\[([^]]*)\],\s*\[([^]]*)\]', bundle)
                self.assertEqual(len(records), 1, bundle)
                source, destination = records[0]
                slots = [item.strip() for item in source.split(',')]
                self.assertTrue(slots[0], bundle)
                self.assertTrue(all(item == 'zero' for item in slots[1:]), bundle)
                self.assertFalse(destination.strip(), bundle)
                self.assertNotIn('ExecMaskPresent', bundle)


if __name__ == '__main__':
    unittest.main()
