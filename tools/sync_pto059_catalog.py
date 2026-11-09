#!/usr/bin/env python3
"""Refresh pinned PTO catalogs from Git objects without switching the spec checkout."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('spec_repo', type=Path)
    args = parser.parse_args()
    lock = json.loads((ROOT / 'contracts/pto-isa-0.59.0-sources.json').read_text())
    from check_pto_catalog_alignment import source_pins
    # Check all Git sources first; local files may be absent during a refresh.
    for kind, pin in source_pins(lock):
        for path, tree in pin.get('normative_trees', {}).items():
            actual = subprocess.check_output(['git', '-C', str(args.spec_repo), 'rev-parse',
                                              pin['commit'] + ':' + path], text=True).strip()
            if actual != tree:
                raise SystemExit('source normative tree does not match pin: ' + kind + '/' + path)
    outputs = []
    for kind, source in source_pins(lock):
        data = subprocess.check_output(['git', '-C', str(args.spec_repo), 'show',
                                       source['commit'] + ':spec/catalog/tile-operations.json'])
        if hashlib.sha256(data).hexdigest() != source['catalog_sha256']:
            raise SystemExit('source catalog does not match pin: ' + kind)
        outputs.append((ROOT / 'contracts' / source['catalog'], data))
    for path, data in outputs:
        path.write_bytes(data)


if __name__ == '__main__':
    main()
