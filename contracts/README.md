# LinxISA contract projections

`pto-isa-0.59.0-tile-operations.json` is the checked-in machine-readable snapshot
of the PTO ISA 0.59.0 active Tile catalog. It records all 119 accepted direct
operations plus the catalog's deleted and rejected inventories. Its source of
truth is `PTO-ISA/pto-spec/spec/catalog/tile-operations.json`; refresh it only
from the pinned PTO-SPEC main commit and review the resulting diff.

`linxisa-v0.58-engine-ops.json` remains a generated, provenance-pinned
**historical** LinxISA/PTO 0.58.3 projection. It is retained only for legacy
engine/compiler compatibility tests and must not be used to claim current
operation availability:

```bash
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
```

The catalog drives current operation inventory and documentation checks. ASL/NDF
still owns legality, fault, completion, rollback, definedness, and memory-order
semantics; this wrapper repository does not duplicate those formal semantics.

`pto-isa-0.59.0-sources.json` records three separate sources:

- `active`: reviewed main commit, catalog SHA-256 and complete normative ASL/spec trees.
- `release`: immutable v0.59.0.0 catalog and encoding ABI identity.
- `extensions.indexed_tlsu_range`: optional, unmerged #424 snapshot with its own catalog SHA-256 and ASL/spec trees.

`pto-isa-0.59.0-release-tile-operations.json` preserves the release catalog;
`pto-isa-0.59.0-extensions-tile-operations.json` preserves the extension catalog.
The main catalog does not certify range APIs: those require the separate #424
contract and matching LLVM. No extra operation names are added by the extension.

```sh
python3 tools/sync_pto059_catalog.py /path/to/pto-spec
python3 tools/check_pto_catalog_alignment.py --spec-repo /path/to/pto-spec --spec-ref origin/main
```

Source checks read pinned Git objects without switching the spec checkout.
An explicit ref must match the reviewed main commit; a moved ref requires review
and repinning. These checks do not execute ASL or certify hardware conformance.
