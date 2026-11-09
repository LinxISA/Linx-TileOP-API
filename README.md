# Linx TileOP API

Header-only C++ TileOP bindings for the LinxISA / PTO ISA 0.59.0 architectural contract.
The checked-in catalog and documentation are generated against the PTO ISA 0.59.0
baseline (`0.59.0.0`, ABI `pto-isa-0.59.0-mode-function-v1`).

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

- [0.59 alignment and validation](docs/tileop-usage/migration/pto-0590-alignment.md)
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

The active 119-operation catalog is pinned to PTO-SPEC `main` at `42067d0f2890`.
Optional `IndexedByteRange` APIs retain the unmerged [PTO-SPEC #424](https://github.com/PTO-ISA/pto-spec/pull/424)
extension at `1981bceffa82`. Both normative snapshots and their hashes are
recorded separately in [the source lock](contracts/pto-isa-0.59.0-sources.json).
The immutable v0.59.0.0 release catalog is retained for ABI identity tests.
Range APIs require LLVM `b19e8c0e184c` support; API availability does not imply
complete runtime or numeric conformance. See [support status](docs/tileop-usage/support-status.md).

```sh
python3 tools/sync_pto059_catalog.py /path/to/pto-spec
python3 tools/check_pto_catalog_alignment.py --spec-repo /path/to/pto-spec --spec-ref origin/main
python3 tools/generate_engine_docs.py
make check
```

Indexed TLSU 显式字节范围扩展见 [PR #424 使用说明](docs/tileop-usage/indexed-tlsu-range.md)。
