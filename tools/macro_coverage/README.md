# Scheduled SuperNPUBench macro coverage

The `SuperNPUBench macro coverage` workflow runs every day at 02:00
Asia/Shanghai (UTC 18:00 the previous day), a 24-hour schedule. GitHub may
delay scheduled runs, so actual start times may be more than 24 hours apart.
Manual dispatch always runs immediately and does not change the daily
schedule. The workflow must be merged into default branch `linx` before
scheduled execution is possible.

## Shared compiler

Both the required gate and this survey call
`.github/actions/prepare-toolchain`. It preserves the gate's exact LLVM-SHA
v2 cache key, legacy fallback, incremental/clean recovery, and full sysroot.
Cache saves precede candidate TileOP header installation. Each job restores
its own working copy; no installation directory is concurrently modified by
two workflows. PR merge-ref caches remain PR-local; the push gate and survey
on default branch `linx` populate the shared cache.

The survey fixes the LLVM branch SHA, TileOP `linx` SHA and SuperNPUBench `main`
SHA for each run. Compiler/header revision checks and positive/negative MC
self-tests reject stale or missing macro diagnostics. A cache miss can still
require a full toolchain build. Runner OS/architecture and build recipe must
remain compatible with the existing cache; bump its version when these change.

## Survey

`check.py` discovers non-archived one-level `compile.all` scripts plus
cube/fixp/memory/scalar/vector microbenchmarks. Shell loops and conditions are
executed while `make` is recorded; golden generation is suppressed during
discovery. QLI and QSMLA embedded data are prepared separately before builds.
These explicit data adapters follow current upstream scripts: new benchmark
data requirements may need another adapter. Discovery/data errors fail the
survey rather than silently presenting an incomplete run as success.

Each configuration uses private objects and ELF, upstream flags and `make -j1`.
Default macro objdump and `-M no-tile-macros` listings are checked per address.
Coverage = (recognized physical TileOp headers - diagnostic occurrences) /
recognized physical TileOp headers. Failed builds are excluded from this
denominator. Scalar-only ELF are not counted as TileOp passes. `<unknown>`
physical instructions are reported separately. This is static macro
presentation coverage; no simulator or numerical verification runs here.

Hosted Linux runners run at most two compilers (one on smaller machines),
with a 3 GiB per-process address-space limit. A systemd scope reserves 2 GiB
for the runner and caps the survey at 8 GiB RAM, 512 MiB swap and two CPUs.
Timeouts kill complete process groups; resource failures cannot become an
accepted baseline. Full-job timeout is 330 minutes. The PR gate concurrency
group and required check name are unchanged.

## Reports and baseline policy

Every run uploads reports, diagnostic logs, ELF and both disassembly views as
an Actions artifact retained for 30 days. Reports still upload after failures;
an interrupted run without a complete summary is never published as coverage.
Objects and generated QSMLA input data are excluded from artifacts.

A separate, write-scoped publish job stores small reports under
`coverage-reports/history/<UTC-date>/<run-id>-<attempt>/` and per-run trend
points under `trend/`. The job uses the Git data API and never runs report
content. `gaps.csv` and binaries remain in the artifact. Large repositories
should periodically archive report history according to their retention policy.

`latest/comparison.json` is the last accepted baseline. First complete run
creates it even if it has existing gaps/build failures. Subsequent runs fail
on new build failures or additional gap occurrences in matching configurations,
and on new unknown physical instructions. Normalized full make configurations
identify cases; gap comparison uses physical operation/dtype counts rather
than addresses. Added/removed configurations are listed separately. New failing
configurations fail; new successfully built configurations establish their
own initial gap counts. Regressions are archived but never advance `latest`.
Infrastructure failures also never advance it.

The workflow needs Actions enabled and permission to write `coverage-reports`
using `GITHUB_TOKEN`. If branch rules prohibit that write, permit this workflow
or choose an approved report publication target; artifact upload remains
available independently. No PAT is required. Keep the default branch `linx`.

Run driver tests locally:

```sh
python3 -m unittest discover -s test -p 'test_macro_coverage.py'
python3 test/test_pr_gate_cache_order.py
```

For a manual local survey with an installed matching toolchain:

```sh
python3 tools/macro_coverage/check.py \
  --source /path/to/SuperNPUBench --toolchain /path/to/toolchain \
  --output /path/to/new-report --llvm-sha FULL_LLVM_SHA \
  --tileop-sha FULL_TILEOP_SHA --workers 2
```

Use the workflow's systemd memory scope when running locally, and provide
`--previous /path/to/comparison.json` for regression detection. Benchmark data
preparation additionally requires `numpy` and `ml_dtypes`.
