#!/usr/bin/env python3
"""Publish small reports using the Git data API; never execute report content."""
import argparse
import base64
import datetime as dt
import json
import os
from pathlib import Path
import subprocess


def api(endpoint, method='GET', body=None):
    cmd=['gh','api',endpoint,'--method',method]
    if body is not None:cmd+=['--input','-']
    result=subprocess.run(cmd,input=json.dumps(body) if body is not None else None,
                          capture_output=True,text=True)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return json.loads(result.stdout)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact',type=Path,required=True)
    args=parser.parse_args()
    summaries=list(args.artifact.rglob('summary.json'))
    if len(summaries)!=1:
        raise RuntimeError('No unique finished report found; refusing to publish partial coverage')
    report=summaries[0].parent
    summary=json.loads(summaries[0].read_text())
    repo=os.environ['GITHUB_REPOSITORY'];prefix=f'repos/{repo}'
    branch='coverage-reports'
    # List branch references so authentication/network errors are never
    # mistaken for an absent branch and an empty history.
    refs=api(f'{prefix}/git/matching-refs/heads/{branch}')
    ref=next((r for r in refs if r['ref']==f'refs/heads/{branch}'),None)
    parent=ref['object']['sha'] if ref else None
    base_tree=api(f'{prefix}/git/commits/{parent}')['tree']['sha'] if parent else None
    run=f'{os.environ["GITHUB_RUN_ID"]}-{os.environ["GITHUB_RUN_ATTEMPT"]}'
    date=dt.datetime.now(dt.timezone.utc).strftime('%Y-%m-%d')
    destination=f'history/{date}/{run}'
    tree=[]
    # Keep binary artifacts and multi-megabyte diagnostics out of git.
    for name in ['REPORT.md','summary.json','comparison.json','operations.csv','cases.csv']:
        path=report/name
        if not path.is_file():raise RuntimeError(f'Missing report attachment: {name}')
        blob=api(f'{prefix}/git/blobs','POST',dict(content=base64.b64encode(path.read_bytes()).decode(),encoding='base64'))
        tree.append(dict(path=f'{destination}/{name}',mode='100644',type='blob',sha=blob['sha']))
        if summary['outcome'] in ['baseline-created','no-regression']:
            tree.append(dict(path=f'latest/{name}',mode='100644',type='blob',sha=blob['sha']))
    # Small index permits plotting trends without downloading every report.
    point=json.dumps({k:summary[k] for k in ['started_at','run_url','outcome','llvm_sha',
                     'tileop_sha','supernpubench_sha','coverage_percent','configurations',
                     'compiled','build_failures','total_bundles','uncovered_bundles']},indent=2)
    tree.append(dict(path=f'trend/{date}-{run}.json',mode='100644',type='blob',content=point))
    payload=dict(tree=tree)
    if base_tree:payload['base_tree']=base_tree
    new_tree=api(f'{prefix}/git/trees','POST',payload)['sha']
    commit=api(f'{prefix}/git/commits','POST',dict(message=f'Macro coverage {date}: {summary["outcome"]}',tree=new_tree,parents=[parent] if parent else []))['sha']
    if parent:
        api(f'{prefix}/git/refs/heads/{branch}','PATCH',dict(sha=commit,force=False))
    else:
        api(f'{prefix}/git/refs','POST',dict(ref=f'refs/heads/{branch}',sha=commit))
    print(f'https://github.com/{repo}/tree/{branch}/{destination}')


if __name__=='__main__':main()
