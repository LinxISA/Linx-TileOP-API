#!/usr/bin/env python3
"""Build SuperNPUBench configurations and audit TileOp macro presentation.

Only the Python standard library is needed by this driver. Benchmark data
generators may additionally need numpy and ml_dtypes.
"""
import argparse
import collections
import concurrent.futures
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import sys
import time

HEADER = re.compile(r'^BSTART\.(TEPL|TLSU|CUBE|GMOV|TIMG2COL)\b')
LINE = re.compile(r'^\s*([0-9a-f]+):\s+(.+)$')
GAP = re.compile(r"TileOp header '([^']+)' at 0x([0-9a-fA-F]+)")


def write_json(path, value):
    temp = path.with_suffix(path.suffix + '.tmp')
    temp.write_text(json.dumps(value, indent=2) + '\n')
    temp.replace(path)


def revision(path):
    return subprocess.check_output(['git', '-C', str(path), 'rev-parse', 'HEAD'], text=True).strip()


def scheduled(date):
    """Even days since a fixed Beijing calendar date: stable across months."""
    return (date - dt.date(2026, 11, 3)).days % 2 == 0


def run(command, cwd, env, log, timeout=600, memory_gib=3):
    command = ['/usr/bin/prlimit', f'--as={memory_gib * 1024**3}', '--core=0', '--', *command]
    with log.open('w') as output:
        with subprocess.Popen(command, cwd=cwd, env=env, stdout=output,
                              stderr=subprocess.STDOUT, start_new_session=True) as process:
            try:
                return process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                return 124


def collect(source, output, env):
    """Execute upstream shell loops while recording make, without building."""
    collector = output / 'record_make.py'
    collector.write_text('import json,os,sys\nwith open(os.environ["COLLECT_OUT"],"a") as f: f.write(json.dumps({"cwd":os.getcwd(),"args":sys.argv[1:]})+"\\n")\n')
    scripts = sorted((source / 'benchmark/one-level-arch/test').rglob('compile.all'))
    scripts = [p for p in scripts if 'V0' not in p.parts]
    scripts += sorted((source / 'microbenchmark').glob('*/compile.all'))
    if not scripts:
        raise RuntimeError('No supported compile.all scripts found')
    configs, errors = [], []
    for i, script in enumerate(scripts):
        record = output / 'logs' / f'collect-{i}.jsonl'
        record.write_text('')
        shell = ('make() { ' + shlex.quote(sys.executable) + ' ' + shlex.quote(str(collector)) +
                 ' "$@"; }; python3() { return 0; }; rm() { return 0; }; '
                 'export -f make python3 rm; source ./compile.all')
        # Some scripts finish with a report that expects ELF files. During
        # discovery python3 is deliberately suppressed; actual make results
        # are captured independently for every configuration below.
        status = run(['bash', '-c', shell], script.parent,
                     dict(env, COLLECT_OUT=str(record)), output/'logs'/f'collect-{i}.log', 120)
        rows = record.read_text().splitlines()
        if status or not rows:
            errors.append({'script': str(script.relative_to(source)), 'exit': status,
                           'reason': 'configuration discovery failed or produced no builds'})
        for line in rows:
            item = json.loads(line)
            item['script'] = str(script.relative_to(source))
            # A normalized upstream cwd + complete arguments identify a case
            # across runners. Compiler installation paths do not identify cases.
            normalized = [re.sub(r'^QSMLA_SPARSE_DATA_ROOT=.*', 'QSMLA_SPARSE_DATA_ROOT=<data>',
                                re.sub(r'^COMPILER_DIR=.*', 'COMPILER_DIR=<toolchain>', a))
                          .replace(str(source), '<source>') for a in item['args']]
            identity = json.dumps([item['script'], normalized], separators=(',', ':'))
            item['key'] = hashlib.sha256(identity.encode()).hexdigest()[:24]
            configs.append(item)
    configs = list({x['key']: x for x in configs}.values())
    for i, item in enumerate(configs):
        item['id'] = i
    write_json(output/'configurations.json', configs)
    return configs, errors


def prepare_data(source, output, env):
    """Generate embedded inputs; no simulator or numeric validation is run."""
    errors = []
    qli = source/'benchmark/one-level-arch/test/solution/qli'
    if (qli/'src/gen_qli_golden.py').exists():
        cmd = [sys.executable, 'src/gen_qli_golden.py', '--mode', 'gen', '--outdir',
               'compare/current', '--sq', '32', '--skv', '8192', '--topk', '512',
               '--g', '64', '--d', '128']
        if run(cmd, qli, env, output/'logs/qli-data.log'):
            errors.append('QLI embedded input preparation failed; see logs/qli-data.log')
        else:
            # Both configurations embed the same inputs. Prebuild this shared
            # object before concurrent private builds to avoid an object race.
            if run([str(Path(env['COMPILER_DIR'])/'clang'), '-c', 'src/qli_check_data.s',
                    '-o', 'src/qli_check_data.o'], qli, env, output/'logs/qli-object.log'):
                errors.append('QLI data object preparation failed')
    qsmla = source/'benchmark/one-level-arch/test/solution/quant_sparse_flash_mla'
    if (qsmla/'run_qsmla_cpu_reference.py').exists():
        for case in ['swa_small', 'hca_small', 'csa_small', 'ori_sparse_small', 'ori_cmp_sparse_small']:
            if run([sys.executable, 'run_qsmla_cpu_reference.py', '--case', case,
                    '--dtype', 'HIF8', '--output-root', env['QSMLA_DATA_ROOT']],
                   qsmla, env, output/'logs'/f'qsmla-{case}.log'):
                errors.append(f'QSMLA data preparation failed: {case}')
    return errors


def listing(text):
    return {int(m[1], 16): m[2] for line in text.splitlines() if (m := LINE.match(line))}


def audit(physical, macro, diagnostics, macro_exit, physical_exit):
    headers = {a: s for a, s in listing(physical).items() if HEADER.match(s)}
    presented = listing(macro)
    gaps = [{'address': int(m[2], 16), 'header': m[1]} for m in GAP.finditer(diagnostics)]
    gaps += [{'address': int(m[1], 16), 'header': 'invalid-macro'} for m in
             re.finditer(r'TileOp macro at 0x([0-9a-fA-F]+) failed', diagnostics)]
    errors = []
    addresses = {g['address'] for g in gaps}
    if len(addresses) != len(gaps):
        errors.append('Duplicate diagnostic addresses')
    if physical_exit or macro_exit != int(bool(gaps)):
        errors.append('Unexpected objdump exit status')
    summary = re.search(r'LinxV5 error: (\d+) TileOp bundle\(s\)', diagnostics)
    if gaps and (not summary or int(summary[1]) != len(gaps)):
        errors.append('Diagnostic summary count mismatch')
    for gap in gaps:
        gap['instruction'] = headers.get(gap['address'], '')
        if not gap['instruction']:
            errors.append('Diagnostic has no physical TileOp header')
        gap['address'] = hex(gap['address'])
    operations = collections.Counter()
    for address, instruction in headers.items():
        words = instruction.split()
        operation = words[0] if words[0] in ['BSTART.GMOV', 'BSTART.TIMG2COL'] else words[1].rstrip(',')
        operations[operation] += 1
        if address not in addresses:
            form = presented.get(address, '')
            if not form or HEADER.match(form) or '<unknown>' in form or '<incomplete' in form:
                errors.append(f'Header {hex(address)} has neither macro nor diagnostic')
    return dict(total_bundles=len(headers), covered_bundles=len(headers)-len(gaps),
                gaps=gaps, operations=dict(operations), unknown=physical.count('<unknown>'),
                audit_errors=errors)


def build(config, source, output, tools, env, timeout, memory):
    case = output/'cases'/f'{config["id"]:04d}'
    case.mkdir(parents=True, exist_ok=True)
    result = dict(config, group=str(Path(config['cwd']).relative_to(source)), case=f'cases/{config["id"]:04d}')
    args = [a for a in config['args'] if a not in ['all', 'diss', '-B'] and not a.startswith('-j')]
    # The upstream QSMLA discovery command carries its default /tmp data
    # location. Point every build at the isolated data prepared for this run;
    # otherwise a stale runner-local dataset could silently affect coverage.
    args = [f'QSMLA_SPARSE_DATA_ROOT={env["QSMLA_DATA_ROOT"]}'
            if a.startswith('QSMLA_SPARSE_DATA_ROOT=') else a
            for a in args]
    cmd = ['make', '-j1', *args, 'all', f'COMPILER_DIR={tools}',
           f'OBJ_ROOT={case}/obj', f'TARGET={case}/program.elf']
    result['command'] = cmd
    started = time.monotonic()
    try:
        status = run(cmd, config['cwd'], env, case/'build.log', timeout, memory)
        result['build_exit'] = status
        log = (case/'build.log').read_text(errors='replace')
        result['resource_failure'] = status == 124 or bool(re.search(
            r'bad_alloc|Cannot allocate memory|out of memory|Killed|Resource temporarily unavailable', log))
        if status:
            result['status'] = 'build-failure'
            result['failure'] = '\n'.join(line for line in log.splitlines() if re.search(
                r'error:|UNREACHABLE|undefined symbol:|No rule to make target', line))[:4000]
        else:
            texts, exits = {}, {}
            for name, options in [('macro', []), ('physical', ['-M', 'no-tile-macros'])]:
                with (case/f'{name}.txt').open('w') as out, (case/f'{name}.err').open('w') as err:
                    with subprocess.Popen([str(tools/'llvm-objdump'), '-d', '--no-show-raw-insn',
                                           *options, str(case/'program.elf')], stdout=out, stderr=err,
                                          start_new_session=True) as process:
                        try:
                            exits[name] = process.wait(timeout=120)
                        except subprocess.TimeoutExpired:
                            os.killpg(process.pid, signal.SIGKILL)
                            process.wait()
                            raise RuntimeError(f'{name} disassembly timed out')
                texts[name] = (case/f'{name}.txt').read_text()
            result.update(audit(texts['physical'], texts['macro'], (case/'macro.err').read_text(),
                                exits['macro'], exits['physical']))
            result['status'] = 'survey-error' if result['audit_errors'] else 'built'
    except Exception as error:
        result.update(status='survey-error', error=str(error))
    result['seconds'] = round(time.monotonic()-started, 2)
    write_json(case/'result.json', result)
    return result


def compare(results, previous):
    old = {r['key']: r for r in previous.get('cases', [])}
    regressions, added, removed = [], [], sorted(set(old)-{r['key'] for r in results})
    for result in results:
        baseline = old.get(result['key'])
        if baseline is None:
            added.append(result['key'])
            if previous and result['status'] != 'built':
                regressions.append(dict(key=result['key'],case=result['case'],
                                        status=result['status'],new_gaps={},reason='new failing configuration'))
            continue
        before = collections.Counter(g['instruction'] for g in baseline.get('gaps', []))
        after = collections.Counter(g['instruction'] for g in result.get('gaps', []))
        new_gaps = dict(after-before)
        if ((baseline['status'] == 'built' and result['status'] != 'built') or new_gaps
                or result.get('unknown',0)>baseline.get('unknown',0)):
            regressions.append(dict(key=result['key'], case=result['case'], new_gaps=new_gaps,
                                    status=result['status']))
    return dict(regressions=regressions, added=added, removed=removed,
                baseline=bool(previous))


def report(output, results, metadata, infrastructure, previous):
    built = [r for r in results if r['status'] == 'built']
    total = sum(r['total_bundles'] for r in built)
    missing = sum(len(r['gaps']) for r in built)
    delta = compare(results, previous)
    infrastructure += [f"{r['case']}: resource limit or timeout" for r in results if r.get('resource_failure')]
    infrastructure += [f"{r['case']}: {r.get('error', r.get('audit_errors'))}" for r in results if r['status']=='survey-error']
    summary = dict(metadata, configurations=len(results), compiled=len(built),
                   build_failures=sum(r['status']=='build-failure' for r in results),
                   total_bundles=total, covered_bundles=total-missing, uncovered_bundles=missing,
                   coverage_percent=100*(total-missing)/total if total else None,
                   tileop_elf_pass=sum(r['total_bundles']>0 and not r['gaps'] for r in built),
                   tileop_elf_fail=sum(bool(r['gaps']) for r in built),
                   elf_without_tileop=sum(r['total_bundles']==0 for r in built),
                   unknown_instructions=sum(r['unknown'] for r in built),
                   infrastructure_errors=infrastructure, **delta)
    if not total:
        infrastructure.append('No TileOp headers found: cannot report coverage')
    if infrastructure:
        summary['outcome'] = 'infrastructure-failure'
    elif delta['regressions']:
        summary['outcome'] = 'regression'
    elif not previous:
        summary['outcome'] = 'baseline-created'
    else:
        summary['outcome'] = 'no-regression'
    write_json(output/'summary.json', summary)
    # A compact comparison snapshot excludes large logs and absolute paths.
    snapshot = dict(summary, cases=[{k:r[k] for k in ['key','status','case','group']} |
                                   {'gaps':r.get('gaps',[]),'unknown':r.get('unknown',0)} for r in results])
    write_json(output/'comparison.json', snapshot)
    with (output/'cases.csv').open('w') as f:
        writer=csv.writer(f);writer.writerow(['key','case','group','status','total','uncovered','seconds','failure'])
        for r in results:writer.writerow([r['key'],r['case'],r['group'],r['status'],r.get('total_bundles',''),len(r.get('gaps',[])),r['seconds'],r.get('failure',r.get('error',''))])
    operations=collections.Counter(); gaps=collections.Counter()
    with (output/'gaps.csv').open('w') as f:
        writer=csv.writer(f);writer.writerow(['key','case','address','header','instruction'])
        for r in built:
            operations.update(r['operations'])
            for g in r['gaps']:
                writer.writerow([r['key'],r['case'],g['address'],g['header'],g['instruction']])
                words=g['instruction'].split()
                gaps[words[0] if words[0] in ['BSTART.GMOV','BSTART.TIMG2COL'] else words[1].rstrip(',')]+=1
    with (output/'operations.csv').open('w') as f:
        writer=csv.writer(f);writer.writerow(['operation','total','covered','uncovered'])
        for name,count in sorted(operations.items()):writer.writerow([name,count,count-gaps[name],gaps[name]])
    rate=f'{summary["coverage_percent"]:.4f}%' if total else 'unavailable'
    lines=['# SuperNPUBench TileOp macro coverage','',f'Outcome: **{summary["outcome"]}**','',
           f'Coverage: **{rate}** = {total-missing:,} / {total:,} static TileOp occurrences.',
           f'Configurations: {len(results)}; ELF built: {len(built)}; build failures: {summary["build_failures"]}.',
           f'ELF containing TileOp: {summary["tileop_elf_pass"]} pass, {summary["tileop_elf_fail"]} have gaps; {summary["elf_without_tileop"]} ELF contain no TileOp.',
           f'Unknown physical instructions: {summary["unknown_instructions"]}.', '',
           'Build failures are excluded from the occurrence denominator. This is presentation coverage, not runtime or ISA support coverage. Two-level architecture and archived V0 suites are excluded.', '',
           '## Revisions','',*[f'- {k}: `{v}`' for k,v in metadata.items()], '',
           '## Changes against previous successful survey','',
           f'Baseline available: {delta["baseline"]}; regressions: {len(delta["regressions"])}; added configurations: {len(delta["added"])}; removed: {len(delta["removed"])}.',
           'Comparisons match normalized configurations and compare gap counts per physical operation/dtype, independently of code addresses. Added/removed configurations are reported separately.', '',
           '## Coverage by operation','', '| Operation | Total | Uncovered | Coverage |','|---|---:|---:|---:|']
    for name,count in sorted(operations.items(), key=lambda x:gaps[x[0]], reverse=True):
        lines.append(f'| {name} | {count} | {gaps[name]} | {100*(count-gaps[name])/count:.4f}% |')
    lines += ['', '## Infrastructure diagnostics', '', *[f'- {s}' for s in infrastructure], '',
              '## Build failures', '', *[f'- `{r["case"]}` `{r["group"]}`: see build.log' for r in results if r['status']=='build-failure'], '',
              'Full details: summary.json, comparison.json, cases.csv, operations.csv, gaps.csv and cases/*/{result.json,build.log,macro.txt,macro.err,physical.txt,physical.err}.','']
    (output/'REPORT.md').write_text('\n'.join(lines))
    return summary


def self_check(tools, output):
    directory=output/'self-check';directory.mkdir()
    for name, source, expected in [
        ('valid','TADD <Row=8, Col=64, FP32>, T#1, T#2, ->T<2KB>\n',0),
        ('invalid','BSTART.TEPL TADD, FP32\n',1)]:
        assembly=directory/f'{name}.s';assembly.write_text(source)
        subprocess.run([str(tools/'llvm-mc'),'-triple=linx64v5','-filetype=obj',str(assembly),'-o',str(directory/f'{name}.o')],check=True)
        macro=subprocess.run([str(tools/'llvm-objdump'),'-d','--no-show-raw-insn',str(directory/f'{name}.o')],capture_output=True,text=True,check=False)
        physical=subprocess.run([str(tools/'llvm-objdump'),'-d','--no-show-raw-insn','-M','no-tile-macros',str(directory/f'{name}.o')],capture_output=True,text=True,check=False)
        (directory/f'{name}.txt').write_text(macro.stdout+macro.stderr)
        checked=audit(physical.stdout,macro.stdout,macro.stderr,macro.returncode,physical.returncode)
        if macro.returncode!=expected or checked['audit_errors'] or bool(checked['gaps'])!=bool(expected):
            raise RuntimeError(f'Macro checker self-test failed: {name}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--schedule-date', help='Print whether Beijing YYYY-MM-DD is on the 48-hour cadence')
    parser.add_argument('--source',type=Path)
    parser.add_argument('--toolchain',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--previous',type=Path)
    parser.add_argument('--llvm-sha')
    parser.add_argument('--tileop-sha')
    parser.add_argument('--workers',type=int,default=2,choices=[1,2,3])
    parser.add_argument('--timeout',type=int,default=600)
    parser.add_argument('--memory-gib',type=int,default=3,choices=[1,2,3,4])
    args=parser.parse_args()
    if args.schedule_date:
        print(str(scheduled(dt.date.fromisoformat(args.schedule_date))).lower());return 0
    if not all([args.source,args.toolchain,args.output,args.llvm_sha,args.tileop_sha]):parser.error('source, toolchain, output, llvm-sha and tileop-sha are required')
    source=args.source.resolve();output=args.output.resolve();tools=args.toolchain.resolve()/'bin'
    output.mkdir(parents=True,exist_ok=True);(output/'logs').mkdir(exist_ok=True)
    if (output/'summary.json').exists():raise RuntimeError('Use a fresh output directory for each source revision')
    env=dict(os.environ,COMPILER_DIR=str(tools),GEN_GOLDEN='0',MAKEFLAGS='-j1',
             OPENBLAS_NUM_THREADS='1',QSMLA_DATA_ROOT=str(output/'qsmla-data'))
    metadata=dict(llvm_sha=args.llvm_sha,tileop_sha=args.tileop_sha,supernpubench_sha=revision(source),
                  toolchain_build_sha=revision(args.toolchain.resolve().parents[1]),
                  started_at=dt.datetime.now(dt.timezone.utc).isoformat(),
                  run_url=os.environ.get('GITHUB_SERVER_URL','https://github.com')+'/'+os.environ.get('GITHUB_REPOSITORY','')+'/actions/runs/'+os.environ.get('GITHUB_RUN_ID','local'))
    previous=json.loads(args.previous.read_text()) if args.previous and args.previous.exists() else {}
    results=[];infrastructure=[]
    try:
        version=subprocess.check_output([str(tools/'clang++'),'--version'],text=True)
        (output/'logs/compiler-version.txt').write_text(version)
        if args.llvm_sha not in version:raise RuntimeError('Installed compiler revision does not match resolved LLVM SHA')
        resource_dir=Path(subprocess.check_output([str(tools/'clang'),'-print-resource-dir'],text=True).strip())
        header=(resource_dir/'include/tileop-api/common/pto_tileop_api_revision.hpp').read_text()
        if f'PTO_TILEOP_API_REVISION "{args.tileop_sha}"' not in header:raise RuntimeError('Installed TileOP headers have the wrong revision')
        self_check(tools,output)
        configs,errors=collect(source,output,env);infrastructure.extend(errors)
        infrastructure.extend(prepare_data(source,output,env))
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures=[pool.submit(build,c,source,output,tools,env,args.timeout,args.memory_gib) for c in configs]
            for future in concurrent.futures.as_completed(futures):
                results.append(future.result())
                if len(results)%20==0:print(f'{len(results)}/{len(configs)} complete',flush=True)
    except Exception as error:
        infrastructure.append(str(error))
    summary=report(output,sorted(results,key=lambda x:x['id']),metadata,infrastructure,previous)
    print(json.dumps({k:v for k,v in summary.items() if k not in ['regressions','added','removed']},indent=2))
    return int(summary['outcome'] in ['infrastructure-failure','regression'])


if __name__=='__main__':
    sys.exit(main())
