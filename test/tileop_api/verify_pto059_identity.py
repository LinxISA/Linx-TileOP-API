#!/usr/bin/env python3
"""Validate an ELF against the pinned PTO 0.59.0 release identity."""
import json
from pathlib import Path
import sys
import verify_pto_identity as legacy

ROOT = Path(__file__).resolve().parents[2]
release = json.loads((ROOT / "contracts/pto-isa-0.59.0-sources.json").read_text())["release"]
EXPECTED_DESC = json.dumps({key: release[key] for key in
    ("release", "encoding_abi", "encoding_projection_sha256")},
    sort_keys=True, separators=(",", ":")).encode()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_pto059_identity.py image.elf")
    try:
        legacy.validate(Path(sys.argv[1]), EXPECTED_DESC)
    except (OSError, legacy.IdentityError, legacy.struct.error) as error:
        raise SystemExit(f"FAIL PTO 0.59.0 identity: {error}")
    print("PASS: PTO 0.59.0 ELF identity")
