# LinxISA contract projections

`linxisa-v0.58-engine-ops.json` is a generated, provenance-pinned projection of
the released PTO ISA v0.58.1 Tile-operation catalog used by the LinxISA v0.58
common subset. It is not an independent ISA definition. Refresh it only from
an exact, clean PTO ISA `v0.58.1` checkout:

```bash
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/pto-spec/spec/catalog/tile-operations.json
```

The projection drives repository checks for canonical VEC/SFU assembly,
retired operation removal, TLSU naming, and the four semantic Engine classes.
