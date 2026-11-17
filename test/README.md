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

## set qemu simulator binary
Method 1: change the `QEMU` variable in Makefile.common to the qemu-linx binary location

Method 2: pass the qemu-linx binary location through make command parameter e.g.:
`make TESTCASE=TAdd QEMU=path-to-qemu-linx-bin-exe`

## run a single case

`make TESTCASE=TAdd sim`

## run all cases
`make sim_all`

Matrix-MX operand-order regression (issue #319):

```bash
LINX_CXX=/path/to/clang++ LINX_SYSROOT=/path/to/sysroot \
  python3 test/test_mx_operand_order.py -v
```

This checks the compiled Local and Shared source streams for plain MX, ACC,
and BIAS, all four scale-presence combinations, basic/options calls, RowMax,
CScale, and explicit groupM. Separate kernels with real Tile producers must
also assemble. The test overlays this checkout's headers into a temporary
compiler resource directory. It skips when no target compiler is configured;
CI configures the compiler explicitly. Numerical model execution is separate.
