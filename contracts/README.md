# LinxISA contract projections

`pto-isa-0.59.0-tile-operations.json` is the checked-in machine-readable snapshot
of the published PTO ISA 0.59.0.0 release Tile catalog. It records all 119 accepted direct
operations plus the catalog's deleted and rejected inventories. Its source of
truth is `PTO-ISA/pto-spec/spec/catalog/tile-operations.json`; refresh it only
from the `v0.59.0.0` release commit `b0165320a4b575b18d2f4d93faf40178f661ab49`
and review the resulting diff. These bytes are identical to the PTO 0.58.7
development golden catalog (SHA-256 533db9b8...); only the surrounding encoding
identity moved to the release.

`linxisa-v0.58-engine-ops.json` is a generated, provenance-pinned projection of
the LinxISA v0.58 authority onto the PTO ISA 0.59.0.0 release encoding identity.
It records operation ownership and encodings; it does not claim that this
repository implements every projected operation. There is no LinxISA 0.59
profile, so the LinxISA `source` stays v0.58 and the development-golden lock it
carried is preserved under `pto_source.development_lock`:

```bash
python3 tools/sync_pto_isa_0590_contract.py \
  /path/to/linx-isa/isa/v0.58/linxisa-v0.58.json \
  --pto-root /path/to/pto-spec
```

The catalog drives current operation inventory and documentation checks. ASL/NDF
still owns legality, fault, completion, rollback, definedness, and memory-order
semantics; this wrapper repository does not duplicate those formal semantics.
