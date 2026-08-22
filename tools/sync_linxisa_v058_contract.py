#!/usr/bin/env python3
"""Project the reviewed LinxISA/PTO 0.58.3 engine catalog into this API."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
EXPECTED_COUNTS = {"CUBE": 12, "SFU": 56, "TLSU": 10, "VEC": 31}
EXPECTED_COMMIT = "dd52a2e579d8058c0d8e33043e705122b340e73f"
EXPECTED_TREE = "1cfc7343e714489f95f67592475e8b9f079241ee"
EXPECTED_CATALOG_SHA256 = "34ecbcfa075166490b622647eb53c13a9c360848d6c7acb2e034d3e47f8c9a8a"
EXPECTED_PTO_COMMIT = "e599a3d36ebfad43362ff591ea5e128816c684c7"
EXPECTED_PTO_TREE = "abb6899d2e664e378ac9c1b77062670daa4d31b4"
EXPECTED_PTO_ENCODING_SHA256 = "8a48b80e04484c70870f155bf9efc79d2a805cf99e809f4e4e8a7e6a7eb34172"
EXPECTED_PTO_CONTENT_SHA256 = "f299fe3d256c5d071e57bb4aaa2be2de2e4a386ae090048df1f73ae92d392678"


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
    commit = git(repo, "rev-parse", "HEAD")
    tree = git(repo, "rev-parse", "HEAD^{tree}")
    source_sha256 = hashlib.sha256(source.read_bytes()).hexdigest()
    if (commit, tree, source_sha256) != (EXPECTED_COMMIT, EXPECTED_TREE, EXPECTED_CATALOG_SHA256):
        raise SystemExit("LinxISA v0.58 source identity does not match the reviewed release")

    data = json.loads(source.read_text(encoding="utf-8"))
    if data.get("isa") != "LinxISA" or data.get("version") != "0.58.3":
        raise SystemExit("source is not the LinxISA/PTO 0.58.3 canonical catalog")
    engine_ops = data["state"]["engine_ops"]
    if engine_ops["semantic_engine_counts"] != EXPECTED_COUNTS:
        raise SystemExit("unexpected LinxISA v0.58 engine counts")
    pto_lock_path = source.parent / "pto-spec.lock.json"
    pto_lock = json.loads(pto_lock_path.read_text(encoding="utf-8"))
    pto_identity = (
        pto_lock.get("release"),
        pto_lock.get("source", {}).get("commit"),
        pto_lock.get("source", {}).get("tree"),
        pto_lock.get("encoding_projection_sha256"),
        pto_lock.get("content_sha256"),
    )
    expected_pto_identity = (
        "0.58.3",
        EXPECTED_PTO_COMMIT,
        EXPECTED_PTO_TREE,
        EXPECTED_PTO_ENCODING_SHA256,
        EXPECTED_PTO_CONTENT_SHA256,
    )
    if pto_identity != expected_pto_identity:
        raise SystemExit("LinxISA authority does not carry the reviewed PTO 0.58.3 lock")

    relative = source.relative_to(repo).as_posix()
    return {
        "schema": "linx-tileop-api.engine-ops-projection.v1",
        "profile": "v0.58",
        "source": {
            "repo": "https://github.com/LinxISA/linx-isa",
            "release": "0.58.3",
            "commit": commit,
            "tree": tree,
            "path": relative,
            "sha256": source_sha256,
        },
        "pto_source": {
            "repo": pto_lock["source"]["repository"],
            "release": pto_lock["release"],
            "commit": pto_lock["source"]["commit"],
            "tree": pto_lock["source"]["tree"],
            "encoding_abi": pto_lock["encoding_abi"],
            "encoding_projection_sha256": pto_lock["encoding_projection_sha256"],
            "content_sha256": pto_lock["content_sha256"],
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
