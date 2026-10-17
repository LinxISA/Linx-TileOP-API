# LinxISA contract projections

`pto-isa-0.58.7-tile-operations.json` is the checked-in machine-readable snapshot
of the reviewed PTO ISA 0.58.7 development Tile catalog. It records all 119 accepted direct
operations plus the catalog's deleted and rejected inventories. Its source of
truth is `PTO-ISA/pto-spec/spec/catalog/tile-operations.json`; refresh it only
from commit `f0a876e449a95d26841c5ee7affb4c905ebc5f5b` and review the resulting diff.

`linxisa-v0.58-engine-ops.json` is a generated, provenance-pinned LinxISA/PTO
0.58.7 development projection. It records operation ownership and encodings;
it does not claim that this repository implements every projected operation:

```bash
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json \
  --pto-root /path/to/pto-spec
```

The catalog drives current operation inventory and documentation checks. ASL/NDF
still owns legality, fault, completion, rollback, definedness, and memory-order
semantics; this wrapper repository does not duplicate those formal semantics.
