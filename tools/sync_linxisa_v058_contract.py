#!/usr/bin/env python3
"""Project the released PTO ISA v0.58.1 Tile engine catalog into this API repo."""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
EXPECTED_COUNTS = {"CUBE": 12, "SFU": 56, "TLSU": 10, "VEC": 31}
EXPECTED_COMMIT = "c381465b2b8e457e162a4246ee58bb9a2c5b49fd"
EXPECTED_TREE = "463a19db3d6ba70022f18bdbca0d4b2c6ed586e4"
EXPECTED_CATALOG_SHA256 = "f163dea8be281fd67173713d373b60f95a9c3c4e558adcdf8034cc213507a1a3"
EXPECTED_RELEASE = "v0.58.1"


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return result.stdout.strip()


def build_projection(source: Path) -> dict[str, object]:
    source = source.resolve()
    repo = Path(git(source.parent, "rev-parse", "--show-toplevel"))
    if git(repo, "status", "--porcelain=v1", "--untracked-files=all"):
        raise SystemExit(f"refusing dirty PTO ISA source tree: {repo}")
    if git(repo, "describe", "--exact-match", "--tags", "HEAD") != EXPECTED_RELEASE:
        raise SystemExit(f"PTO ISA source HEAD is not the exact {EXPECTED_RELEASE} tag")
    commit = git(repo, "rev-parse", "HEAD")
    tree = git(repo, "rev-parse", "HEAD^{tree}")
    source_sha256 = hashlib.sha256(source.read_bytes()).hexdigest()
    if (commit, tree, source_sha256) != (EXPECTED_COMMIT, EXPECTED_TREE, EXPECTED_CATALOG_SHA256):
        raise SystemExit("PTO ISA v0.58.1 source identity does not match the reviewed release")

    data = json.loads(source.read_text(encoding="utf-8"))
    if data.get("isa") != "PTO Instruction Set Architecture" or data.get("operation_count") != 109:
        raise SystemExit("source is not the PTO ISA 0.58.1 Tile operation catalog")
    operations = data.get("operations")
    if not isinstance(operations, list):
        raise SystemExit("PTO ISA Tile operation catalog has no operations list")
    engine_counts = dict(sorted(Counter(row["engine"] for row in operations).items()))
    if engine_counts != EXPECTED_COUNTS:
        raise SystemExit("unexpected PTO ISA v0.58.1 engine counts")

    tepl_ops = [row for row in operations if row["family"] == "TEPL"]
    tlsu_ops = [row for row in operations if row["family"] == "TLSU"]
    cube_ops = [row for row in operations if row["family"] == "CUBE"]
    if (len(tepl_ops), len(tlsu_ops), len(cube_ops)) != (87, 10, 12):
        raise SystemExit("unexpected PTO ISA Tile carrier counts")

    relative = source.relative_to(repo).as_posix()
    return {
        "schema": "linx-tileop-api.engine-ops-projection.v1",
        "profile": "v0.58",
        "source": {
            "repo": "https://github.com/PTO-ISA/pto-spec",
            "release": EXPECTED_RELEASE,
            "commit": commit,
            "tree": tree,
            "path": relative,
            "sha256": source_sha256,
        },
        "semantic_engine_counts": engine_counts,
        "tepl_carrier": {
            "selector_formula": "(mode << 5) | function",
            "accepted_selector_count": len(tepl_ops),
            "canonical_aliases": ["BSTART.VEC", "BSTART.SFU"],
            "compatibility_spelling": "BSTART.TEPL",
        },
        "tepl_ops": [
            {
                "name": row["name"],
                "logical_selector": int(row["selector"], 0),
                "mode": row["mode"],
                "function": row["function"],
                "engine": row["engine"],
                "classification": row["classification"],
            }
            for row in tepl_ops
        ],
        "tlsu_ops": [
            {
                "name": row["name"],
                "function": row["function"],
                "mnemonic": row["command_mnemonic"],
                "engine": row["engine"],
            }
            for row in tlsu_ops
        ],
        "cube_ops": [
            {
                "name": row["name"],
                "function": row["function"],
                "mnemonic": row["command_mnemonic"],
                "engine": row["engine"],
            }
            for row in cube_ops
        ],
        "deleted_tile_names": data["deleted_names"],
    }


def render(projection: dict[str, object]) -> str:
    return json.dumps(projection, indent=2, sort_keys=True) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "source",
        type=Path,
        help="released PTO ISA spec/catalog/tile-operations.json",
    )
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    expected = render(build_projection(args.source))
    if args.check:
        if not args.output.exists() or args.output.read_text(encoding="utf-8") != expected:
            raise SystemExit(f"stale LinxISA v0.58 projection: {args.output}")
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(expected, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
