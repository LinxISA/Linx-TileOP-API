# Linx TileOP API

Header-only C++ TileOP bindings for the LinxISA / PTO ISA 0.59.0 architectural
contract. The checked-in catalog and documentation are generated against the
published PTO ISA `v0.59.0.0` release (commit `b016532`, ABI
`pto-isa-0.59.0-mode-function-v1`). The release manifest records
`specification_status: draft` and limits implementation conformance to
`b-fpatr-and-tcvt-e8m0-asl-proven-other-numeric-domains-unproven`; this binding
carries that identity without claiming a finalized specification or full
numeric conformance. The semantic engine inventory remains the LinxISA v0.58
authority projection, whose 119-operation Tile catalog is byte-identical to the
release catalog.

Installed headers expose their provenance through
`<common/pto_tileop_api_revision.hpp>`. `PTO_TILEOP_API_VERSION` and
`PTO_TILEOP_API_SPEC_VERSION` identify the API/spec contract, while
`PTO_TILEOP_API_REVISION` records the exact TileOP-API Git commit used by
`make install`. Feature macros such as `PTO_TILEOP_API_HAS_LOCAL_B_KN_FIX`
allow consumers to reject toolchains that predate a required fix.

The normative operation inventory comes from the checked-in PTO catalog at
[`contracts/pto-isa-0.59.0-tile-operations.json`](contracts/pto-isa-0.59.0-tile-operations.json).
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
- [PTO ISA 0.59.0 CUBE layout operations](docs/tileop-usage/layout-and-rearrangement/layout/TPERMUTE.md)
- [Comparison operations](docs/tileop-usage/elementwise-tile-tile/logical/TCMP.md)
- [TEXPDIF](docs/tileop-usage/elementwise-tile-tile/transcendental/TEXPDIF.md)
- [Sorting operations](docs/tileop-usage/irregular-and-complex/sorting/TSORT.md)
- [Fixed-point matrix wrappers](docs/tileop-usage/options.md)

## Validation

```sh
make check
```

Target compilation requires a Linx LLVM build supporting the PTO ISA 0.59.0
encoding ABI. The old PTO ISA 0.58.3 Linx LLVM requirement belongs only to the
historical migration fixtures under `test/tileop_api` and
`docs/tileop-usage/migration/pto-0583-migration.md`; it is not the current API
baseline. Run the catalog and documentation checks before target compilation.

To refresh the pinned projection, provide the clean, exact reviewed LinxISA
v0.58 authority commit together with a clean checkout of `PTO-ISA/pto-spec` at
the `v0.59.0.0` release commit:

```sh
python3 tools/sync_pto_isa_0590_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json \
  --pto-root /path/to/pto-spec
python3 tools/generate_engine_docs.py
make check
```
