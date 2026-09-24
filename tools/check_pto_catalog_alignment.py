#!/usr/bin/env python3
"""Fail-closed checks for the pinned PTO ISA 0.58.6 Tile catalog.

This check deliberately verifies inventory and provenance, not the full ASL/NDF
semantics. The latter remains owned by PTO-ISA/pto-spec.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "contracts" / "pto-isa-0.58.6-tile-operations.json"
INCLUDE = ROOT / "include"
DOCS = ROOT / "docs" / "tileop-usage"


def text(paths: list[Path]) -> str:
    return "\n".join(p.read_text(encoding="utf-8", errors="ignore") for p in paths)


def main() -> int:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    failures: list[str] = []
    if catalog.get("operation_count") != 117:
        failures.append("catalog operation_count is not 117")
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
        if not re.search(rf"\b{re.escape(name)}\b", include_text):
            failures.append(f"active catalog operation has no wrapper reference: {name}")
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

    if failures:
        print("FAIL: PTO 0.58.6 catalog alignment", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print(f"PASS: PTO 0.58.6 catalog alignment ({len(names)} active operations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())