# LinxISA contract projections

`linxisa-v0.58-engine-ops.json` is a generated, provenance-pinned **historical**
projection of the LinxISA/PTO ISA 0.58.3 machine catalog. It is not the current
PTO ISA 0.58.6 catalog and is not an independent ISA definition. Do not use it
to claim current 0.58.6 operation availability. Refresh it only from the exact,
clean authority commit encoded by the generator:

```bash
python3 tools/sync_linxisa_v058_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json
```

The projection drives legacy compatibility checks for VEC/SFU alias identity,
retired operation removal, TLSU naming, the four semantic engine classes, and
the exact historical PTO release/encoding provenance. The active 0.58.6
catalog must be obtained from the locked PTO release or `pto-spec/main`.
