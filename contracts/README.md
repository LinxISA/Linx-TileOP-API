# LinxISA contract projections

`linxisa-v0.58-engine-ops.json` is a generated, provenance-pinned projection of
the reviewed LinxISA/PTO ISA 0.58.3 machine catalog. It is not an independent
ISA definition. Refresh it only from the exact, clean authority commit encoded
by the generator:

```bash
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
```

The projection drives repository checks for VEC/SFU alias identity, retired
operation removal, TLSU naming, the four semantic engine classes, and the
exact PTO release/encoding provenance.
