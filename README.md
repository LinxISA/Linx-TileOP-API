# Linx TileOP API

Header-only C++ TileOP bindings for the LinxISA / PTO ISA 0.58.6 architectural contract.
The checked-in catalog and documentation are generated against the PTO ISA 0.58.6
baseline (`0.58.6.0`, ABI `pto-isa-0.58.6-mode-function-v1`).

Installed headers expose their provenance through
`<common/pto_tileop_api_revision.hpp>`. `PTO_TILEOP_API_VERSION` and
`PTO_TILEOP_API_SPEC_VERSION` identify the API/spec contract, while
`PTO_TILEOP_API_REVISION` records the exact TileOP-API Git commit used by
`make install`. Feature macros such as `PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX`
allow consumers to reject toolchains that predate a required fix.

The normative operation inventory comes from the checked-in PTO catalog at
[`contracts/pto-isa-0.58.6-tile-operations.json`](contracts/pto-isa-0.58.6-tile-operations.json).
The public bindings retain the unique compiled `BSTART.TEPL` carrier for VEC/SFU
source compatibility and emit the named TLSU/CUBE operation forms accepted by
the current Linx compiler. The generated engine index is a wrapper compatibility
view, not a replacement for the PTO ASL/NDF semantics.

## Documentation

- [Execution engines and operations](docs/tileop-usage/generated/engines.md)
- [TLSU operations](docs/tileop-usage/tlsu/load-store-move/TLOAD.md)
- [CUBE operations](docs/tileop-usage/cube/matrix-matrix/TMATMUL.md)
- [Tile and Shared-register constraints](docs/tileop-usage/concepts/tile-constraints.md)
- [Layout helpers](docs/tileop-usage/layout-and-rearrangement/layout/TCONCAT.md)
- [PTO ISA 0.58.6 CUBE layout operations](docs/tileop-usage/layout-and-rearrangement/layout/TPERMUTE.md)
- [Comparison operations](docs/tileop-usage/elementwise-tile-tile/logical/TCMP.md)
- [Sorting operations](docs/tileop-usage/irregular-and-complex/sorting/TSORT.md)
- [Fixed-point matrix wrappers](docs/tileop-usage/options.md)

## Validation

```sh
make check
```

Target compilation requires a Linx LLVM build supporting the PTO ISA 0.58.6
encoding ABI. The old PTO ISA 0.58.3 Linx LLVM requirement belongs only to the
historical migration fixtures under `test/tileop_api` and
`docs/tileop-usage/migration/pto-0583-migration.md`; it is not the current API
baseline. Run the catalog and documentation checks before target compilation.

To refresh the pinned projection from the clean, exact reviewed LinxISA
authority commit recorded by the generator:

```sh
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
python3 tools/generate_engine_docs.py
make check
```
