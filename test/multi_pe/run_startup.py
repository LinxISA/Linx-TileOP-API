#!/usr/bin/env python3
"""Compile this tree's common entry + current compiler-rt, then run four PE tests.

The model harness is SuperScalarModel tests/gfrun_invocation_test/hosted_startup.cpp.
It exercises actual ELF instructions with skewed PE scheduling and fresh launches.
All tool paths are explicit; no SDK download or prebuilt ELF substitution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--sysroot', type=Path, required=True)
    parser.add_argument('--worker', type=Path, required=True)
    parser.add_argument('--runtime-archive', type=Path,
                        help='also verify worker extraction from built compiler-rt')
    parser.add_argument('--model-test', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    cc = args.compiler.resolve()
    common = [str(cc), '--target=linx64v5-unknown-linux-musl',
              '--sysroot=' + str(args.sysroot.resolve()), '-mcpu=janus', '-O2',
              '-ffreestanding', '-fno-builtin', '-fno-stack-protector',
              '-mlinx-runtime-intra-abi', '-I' + str(root / 'include')]
    sources = [root / 'test/common/gfrun_runtime_adapter.c',
               args.worker.resolve(), Path(__file__).with_name('startup.c')]
    commands = []

    def run(name, argv):
        commands.append(argv)
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        result = subprocess.run(argv, capture_output=True, text=True, timeout=180)
        (out / (name + '.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise SystemExit(f'{name} failed: {out / (name + ".log")}')
        return result.stdout

    objects = []
    for i, src in enumerate(sources):
        obj = out / f'{i}.o'
        run(f'compile-{i}', common + ['-c', str(src), '-o', str(obj)])
        objects.append(str(obj))
    elf = out / 'startup.elf'
    link_objects = objects
    if args.runtime_archive:
        link_objects = [objects[0], objects[2], '-u', '__linx_group_worker_start',
                        str(args.runtime_archive.resolve())]
    run('link', [str(cc.with_name('ld.lld')), '-m', 'elf64llinxv5',
                 '-e', '_start', '-o', str(elf), *link_objects])
    symbols = run('symbols', ['readelf', '-sW', str(elf)])
    report = re.search(r'\b([0-9a-fA-F]+)\s+\d+\s+OBJECT\s+GLOBAL\s+DEFAULT\s+\S+\s+startup_report\b', symbols)
    if not report:
        raise SystemExit('startup_report symbol missing')
    run('disassembly', [str(cc.with_name('llvm-objdump')), '-d',
                       '--disassembler-options=no-tile-macros', str(elf)])
    for mode in ['leader-first', 'workers-first', 'missing', 'fault-relaunch']:
        output = run(mode, [str(args.model_test.resolve()), str(elf),
                            '0x' + report[1], mode])
        print(output.strip().splitlines()[-1])
    inputs = [cc, args.model_test.resolve(), *sources, elf]
    if args.runtime_archive:
        inputs.append(args.runtime_archive.resolve())
    (out / 'hashes.json').write_text(json.dumps({
        str(p): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in inputs
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
