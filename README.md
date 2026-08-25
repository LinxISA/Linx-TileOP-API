# Linx TileOP API

Header-only C++ TileOP bindings for the LinxISA / PTO ISA v0.58.3 architectural contract.

The normative instruction names, encodings, and execution-engine classification come from the
pinned LinxISA projection in
[`contracts/linxisa-v0.58-engine-ops.json`](contracts/linxisa-v0.58-engine-ops.json). The public
bindings retain the unique compiled `BSTART.TEPL` carrier for VEC/SFU source
compatibility and emit the named TLSU/CUBE operation forms accepted by the
current Linx compiler. The generated engine index shows the corresponding
canonical aliases.

## Documentation

- [Execution engines and operations](docs/tileop-usage/engines.md)
- [TLSU operations](docs/tileop-usage/tlsu.md)
- [CUBE operations](docs/tileop-usage/cube.md)
- [Tile and Shared-register constraints](docs/tileop-usage/constraints.md)
- [Layout helpers](docs/tileop-usage/layout.md)
- [Comparison operations](docs/tileop-usage/cmp.md)
- [Sorting operations](docs/tileop-usage/sort.md)
- [Fixed-point matrix wrappers](docs/tileop-usage/tmatmul-fixp.md)
- [Hosted four-PE group runtime](docs/tileop-usage/group-runtime.md)

## Validation

```sh
make check
```

Target compilation additionally requires the matching PTO ISA 0.58.3 Linx
LLVM. Run `test/tileop_api/compile.all`, `run_negatives.sh`, and the
disassembly checks with that exact compiler build; a 0.58.1 compiler is
expected to reject the nine-field B.FPATR and new layout/PEMode encodings.

To refresh the pinned projection from the clean, exact reviewed LinxISA
authority commit recorded by the generator:

```sh
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
python3 tools/generate_engine_docs.py
make check
```
