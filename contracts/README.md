# LinxISA contract projections

`linxisa-v0.58-engine-ops.json` is a generated, provenance-pinned projection of
the released LinxISA v0.58 machine catalog. It is not an independent ISA
definition. Refresh it only from an exact, clean `v0.58` checkout:

```bash
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
```

The projection drives repository checks for canonical VEC/SFU assembly,
retired operation removal, TLSU naming, and the four semantic Engine classes.
