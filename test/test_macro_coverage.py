#!/usr/bin/env python3
"""Exercise coverage diagnostics, case discovery and regression decisions."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest
from unittest import mock

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('coverage_check',ROOT/'tools/macro_coverage/check.py')
coverage=importlib.util.module_from_spec(spec)
spec.loader.exec_module(coverage)
publish_spec=importlib.util.spec_from_file_location('coverage_publish',ROOT/'tools/macro_coverage/publish.py')
publisher=importlib.util.module_from_spec(publish_spec)
publish_spec.loader.exec_module(publisher)


class CoverageTest(unittest.TestCase):
    def test_clean_and_unfoldable_occurrences(self):
        physical='  10: BSTART.TEPL TADD, FP32\n  20: BSTART.TLSU TSTORE, FP32\n'
        macro='  10: TADD <Row=8, Col=64, FP32>, T#1, T#2, ->T<2KB>\n  20: BSTART.TLSU TSTORE, FP32\n'
        diagnostic="LinxV5 error: TileOp header 'BSTART.TLSU' at 0x20 has no complete macro instruction form\nLinxV5 error: 1 TileOp bundle(s) have no complete macro instruction form\n"
        result=coverage.audit(physical,macro,diagnostic,1,0)
        self.assertEqual(result['total_bundles'],2)
        self.assertEqual(result['covered_bundles'],1)
        self.assertEqual(result['gaps'][0]['instruction'],'BSTART.TLSU TSTORE, FP32')
        self.assertEqual(result['audit_errors'],[])

    def test_silent_physical_fallback_cannot_pass(self):
        text='  0: BSTART.TEPL TADD, FP32\n'
        self.assertTrue(coverage.audit(text,text,'',0,0)['audit_errors'])

    def test_invalid_macro_and_bad_count(self):
        p='  0: BSTART.CUBE TMATMUL, FP32\n'
        m='  0: <incomplete TileOp macro>\n'
        d='LinxV5 error: TileOp macro at 0x0 failed macro-form validation\nLinxV5 error: 2 TileOp bundle(s) have no complete macro instruction form\n'
        result=coverage.audit(p,m,d,1,0)
        self.assertEqual(result['gaps'][0]['header'],'invalid-macro')
        self.assertIn('Diagnostic summary count mismatch',result['audit_errors'])

    def test_regression_is_address_independent_and_tracks_new_failures(self):
        old=dict(key='a',case='cases/0',status='built',gaps=[dict(address='0x10',instruction='BSTART.TLSU TMOV, U32')])
        new=dict(old,gaps=[dict(address='0x30',instruction='BSTART.TLSU TMOV, U32')])
        self.assertFalse(coverage.compare([new],{'cases':[old]})['regressions'])
        new['gaps']*=2
        self.assertTrue(coverage.compare([new],{'cases':[old]})['regressions'])
        self.assertTrue(coverage.compare([dict(key='new',case='cases/1',status='build-failure')],{'cases':[old]})['regressions'])
        self.assertFalse(coverage.compare([dict(key='new',case='cases/1',status='build-failure')],{})['regressions'])

    def test_discovery_executes_shell_loops_and_stabilizes_paths(self):
        keys=[]
        for _ in range(2):
            with tempfile.TemporaryDirectory() as tmp:
                root=Path(tmp);source=root/'source';out=root/'report';(out/'logs').mkdir(parents=True)
                case=source/'microbenchmark/vector';case.mkdir(parents=True)
                (case/'compile.all').write_text('for mode in A B; do make TESTCASE="$mode" COMPILER_DIR="$COMPILER_DIR" QSMLA_SPARSE_DATA_ROOT="$QSMLA_DATA_ROOT" diss; done\n')
                configs,errors=coverage.collect(source,out,dict(os.environ,COMPILER_DIR=str(root/'compiler'),QSMLA_DATA_ROOT=str(root/'data')))
                self.assertEqual(errors,[])
                self.assertEqual(len(configs),2)
                keys.append([c['key'] for c in configs])
        self.assertEqual(keys[0],keys[1])

    def test_build_uses_isolated_embedded_data_and_outputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);source=root/'source';out=root/'report';tools=root/'bin'
            source.mkdir();out.mkdir();tools.mkdir()
            data=str(out/'qsmla-data')
            (source/'Makefile').write_text('all:\n\t@test "$(QSMLA_SPARSE_DATA_ROOT)" = "$(EXPECTED_DATA_ROOT)"\n\t@touch "$(TARGET)"\n')
            objdump=tools/'llvm-objdump'
            objdump.write_text('#!/bin/sh\ncase "$*" in\n *no-tile-macros*) echo "  0: BSTART.TEPL TADD, FP32";;\n *) echo "  0: TADD <Row=8, Col=64, FP32>, T#1, T#2, ->T<2KB>";;\nesac\n')
            objdump.chmod(0o755)
            config=dict(id=0,key='a',cwd=str(source),script='compile.all',
                        args=['QSMLA_SPARSE_DATA_ROOT=/tmp/stale-data'])
            result=coverage.build(config,source,out,tools,
                dict(os.environ,QSMLA_DATA_ROOT=data,EXPECTED_DATA_ROOT=data),10,1)
            self.assertEqual(result['status'],'built',result)
            self.assertEqual(result['covered_bundles'],1)
            self.assertTrue((out/'cases/0000/program.elf').is_file())

    def test_resource_failure_never_becomes_baseline(self):
        with tempfile.TemporaryDirectory() as tmp:
            output=Path(tmp)
            result=dict(key='a',id=0,status='build-failure',case='cases/0000',group='microbenchmark/vector',seconds=1,resource_failure=True)
            summary=coverage.report(output,[result],{},[],{})
            self.assertEqual(summary['outcome'],'infrastructure-failure')
            self.assertIsNone(summary['coverage_percent'])

    def test_workflow_baseline_lookup_handles_history_without_baseline(self):
        workflow=(ROOT/'.github/workflows/macro-coverage.yml').read_text()
        block=workflow.split('      - name: Read previous accepted comparison\n',1)[1]
        script=textwrap.dedent(block.split('        run: |\n',1)[1].split('\n      - name:',1)[0])
        for scenario in ['no-branch','no-baseline','accepted','denied']:
            with self.subTest(scenario=scenario), tempfile.TemporaryDirectory() as tmp:
                directory=Path(tmp)
                gh=directory/'gh'
                gh.write_text('#!/usr/bin/env python3\nimport os,sys\n'
                              'scenario=os.environ["SCENARIO"]\n'
                              'baseline="contents/latest" in " ".join(sys.argv)\n'
                              'if scenario=="denied":\n print("HTTP 403",file=sys.stderr);sys.exit(1)\n'
                              'if scenario=="no-branch" or (scenario=="no-baseline" and baseline):\n'
                              ' print("HTTP 404",file=sys.stderr);sys.exit(1)\n'
                              'print("{}")\n')
                gh.chmod(0o755)
                result=subprocess.run(['bash','-c',script.replace('/tmp/coverage-',str(directory/'coverage-'))],
                    cwd=directory,env=dict(os.environ,PATH=str(directory)+os.pathsep+os.environ['PATH'],
                                          GITHUB_REPOSITORY='owner/repo',SCENARIO=scenario),
                    capture_output=True,text=True)
                self.assertEqual(result.returncode,1 if scenario=='denied' else 0,result.stderr)
                self.assertEqual((directory/'coverage/previous.json').exists(),scenario=='accepted')

    def test_publication_archives_regressions_without_advancing_baseline(self):
        for outcome in ['baseline-created','no-regression','regression','infrastructure-failure']:
            with self.subTest(outcome=outcome), tempfile.TemporaryDirectory() as tmp:
                artifact=Path(tmp)
                summary=dict(outcome=outcome,started_at='now',run_url='url',llvm_sha='llvm',
                             tileop_sha='tileop',supernpubench_sha='bench',coverage_percent=75,
                             configurations=1,compiled=1,build_failures=0,total_bundles=4,uncovered_bundles=1)
                for name in ['REPORT.md','comparison.json','operations.csv','cases.csv']:
                    (artifact/name).write_text('test')
                (artifact/'summary.json').write_text(json.dumps(summary))
                calls=[]
                def fake_api(endpoint,method='GET',body=None):
                    calls.append((endpoint,method,body))
                    if 'matching-refs' in endpoint:return [{'ref':'refs/heads/coverage-reports','object':{'sha':'parent'}}]
                    if method=='GET':return {'tree':{'sha':'old-tree'}}
                    return {'sha':'new-object'}
                with mock.patch.object(publisher,'api',side_effect=fake_api), mock.patch.dict(
                    os.environ,GITHUB_REPOSITORY='owner/repo',GITHUB_RUN_ID='123',GITHUB_RUN_ATTEMPT='1'), mock.patch(
                    'sys.argv',['publish.py','--artifact',str(artifact)]):
                    publisher.main()
                trees=[body for endpoint,method,body in calls if endpoint.endswith('/git/trees')]
                latest=[entry for entry in trees[0]['tree'] if entry['path'].startswith('latest/')]
                self.assertEqual(bool(latest),outcome in ['baseline-created','no-regression'])
                self.assertTrue(any(entry['path'].startswith('history/') for entry in trees[0]['tree']))
                self.assertFalse(calls[-1][2]['force'])


if __name__=='__main__':unittest.main()
