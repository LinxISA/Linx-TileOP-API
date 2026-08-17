# Linx TileOP API

Header-only C++ TileOP bindings for the LinxISA v0.58 architectural contract.

The normative instruction names, encodings, and execution-engine classification come from the
pinned LinxISA projection in
[`contracts/linxisa-v0.58-engine-ops.json`](contracts/linxisa-v0.58-engine-ops.json). The public
bindings emit canonical `BSTART.VEC` and `BSTART.SFU` aliases plus the named TLSU and CUBE forms
such as `BSTART.TLOAD` and `BSTART.TMATMUL`. The historical TEPL spelling is not emitted by this
library.

## Documentation

- [Execution engines and operations](docs/tileop-usage/engines.md)
- [TLSU operations](docs/tileop-usage/tlsu.md)
- [CUBE operations](docs/tileop-usage/cube.md)
- [Tile and Shared-register constraints](docs/tileop-usage/constraints.md)
- [Layout helpers](docs/tileop-usage/layout.md)
- [Comparison operations](docs/tileop-usage/cmp.md)
- [Fixed-point matrix wrappers](docs/tileop-usage/tmatmul-fixp.md)

## Validation

```sh
make check
```

To refresh the pinned projection from a clean, exact LinxISA v0.58 checkout:

```sh
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
python3 tools/generate_engine_docs.py
make check
```
