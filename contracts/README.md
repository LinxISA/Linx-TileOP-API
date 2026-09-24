# LinxISA contract projections

`pto-isa-0.58.6-tile-operations.json` is the checked-in machine-readable snapshot
of the PTO ISA 0.58.6 active Tile catalog. It records all 117 accepted direct
operations plus the catalog's deleted and rejected inventories. Its source of
truth is `PTO-ISA/pto-spec/spec/catalog/tile-operations.json`; refresh it only
from the pinned PTO release and review the resulting diff.

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
