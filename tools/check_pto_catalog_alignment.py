#!/usr/bin/env python3
"""Fail-closed checks for the pinned PTO ISA 0.59.0 Tile catalog.

Verifies source provenance, inventory and the indexed scalar-operand contract.
Full ASL execution and hardware conformance remain separate validation.
"""

from __future__ import annotations

import argparse
import subprocess
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "contracts" / "pto-isa-0.59.0-tile-operations.json"
INCLUDE = ROOT / "include"
DOCS = ROOT / "docs" / "tileop-usage"


def text(paths: list[Path]) -> str:
    return "\n".join(p.read_text(encoding="utf-8", errors="ignore") for p in paths)


def source_pins(lock: dict):
    return [(kind, lock[kind]) for kind in ("release", "active")] + [
        ("extension:" + name, pin) for name, pin in lock.get("extensions", {}).items()]


def verify_sources(lock: dict, spec_repo: Path | None = None,
                   spec_ref: str | None = None) -> list[str]:
    failures = []
    for kind, pin in source_pins(lock):
        path = ROOT / "contracts" / pin["catalog"]
        if hashlib.sha256(path.read_bytes()).hexdigest() != pin["catalog_sha256"]:
            failures.append(f"{kind} catalog does not match its pinned normative snapshot")
    if spec_ref and not spec_repo:
        failures.append("--spec-ref requires --spec-repo")
    if spec_repo:
        def git(*args):
            return subprocess.check_output(["git", "-C", str(spec_repo), *args],
                                           stderr=subprocess.PIPE)
        try:
            active = lock["active"]
            ref = spec_ref or active["commit"]
            if git("rev-parse", ref + "^{commit}").decode().strip() != active["commit"]:
                failures.append("spec ref differs from the reviewed main commit; review and repin required")
            for kind, pin in source_pins(lock):
                data = git("show", pin["commit"] + ":spec/catalog/tile-operations.json")
                if hashlib.sha256(data).hexdigest() != pin["catalog_sha256"]:
                    failures.append(f"{kind} source Git catalog differs from lock")
            for name, pin in lock.get("extensions", {}).items():
                for path, tree in pin["normative_trees"].items():
                    if git("rev-parse", pin["commit"] + ":" + path).decode().strip() != tree:
                        failures.append(f"extension normative tree differs: {name}/{path}")
            for path, tree in active["normative_trees"].items():
                if git("rev-parse", ref + ":" + path).decode().strip() != tree:
                    failures.append(f"spec normative tree differs: {path}")
        except subprocess.CalledProcessError as error:
            failures.append("cannot read pinned spec Git objects: " + error.stderr.decode().strip())
    return failures


def verify_indexed_contract(header: str) -> list[str]:
    failures = []
    # Inspect each explicit indexed asm block and both atomic macro emitters.
    # Main emits base only; explicitly pinned #424 overloads emit Base/Min/Max.
    blocks = re.findall(r'"BSTART\.TLSU M(?:GATHER|SCATTER)[^;]+;', header)
    blocks += re.findall(r'^#define PTO_DEFINE_M(?:GATHER|SCATTER)(?:_RANGE)?_ATOM\(.*?'
                         r'(?=^PTO_DEFINE_|^#undef)', header, re.M | re.S)
    if not blocks:
        return ["no indexed TLSU emitters found"]
    for block in blocks:
        records = re.findall(r'B\.IOR \[((?:%\[[^]]+\]|[^]])*)\],\s*\[([^]]*)\]', block)
        bounded = "[RangeMin]" in block
        expected = re.compile(r"%\[[Bb]ase\]" +
                              (r",\s*%\[RangeMin\],\s*%\[RangeMax\]" if bounded else ""))
        if not records or any(not expected.fullmatch(source.strip()) or destination.strip()
                              for source, destination in records):
            failures.append("indexed TLSU B.IOR must use main base-only or pinned #424 Base/Min/Max schema")
        if bounded and 'PTO_INDEXED_RANGE_CONSTRAINT(range.min)' not in block:
            failures.append("#424 bounds must preserve explicit descriptor presence")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--spec-repo", type=Path, help="verify pinned source Git objects offline")
    parser.add_argument("--spec-ref", help="also require this ref to equal the reviewed main commit")
    args = parser.parse_args()
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    lock = json.loads((ROOT / "contracts/pto-isa-0.59.0-sources.json").read_text())
    failures = verify_sources(lock, args.spec_repo, args.spec_ref)
    failures += verify_indexed_contract((INCLUDE / "jcore/template_asm.hpp").read_text())
    if catalog.get("operation_count") != 119:
        failures.append("catalog operation_count is not 119")
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

    # 0.59.0 changed the gather/scatter index interpretation and the expansion
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
        print("FAIL: PTO 0.59.0 catalog alignment", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1
    print(f"PASS: PTO 0.59.0 catalog alignment ({len(names)} active operations; #424 separately pinned)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
