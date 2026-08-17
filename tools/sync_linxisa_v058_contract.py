#!/usr/bin/env python3
"""Project the released LinxISA v0.58 engine catalog into this API repo."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
EXPECTED_COUNTS = {"CUBE": 12, "SFU": 52, "TLSU": 10, "VEC": 35}
EXPECTED_COMMIT = "0a12890427edc2179ed75ad26039cdcebc6b4486"
EXPECTED_TREE = "fef6c084b166f3fd85a1b3d1b72fc069e6050800"
EXPECTED_CATALOG_SHA256 = "b38864f4630be258ec62e5690d794463d0574443782c06b9a79d7d0a4362c61b"


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
        raise SystemExit(f"refusing dirty LinxISA source tree: {repo}")
    if git(repo, "describe", "--exact-match", "--tags", "HEAD") != "v0.58":
        raise SystemExit("LinxISA source HEAD is not the exact v0.58 tag")
    commit = git(repo, "rev-parse", "HEAD")
    tree = git(repo, "rev-parse", "HEAD^{tree}")
    source_sha256 = hashlib.sha256(source.read_bytes()).hexdigest()
    if (commit, tree, source_sha256) != (EXPECTED_COMMIT, EXPECTED_TREE, EXPECTED_CATALOG_SHA256):
        raise SystemExit("LinxISA v0.58 source identity does not match the reviewed release")

    data = json.loads(source.read_text(encoding="utf-8"))
    if data.get("isa") != "LinxISA" or data.get("version") != "0.58.0":
        raise SystemExit("source is not the LinxISA 0.58.0 canonical catalog")
    engine_ops = data["state"]["engine_ops"]
    if engine_ops["semantic_engine_counts"] != EXPECTED_COUNTS:
        raise SystemExit("unexpected LinxISA v0.58 engine counts")

    relative = source.relative_to(repo).as_posix()
    return {
        "schema": "linx-tileop-api.engine-ops-projection.v1",
        "profile": "v0.58",
        "source": {
            "repo": "https://github.com/LinxISA/linx-isa",
            "release": "v0.58",
            "commit": commit,
            "tree": tree,
            "path": relative,
            "sha256": source_sha256,
        },
        "semantic_engine_counts": engine_ops["semantic_engine_counts"],
        "tepl_carrier": {
            "selector_formula": engine_ops["tepl"]["selector_formula"],
            "accepted_selector_count": engine_ops["tepl"]["accepted_selector_count"],
            "canonical_aliases": ["BSTART.VEC", "BSTART.SFU"],
            "compatibility_spelling": "BSTART.TEPL",
        },
        "tepl_ops": [
            {
                "name": row["name"],
                "logical_selector": row["logical_selector"],
                "mode": row["mode"],
                "function": row["function"],
                "engine": row["engine"],
                "classification": row["classification"],
            }
            for row in engine_ops["tepl"]["ops"]
        ],
        "tlsu_ops": [
            {
                "name": row["name"],
                "function": row["function"],
                "mnemonic": row["mnemonic"],
                "engine": row["engine"],
            }
            for row in engine_ops["tlsu"]["legal_aliases"]
        ],
        "cube_ops": [
            {
                "name": row["name"],
                "function": row["function"],
                "mnemonic": row["mnemonic"],
                "engine": row["engine"],
            }
            for row in engine_ops["cube"]["legal_aliases"]
        ],
        "deleted_tile_names": data["retired_encodings"]["deleted_tile_names"],
    }


def render(projection: dict[str, object]) -> str:
    return json.dumps(projection, indent=2, sort_keys=True) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="released isa/v0.58/linxisa-v0.58.json")
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
