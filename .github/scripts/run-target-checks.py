#!/usr/bin/env python3
"""Run independent gate suites with bounded parallelism and separate evidence."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
SUITES = (
    'test_scalar_shareability', 'test_tsel_contract', 'test_pto059_gap_api',
    'test_pto_main_contract', 'test_indexed_tlsu_range', 'test_indexed_tlsu_offset_types',
    'test_issue312_bias_macro', 'test_issue314_row_expand_offset',
    'test_packed_bits', 'test_packed_bf16_numeric', 'test_issue325_legacy_simt',
    'test_llvm138_narrow_tcmp', 'test_tcvt_select_regressions', 'test_predicate_adapters',
    'test_toolchain24_cube_layout', 'test_local_m32_tload_assembly',
    'test_shared_nd_to_cube',
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--workers', type=int, choices=(1, 2), default=2)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)

    def run(name):
        command = [sys.executable, str(ROOT / 'test' / (name + '.py')), '-v']
        started = time.monotonic()
        with (output / (name + '.log')).open('w') as log:
            try:
                process = subprocess.Popen(command, cwd=ROOT, stdout=log,
                                           stderr=subprocess.STDOUT,
                                           start_new_session=True)
                status = process.wait(timeout=900)
            except subprocess.TimeoutExpired:
                # Kill compiler descendants as well as the Python suite.
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                status = 124
                log.write('\nFAIL: suite exceeded 900 seconds\n')
        return {'suite': name, 'command': command, 'exit': status,
                'seconds': round(time.monotonic() - started, 2)}

    results = []
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        pending = [pool.submit(run, name) for name in SUITES]
        for future in as_completed(pending):
            result = future.result()
            results.append(result)
            print(f"{result['suite']}: exit={result['exit']} ({result['seconds']}s)", flush=True)
            if result['exit']:
                print((output / (result['suite'] + '.log')).read_text()[-16000:], flush=True)
            (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(any(result['exit'] for result in results))


if __name__ == '__main__':
    sys.exit(main())
