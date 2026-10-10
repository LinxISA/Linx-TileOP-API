#!/usr/bin/env python3
"""Fail-closed checks for the pinned PTO ISA 0.58.7 Tile catalog.

This check deliberately verifies inventory and provenance, not the full ASL/NDF
semantics. The latter remains owned by PTO-ISA/pto-spec.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "contracts" / "pto-isa-0.58.7-tile-operations.json"
INCLUDE = ROOT / "include"
DOCS = ROOT / "docs" / "tileop-usage"


def text(paths: list[Path]) -> str:
    return "\n".join(p.read_text(encoding="utf-8", errors="ignore") for p in paths)


def main() -> int:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    failures: list[str] = []
    if catalog.get("operation_count") != 118:
        failures.append("catalog operation_count is not 118")
    operations = catalog.get("operations", [])
    if len(operations) != catalog.get("operation_count"):
        failures.append("catalog operation_count does not match operations length")
    names = [op.get("name") for op in operations]
    if len(names) != len(set(names)):
        failures.append("catalog contains duplicate operation names")
    if not isinstance(catalog.get("schema_version"), int):
        failures.append("tile catalog schema_version is missing or malformed")

    include_text = text([p for p in INCLUDE.rglob("*") if p.is_file()])
    docs_text = text([p for p in DOCS.rglob("*.md") if p.is_file()])
    for name in names:
        if not re.search(rf"\b{re.escape(name)}\b", include_text) and not re.search(
            rf"`{re.escape(name)}`[^\n]{{0,180}}尚无 wrapper", docs_text
        ):
            failures.append(
                f"active catalog operation has neither wrapper nor explicit unsupported marker: {name}"
            )
    for name in catalog.get("deleted_names", []):
        active = re.search(rf"\b{re.escape(name)}\s*\(", include_text)
        retirement = re.search(
            rf"{re.escape(name)}[^\n]{{0,180}}(?:retired|removed|历史|deleted)",
            include_text,
            re.IGNORECASE,
        )
        if active and not retirement:
            failures.append(f"deleted operation appears active without retirement marker: {name}")

    # Every current catalog operation must be either documented or explicitly
    # listed in the wrapper support-status page. This prevents silent drift.
    status_marker = "尚无 wrapper"
    if status_marker not in docs_text:
        failures.append("documentation has no unified wrapper support-status marker")

    # 0.58.7 changed the gather/scatter index interpretation and the expansion
    # operand roles. Keep these contract deltas visible until every wrapper has
    # been audited; names alone are not semantic coverage.
    #
    # TEXPDIF is intentionally checked by backend dimension rather than by the
    # generic "尚无 wrapper" marker.  The TileOP repository has a CPU simulator
    # implementation and a JCORE inline-asm wrapper, while LLVM compiler
    # lowering and AArch64/SME support are separate capabilities.  Treating
    # those capabilities as one boolean incorrectly reported the existing
    # wrapper as unsupported.
    if "TEXPDIF" in names:
        for marker in (
            "CPU simulator",
            "JCORE inline-asm",
            "LLVM compiler lowering",
            "AArch64/SME",
        ):
            if marker not in docs_text:
                failures.append(f"TEXPDIF support status is missing backend dimension: {marker}")

    if failures:
        print("FAIL: PTO 0.58.7 catalog alignment", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print(f"PASS: PTO 0.58.7 catalog alignment ({len(names)} active operations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())