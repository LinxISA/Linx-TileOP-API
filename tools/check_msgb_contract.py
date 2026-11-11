#!/usr/bin/env python3
"""Check the opt-in MSGB pin and C projection without moving active ISA pins."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def check(spec=None):
    pin = json.loads((ROOT / 'contracts/pto-msgb-sources.json').read_text())
    header = (ROOT / 'docs/proposals/sync-intrinsics/backend/msgb_registers.h').read_text()
    constants = {k: int(v, 16) for k, v in re.findall(
        r'PTO_(MSGB[A-Z0-9_]*)\s*=\s*(0x[0-9a-f]+)', header)}
    for reg in pin['registers']:
        name = reg['name']
        if re.fullmatch(r'MSGBD[2-9]', name):
            actual = constants['MSGBD1'] + int(name[5:]) - 1
        else:
            actual = constants[name]
        if actual != int(reg['address'], 16):
            raise ValueError('register projection differs: ' + name)
    if len(pin['registers']) != 28:
        raise ValueError('expected all 28 MSGB definitions')
    if spec:
        sources = {}
        for path, expected in pin['files'].items():
            data = subprocess.check_output(['git', '-C', str(spec), 'show',
                                            pin['commit'] + ':' + path])
            sources[path] = data
            if hashlib.sha256(data).hexdigest() != expected:
                raise ValueError('pinned source differs: ' + path)
        catalog = json.loads(sources['spec/catalog/system-registers.json'])
        registers = [r for r in catalog['registers']
                     if r['name'].startswith('MSGB')]
        if registers != pin['registers']:
            raise ValueError('register projection differs from pinned catalog')
    return pin


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--spec-repo', type=Path)
    args = parser.parse_args()
    check(args.spec_repo)
    print('MSGB draft register projection matches the pinned #426 contract')
