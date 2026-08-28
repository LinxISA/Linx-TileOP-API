#!/usr/bin/env python3
"""Generate the v0.58.3 engine index from the pinned LinxISA projection.

The output mirrors the PTO ISA 0.58.3 tile-operation catalog
(spec/catalog/tile-operations.json) for the four architectural engine
classes: **VEC**, **SFU**, **TLSU**, and **CUBE**.  Engine and
classification are decoupled (per ADR 0057): TEXP/TLOG/... are SFU-executed
elementwise operations, TGATHER/TSCATTER/TCONCAT are SFU-executed
layout/irregular operations, and TMOV is a TLSU-executed layout operation.
TEPL is the unique compiled carrier identity. `BSTART.VEC` / `BSTART.SFU` are
engine-specific assembly aliases; current inline wrappers retain TEPL so the
same source also compiles with the preceding 0.58 toolchain.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
OUTPUT = ROOT / "docs" / "tileop-usage" / "generated" / "engines.md"


def render() -> str:
    contract = json.loads(CONTRACT.read_text(encoding="utf-8"))
    lines = [
        "# LinxISA / PTO ISA v0.58.3 execution engines",
        "",
        "The architectural engine classes are exactly **VEC**, **TLSU**, **CUBE**, and **SFU**.",
        "VEC contains elementwise operations only. SFU contains reductions, broadcasts, transforms,",
        "sorting, and other operations that require more complex hardware. TEPL remains the unique",
        "compiled carrier identity. `BSTART.VEC` and `BSTART.SFU` are engine-specific assembly aliases;",
        "the inline wrappers retain `BSTART.TEPL` for source compatibility with the preceding toolchain.",
        "",
        "The table is projected from the pinned LinxISA authority recorded in",
        "[`contracts/linxisa-v0.58-engine-ops.json`](../../contracts/linxisa-v0.58-engine-ops.json).",
        "",
    ]

    for engine in ("VEC", "SFU"):
        rows = [row for row in contract["tepl_ops"] if row["engine"] == engine]
        lines.extend(
            [
                f"## {engine}",
                "",
                "| API / operation | Canonical assembly | Logical selector | Classification |",
                "| --- | --- | ---: | --- |",
            ]
        )
        for row in rows:
            lines.append(
                f"| `{row['name']}` | `BSTART.{engine} {row['name']}` | "
                f"{row['logical_selector']} | {row['classification']} |"
            )
        lines.append("")

    for engine, key in (("TLSU", "tlsu_ops"), ("CUBE", "cube_ops")):
        lines.extend(
            [
                f"## {engine}",
                "",
                "| Operation | Canonical block start | Function |",
                "| --- | --- | ---: |",
            ]
        )
        for row in contract[key]:
            lines.append(f"| `{row['name']}` | `{row['mnemonic']}` | {row['function']} |")
        lines.append("")

    lines.extend(
        [
            "## Classification semantics",
            "",
            "PTO ISA 0.58.3 decouples the execution engine from the operation",
            "classification (ADR 0057). Operations in the `elementwise-tile-tile`",
            "and `tile-scalar-and-immediate` classes run elementwise on VEC, but a",
            "few elementwise-class operations (`TEXP`, `TLOG`, `TRECIP`, `TSQRT`,",
            "`TRSQRT`) are executed by SFU. `reduce-and-expand`, `layout-and-",
            "rearrangement`, and `irregular-and-complex` classes are SFU-executed.",
            "All TLSU and CUBE operations are executed by their own engine class.",
            "The `BSTART.VEC` / `BSTART.SFU` spellings are canonical aliases of",
            "the unique `BSTART.TEPL` compiled carrier (ADR 0057).",
            "",
        ]
    )

    deleted = contract.get("deleted_tile_names", [])
    if deleted:
        lines.extend(
            [
                "## Removed from earlier versions",
                "",
                "Pre-0.58 versions additionally shipped several tile operations",
                "that the active catalog removed (for example the ACC-style",
                "post-processing helpers). None of the removed operations are",
                "emitted by this library; the canonical list of retired names is",
                "recorded in the contract under `deleted_tile_names`.",
                "",
            ]
        )

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = render()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != expected:
            raise SystemExit(f"stale generated documentation: {OUTPUT.relative_to(ROOT)}")
        return 0
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(expected, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
