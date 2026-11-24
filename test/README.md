Method 1: in Makefile.common to specify COMPILER DIRECTORY(COMPILER_DIR) in your own environment

Method 2: invoke the make command with COMPILER_DIR in parameter e.g.:

`make TESTCASE=TAdd PLAT=linx COMPILER_DIR=/path-to-compiler-bin-dir`

## x86 plat
`make clean;make TESTCASE=TAdd PLAT=cpu`
`make clean;make TESTCASE=attention PLAT=cpu`

## arm plat
`make clean;make TESTCASE=TAdd PLAT=arm_sme`
`make clean;make TESTCASE=attention PLAT=arm_sme`
## linxISA plat
`make clean;make TESTCASE=TAdd PLAT=linx`
`make clean;make TESTCASE=attention PLAT=linx`

## build all cases

`make PLAT=linx build_all`

## output
- the elf will be in output dir/case_name

# compile py lib
- make clean;make TESTCASE=tileop_py PLAT=cpu PY_LIB=on

It will generate tileop_py.so in output/tileop_py

using golden cmp.py to compare python result with tileop_c result

- python3 golden_cmp/golden_cmp.py -i tadd(for example)

- for how to add new case, please ref to golden_cmp/README.md

# run case

## TCVT producer capacity regression (#330)

```sh
LINX_CXX=/path/to/clang++ LINX_SYSROOT=/path/to/sysroot \
LINX_TEST_CXXFLAGS='-mlinx-runtime-intra-abi -fno-exceptions -fno-builtin' \
  python3 test/test_tcvt_select_regressions.py -v
```

The odd-column cases compile TLOAD → TCVT → TSTORE, check the emitted TSize
and dimensions, and reject destinations that cannot hold the producer's full
physical rows. FP16/BF16 and both row-padded FP4 formats are covered. Use a
compiler with the paired odd-row descriptor capacity fix; syntax-only checks
cannot validate that backend path. These tests do not execute on a model or
hardware.

## Packed BF16 / legacy SIMT issue #325

```sh
LINX_CXX=/path/to/clang++ LINX_SYSROOT=/path/to/sysroot \
  python3 test/test_issue325_legacy_simt.py -v
python3 test/test_packed_bits.py -v
```

The contract test checks Janus packed conversion/addition, legacy SIMT
rejection, unsupported controls and invalid operand types. Raw-bit tests
cover floating and integer carriers. Run the independent numerical oracle:

```sh
python3 test/test_packed_bf16_numeric.py NumericHost -v
LINX_CXX=/path/to/clang++ LINX_SYSROOT=/path/to/sysroot \
LINX_MODEL_ROOT=/path/to/SuperScalarModel \
LINX_BF16_ARTIFACT_DIR=/tmp/issue325-results \
  python3 test/test_packed_bf16_numeric.py -v
```

The target test uses the model's documented `--stop_pc` observer at a separate
function reached after all outputs are stored. It verifies numerical results,
not formal kernel End or multi-PE lifecycle behavior. Compiler and model scalar
memory encodings must match. Without
`LINX_MODEL_ROOT`, target numerical execution is explicitly skipped. Optional
`LINX_BUILTINS` points to compiler-rt for an in-tree Clang. See
[the software profile and migration](../docs/tileop-usage/migration/legacy-simt-packed-bf16.md).

## set qemu simulator binary
Method 1: change the `QEMU` variable in Makefile.common to the qemu-linx binary location

Method 2: pass the qemu-linx binary location through make command parameter e.g.:
`make TESTCASE=TAdd QEMU=path-to-qemu-linx-bin-exe`

## run a single case

`make TESTCASE=TAdd sim`

## run all cases
`make sim_all`

