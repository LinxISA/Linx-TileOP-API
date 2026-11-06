# SuperNPUBench TileOp macro coverage

Outcome: **no-regression**

Coverage: **100.0000%** = 251,916 / 251,916 static TileOp occurrences.
Configurations: 896; ELF built: 781; build failures: 115.
ELF containing TileOp: 655 pass, 0 have gaps; 126 ELF contain no TileOp.
Unknown physical instructions: 0.

Build failures are excluded from the occurrence denominator. This is presentation coverage, not runtime or ISA support coverage. Two-level architecture and archived V0 suites are excluded.

## Revisions

- llvm_sha: `9c28145d1446d24e6862032eb1b9eabdabf8029e`
- tileop_sha: `bb4de26b4a6d7342bb486fa5e218bb476e1486af`
- supernpubench_sha: `c1d9a4f982d218bd14605376fa8c72eb105887c2`
- toolchain_build_sha: `e6a31efb4cfb17f1f1c33265cbf6dbb61bbba156`
- started_at: `2026-11-05T23:33:22.222383+00:00`
- run_url: `https://github.com/LinxISA/Linx-TileOP-API/actions/runs/42129080594`

## Changes against previous successful survey

Baseline available: True; regressions: 0; added configurations: 0; removed: 0.
Comparisons match normalized configurations and compare gap counts per physical operation/dtype, independently of code addresses. Added/removed configurations are reported separately.

## Coverage by operation

| Operation | Total | Uncovered | Coverage |
|---|---:|---:|---:|
| TEXPANDS | 1074 | 0 | 100.0000% |
| TLOAD | 141637 | 0 | 100.0000% |
| TANDS | 23 | 0 | 100.0000% |
| TCMPS | 100 | 0 | 100.0000% |
| TSEL | 156 | 0 | 100.0000% |
| TSHRS | 21 | 0 | 100.0000% |
| TMULS | 5889 | 0 | 100.0000% |
| TADDS | 73 | 0 | 100.0000% |
| TXOR | 13 | 0 | 100.0000% |
| TROWMAX | 2576 | 0 | 100.0000% |
| TROWARGMAX | 13 | 0 | 100.0000% |
| TCMP | 42 | 0 | 100.0000% |
| TSTORE | 3646 | 0 | 100.0000% |
| TCOLMAX | 6 | 0 | 100.0000% |
| TCOLARGMAX | 2 | 0 | 100.0000% |
| TCI | 27 | 0 | 100.0000% |
| TROWEXPAND | 9 | 0 | 100.0000% |
| TROWMIN | 4 | 0 | 100.0000% |
| TCVT | 7407 | 0 | 100.0000% |
| TADD | 1771 | 0 | 100.0000% |
| TCOLEXPAND | 15 | 0 | 100.0000% |
| TCOLMIN | 4 | 0 | 100.0000% |
| MGATHER | 30 | 0 | 100.0000% |
| TREMS | 4 | 0 | 100.0000% |
| TDIVS | 31 | 0 | 100.0000% |
| TSUB | 1333 | 0 | 100.0000% |
| TMOV | 2399 | 0 | 100.0000% |
| MSCATTER | 8 | 0 | 100.0000% |
| TMATMUL | 4444 | 0 | 100.0000% |
| TMAXS | 850 | 0 | 100.0000% |
| TMINS | 848 | 0 | 100.0000% |
| TMUL | 1451 | 0 | 100.0000% |
| TEXP | 863 | 0 | 100.0000% |
| TRECIP | 286 | 0 | 100.0000% |
| TMATMULMX | 1134 | 0 | 100.0000% |
| TMAX | 2680 | 0 | 100.0000% |
| TROWEXPANDEXPDIF | 4218 | 0 | 100.0000% |
| TROWSUM | 2816 | 0 | 100.0000% |
| TROWEXPANDMUL | 2280 | 0 | 100.0000% |
| 119 | 415 | 0 | 100.0000% |
| TFMA | 1582 | 0 | 100.0000% |
| TROWEXPANDDIV | 213 | 0 | 100.0000% |
| TMATMUL.ACC | 51772 | 0 | 100.0000% |
| TMATMULMX.ACC | 7278 | 0 | 100.0000% |
| MGATHER.ADD | 8 | 0 | 100.0000% |
| 118 | 13 | 0 | 100.0000% |
| TAND | 8 | 0 | 100.0000% |
| MSCATTER.MASK | 7 | 0 | 100.0000% |
| TSUBS | 13 | 0 | 100.0000% |
| BSTART.TIMG2COL | 2 | 0 | 100.0000% |
| TTRI | 4 | 0 | 100.0000% |
| TSHLS | 7 | 0 | 100.0000% |
| TCOLSUM | 141 | 0 | 100.0000% |
| TCOLEXPANDMUL | 76 | 0 | 100.0000% |
| TROWEXPANDADD | 37 | 0 | 100.0000% |
| TROWEXPANDSUB | 43 | 0 | 100.0000% |
| TSQRT | 8 | 0 | 100.0000% |
| TNOT | 4 | 0 | 100.0000% |
| TORS | 4 | 0 | 100.0000% |
| TABS | 30 | 0 | 100.0000% |
| TMATMUL.BIAS | 4 | 0 | 100.0000% |
| TGEMV | 3 | 0 | 100.0000% |
| TGEMV.BIAS | 2 | 0 | 100.0000% |
| TGEMV.ACC | 2 | 0 | 100.0000% |
| TGEMVMX | 1 | 0 | 100.0000% |
| BSTART.GMOV | 1 | 0 | 100.0000% |
| MGATHER.MASK | 2 | 0 | 100.0000% |
| TPREFETCH | 1 | 0 | 100.0000% |
| TCOLEXPANDADD | 2 | 0 | 100.0000% |
| TCOLEXPANDDIV | 2 | 0 | 100.0000% |
| TCOLEXPANDEXPDIF | 2 | 0 | 100.0000% |
| TCOLEXPANDMAX | 2 | 0 | 100.0000% |
| TCOLEXPANDMIN | 2 | 0 | 100.0000% |
| TCOLEXPANDSUB | 2 | 0 | 100.0000% |
| TDIV | 4 | 0 | 100.0000% |
| TGATHER | 1 | 0 | 100.0000% |
| ESAVE | 1 | 0 | 100.0000% |
| TLOG | 2 | 0 | 100.0000% |
| TMIN | 4 | 0 | 100.0000% |
| TNEG | 4 | 0 | 100.0000% |
| TOR | 2 | 0 | 100.0000% |
| 117 | 1 | 0 | 100.0000% |
| TRELU | 2 | 0 | 100.0000% |
| TREM | 3 | 0 | 100.0000% |
| TROWEXPANDMAX | 2 | 0 | 100.0000% |
| TROWEXPANDMIN | 2 | 0 | 100.0000% |
| TRSQRT | 2 | 0 | 100.0000% |
| TSCATTER | 1 | 0 | 100.0000% |
| TSELS | 2 | 0 | 100.0000% |
| TSHL | 2 | 0 | 100.0000% |
| TSHR | 2 | 0 | 100.0000% |
| 120 | 1 | 0 | 100.0000% |
| TXORS | 2 | 0 | 100.0000% |

## Infrastructure diagnostics


## Build failures

- `cases/0000` `benchmark/one-level-arch/test/kernel/argmaxv2`: see build.log
- `cases/0006` `benchmark/one-level-arch/test/kernel/argmaxv2`: see build.log
- `cases/0007` `benchmark/one-level-arch/test/kernel/argmaxv2`: see build.log
- `cases/0016` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0017` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0020` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0021` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0022` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0023` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0032` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0038` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0039` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0040` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0048` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0056` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0062` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0063` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0064` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0072` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0079` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0085` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0086` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0087` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0094` `benchmark/one-level-arch/test/kernel/fa`: see build.log
- `cases/0096` `benchmark/one-level-arch/test/kernel/hashtable_lookup`: see build.log
- `cases/0108` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0123` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0127` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0131` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0135` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0139` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0143` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0152` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0255` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0264` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0288` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0294` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0297` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0303` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0306` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0312` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0318` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0327` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0330` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0373` `benchmark/one-level-arch/test/kernel/matmul`: see build.log
- `cases/0380` `benchmark/one-level-arch/test/kernel/reduction/reduceprod_row`: see build.log
- `cases/0382` `benchmark/one-level-arch/test/kernel/transpose`: see build.log
- `cases/0384` `benchmark/one-level-arch/test/solution/conv2d`: see build.log
- `cases/0387` `benchmark/one-level-arch/test/solution/gather_v2`: see build.log
- `cases/0391` `benchmark/one-level-arch/test/solution/group_token_vec`: see build.log
- `cases/0392` `benchmark/one-level-arch/test/solution/group_token_vec`: see build.log
- `cases/0393` `benchmark/one-level-arch/test/solution/group_token_vec`: see build.log
- `cases/0394` `benchmark/one-level-arch/test/solution/matmul_test`: see build.log
- `cases/0395` `benchmark/one-level-arch/test/solution/matmul_test`: see build.log
- `cases/0396` `benchmark/one-level-arch/test/solution/matmul_test`: see build.log
- `cases/0397` `benchmark/one-level-arch/test/solution/mega_moe`: see build.log
- `cases/0398` `benchmark/one-level-arch/test/solution/mega_moe`: see build.log
- `cases/0399` `benchmark/one-level-arch/test/solution/mega_moe`: see build.log
- `cases/0413` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0415` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0416` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0418` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0420` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0421` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0422` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0423` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0424` `benchmark/one-level-arch/test/solution/quant/dynamic_mx_quant`: see build.log
- `cases/0431` `benchmark/one-level-arch/test/solution/quant_batch_matmul_test`: see build.log
- `cases/0446` `microbenchmark/cube`: see build.log
- `cases/0447` `microbenchmark/cube`: see build.log
- `cases/0448` `microbenchmark/cube`: see build.log
- `cases/0508` `microbenchmark/fixp`: see build.log
- `cases/0509` `microbenchmark/fixp`: see build.log
- `cases/0510` `microbenchmark/fixp`: see build.log
- `cases/0511` `microbenchmark/fixp`: see build.log
- `cases/0514` `microbenchmark/fixp`: see build.log
- `cases/0515` `microbenchmark/fixp`: see build.log
- `cases/0516` `microbenchmark/fixp`: see build.log
- `cases/0517` `microbenchmark/fixp`: see build.log
- `cases/0518` `microbenchmark/fixp`: see build.log
- `cases/0524` `microbenchmark/fixp`: see build.log
- `cases/0525` `microbenchmark/fixp`: see build.log
- `cases/0526` `microbenchmark/fixp`: see build.log
- `cases/0527` `microbenchmark/fixp`: see build.log
- `cases/0528` `microbenchmark/fixp`: see build.log
- `cases/0531` `microbenchmark/fixp`: see build.log
- `cases/0532` `microbenchmark/fixp`: see build.log
- `cases/0533` `microbenchmark/fixp`: see build.log
- `cases/0537` `microbenchmark/fixp`: see build.log
- `cases/0538` `microbenchmark/fixp`: see build.log
- `cases/0539` `microbenchmark/fixp`: see build.log
- `cases/0567` `microbenchmark/fixp`: see build.log
- `cases/0568` `microbenchmark/fixp`: see build.log
- `cases/0576` `microbenchmark/memory`: see build.log
- `cases/0745` `microbenchmark/vector`: see build.log
- `cases/0746` `microbenchmark/vector`: see build.log
- `cases/0747` `microbenchmark/vector`: see build.log
- `cases/0748` `microbenchmark/vector`: see build.log
- `cases/0765` `microbenchmark/vector`: see build.log
- `cases/0766` `microbenchmark/vector`: see build.log
- `cases/0767` `microbenchmark/vector`: see build.log
- `cases/0768` `microbenchmark/vector`: see build.log
- `cases/0769` `microbenchmark/vector`: see build.log
- `cases/0770` `microbenchmark/vector`: see build.log
- `cases/0771` `microbenchmark/vector`: see build.log
- `cases/0772` `microbenchmark/vector`: see build.log
- `cases/0773` `microbenchmark/vector`: see build.log
- `cases/0836` `microbenchmark/vector`: see build.log
- `cases/0837` `microbenchmark/vector`: see build.log
- `cases/0857` `microbenchmark/vector`: see build.log
- `cases/0858` `microbenchmark/vector`: see build.log
- `cases/0859` `microbenchmark/vector`: see build.log
- `cases/0860` `microbenchmark/vector`: see build.log
- `cases/0861` `microbenchmark/vector`: see build.log
- `cases/0862` `microbenchmark/vector`: see build.log

Full details: summary.json, comparison.json, cases.csv, operations.csv, gaps.csv and cases/*/{result.json,build.log,macro.txt,macro.err,physical.txt,physical.err}.
