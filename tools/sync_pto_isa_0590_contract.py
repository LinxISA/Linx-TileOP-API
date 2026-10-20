#!/usr/bin/env python3
"""Project the reviewed LinxISA v0.58 authority and the published PTO ISA
0.59.0.0 release into this API.

Provenance model
----------------
* The LinxISA v0.58 authority is unchanged: it still owns the semantic engine
  projection (VEC/SFU/TLSU/CUBE operation inventory and selectors) and it
  locked the PTO *development* golden through ``pto-scalar-proposal.lock.json``.
* The active PTO *encoding identity* (ABI, encoding-projection SHA-256 and ELF
  ``release`` field) is the published PTO ISA ``v0.59.0.0`` release, read from a
  clean ``spec/release-manifest.json``.
* There is no LinxISA ``0.59`` profile, so this tool does not fabricate one:
  the LinxISA ``source`` stays ``v0.58`` and the development-golden lock is
  recorded verbatim.  The 119-operation Tile catalog is byte-identical between
  the development golden and the 0.59.0.0 release (both SHA-256 533db9b8...),
  which is what makes the v0.58 engine projection valid against the release.

The release manifest records ``specification_status: draft``; this tool carries
that status faithfully rather than claiming a finalized specification.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "contracts" / "linxisa-v0.58-engine-ops.json"
DEFAULT_PTO_OUTPUT = ROOT / "contracts" / "pto-isa-0.59.0-tile-operations.json"
EXPECTED_COUNTS = {"CUBE": 12, "SFU": 47, "TLSU": 28, "VEC": 32}

# LinxISA v0.58 authority (unchanged).
EXPECTED_COMMIT = "f77a8960f61f97c264a90b7aee757f99891b0943"
EXPECTED_TREE = "da085d3f000a5b92031c9694024dc9a7f3cdaa89"
EXPECTED_CATALOG_SHA256 = "51ca9be275518d29ec48bed570b7a9bd441a65b9c5e6d4f79072f650ee732925"

# PTO development golden that the LinxISA v0.58 authority locked.
EXPECTED_DEV_COMMIT = "f0a876e449a95d26841c5ee7affb4c905ebc5f5b"
EXPECTED_DEV_TREE = "fe9e8e050375fbce3f526d08acca13c37cd15830"
EXPECTED_DEV_MANIFEST_SHA256 = "0c6d0b2a1ec0b49e32e13971e751a5ea19946351a3880bfdfcef0f50351944f7"

# Published PTO ISA 0.59.0.0 release (the active encoding identity).
EXPECTED_PTO_COMMIT = "b0165320a4b575b18d2f4d93faf40178f661ab49"
EXPECTED_PTO_TREE = "31ff0c2d39cbed714baf5be7929bfd06887be9fb"
EXPECTED_PTO_RELEASE_MANIFEST_SHA256 = (
    "3c6b20623e73822c0bd26021c10beb233b8809328b5515f57609eadad1749dc4"
)
EXPECTED_PTO_TILE_SHA256 = "533db9b8f31f118af5978de813cfaa3fa738de820ee24655faa2ee71f343a706"
EXPECTED_RELEASE_IDENTITY = {
    "release": "0.59.0",
    "publication_version": "0.59.0.0",
    "encoding_abi": "pto-isa-0.59.0-mode-function-v1",
    "encoding_projection_sha256": (
        "91509080ca656eb124f47830dcb09c50389dc177266b3b8df9d2d11dc37d5697"
    ),
    "content_sha256": (
        "a3e71975e7f753864f7ec09c57515e60f5a6e3e72dfdbce8e444c815a20b2835"
    ),
}


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(repo), *args],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    return result.stdout.strip()


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require_clean_exact_repo(repo: Path, commit: str, tree: str, label: str) -> None:
    if git(repo, "status", "--porcelain=v1", "--untracked-files=all"):
        raise SystemExit(f"refusing dirty {label} source tree: {repo}")
    actual = (git(repo, "rev-parse", "HEAD"), git(repo, "rev-parse", "HEAD^{tree}"))
    if actual != (commit, tree):
        raise SystemExit(f"{label} source identity does not match the reviewed pin")


def load_sources(
    source: Path, pto_root: Path
) -> tuple[dict[str, object], dict[str, object], Path]:
    source = source.resolve()
    linx_repo = Path(git(source.parent, "rev-parse", "--show-toplevel"))
    require_clean_exact_repo(linx_repo, EXPECTED_COMMIT, EXPECTED_TREE, "LinxISA v0.58.7")
    if sha256(source) != EXPECTED_CATALOG_SHA256:
        raise SystemExit("LinxISA catalog bytes do not match the reviewed authority")

    pto_repo = Path(git(pto_root.resolve(), "rev-parse", "--show-toplevel"))
    require_clean_exact_repo(
        pto_repo, EXPECTED_PTO_COMMIT, EXPECTED_PTO_TREE, "PTO ISA 0.59.0.0 release"
    )
    manifest_path = pto_repo / "spec" / "release-manifest.json"
    tile_catalog_path = pto_repo / "spec" / "catalog" / "tile-operations.json"
    if sha256(manifest_path) != EXPECTED_PTO_RELEASE_MANIFEST_SHA256:
        raise SystemExit("PTO release manifest bytes do not match the reviewed release")
    if sha256(tile_catalog_path) != EXPECTED_PTO_TILE_SHA256:
        raise SystemExit("PTO Tile catalog bytes do not match the reviewed release")

    linx_data = json.loads(source.read_text(encoding="utf-8"))
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    tile_catalog = json.loads(tile_catalog_path.read_text(encoding="utf-8"))
    if linx_data.get("isa") != "LinxISA" or linx_data.get("version") != "0.58.7":
        raise SystemExit("source is not the reviewed LinxISA 0.58.7 catalog")

    release_identity = {
        "release": manifest.get("release"),
        "publication_version": manifest.get("publication_version"),
        "encoding_abi": manifest.get("encoding_abi"),
        "encoding_projection_sha256": manifest.get("encoding_projection_sha256"),
        "content_sha256": manifest.get("content_sha256"),
    }
    if release_identity != EXPECTED_RELEASE_IDENTITY:
        raise SystemExit("unexpected PTO 0.59.0.0 release identity")
    counts = manifest.get("catalog_counts", {}).get("tile_operations_total")
    if counts != 119:
        raise SystemExit("unexpected PTO release Tile operation count")
    if tile_catalog.get("operation_count") != 119 or len(tile_catalog.get("operations", [])) != 119:
        raise SystemExit("unexpected PTO Tile catalog inventory")

    # The LinxISA v0.58 authority locked the PTO development golden. Keep that
    # lock fail-closed and additionally require its Tile catalog bytes to equal
    # the release Tile catalog, which is what makes this projection valid.
    lock_path = source.parent / "pto-scalar-proposal.lock.json"
    lock = json.loads(lock_path.read_text(encoding="utf-8"))
    lock_identity = (
        lock.get("status"),
        lock.get("source", {}).get("commit"),
        lock.get("source", {}).get("tree"),
        lock.get("development_manifest", {}).get("sha256"),
        lock.get("catalogs", {}).get("tile_operations", {}).get("sha256"),
        lock.get("catalogs", {}).get("tile_operations", {}).get("count"),
    )
    expected_lock_identity = (
        "development-not-published",
        EXPECTED_DEV_COMMIT,
        EXPECTED_DEV_TREE,
        EXPECTED_DEV_MANIFEST_SHA256,
        EXPECTED_PTO_TILE_SHA256,
        119,
    )
    if lock_identity != expected_lock_identity:
        raise SystemExit("LinxISA authority does not carry the reviewed PTO development lock")
    return linx_data, manifest, tile_catalog_path


def build_projection(source: Path, pto_root: Path) -> tuple[dict[str, object], bytes]:
    linx_data, manifest, tile_catalog_path = load_sources(source, pto_root)
    engine_ops = linx_data["state"]["engine_ops"]
    if engine_ops["semantic_engine_counts"] != EXPECTED_COUNTS:
        raise SystemExit("unexpected LinxISA v0.58.7 engine counts")
    if sum(EXPECTED_COUNTS.values()) != 119:
        raise SystemExit("engine counts do not cover the PTO release Tile catalog")

    linx_repo = Path(git(source.resolve().parent, "rev-parse", "--show-toplevel"))
    relative = source.resolve().relative_to(linx_repo).as_posix()
    projection = {
        "schema": "linx-tileop-api.engine-ops-projection.v2",
        "profile": "v0.58",
        "source": {
            "repo": "https://github.com/LinxISA/linx-isa",
            "release": "0.58.7",
            "commit": EXPECTED_COMMIT,
            "tree": EXPECTED_TREE,
            "path": relative,
            "sha256": EXPECTED_CATALOG_SHA256,
        },
        "pto_source": {
            "repo": "https://github.com/PTO-ISA/pto-spec.git",
            "release": EXPECTED_RELEASE_IDENTITY["release"],
            "publication_version": EXPECTED_RELEASE_IDENTITY["publication_version"],
            "release_tag": "v0.59.0.0",
            "specification_status": manifest["specification_status"],
            "commit": EXPECTED_PTO_COMMIT,
            "tree": EXPECTED_PTO_TREE,
            "release_manifest": {
                "path": "spec/release-manifest.json",
                "sha256": EXPECTED_PTO_RELEASE_MANIFEST_SHA256,
            },
            "encoding_abi": EXPECTED_RELEASE_IDENTITY["encoding_abi"],
            "encoding_projection_sha256": EXPECTED_RELEASE_IDENTITY[
                "encoding_projection_sha256"
            ],
            "content_sha256": EXPECTED_RELEASE_IDENTITY["content_sha256"],
            "tile_catalog": {
                "path": "spec/catalog/tile-operations.json",
                "sha256": EXPECTED_PTO_TILE_SHA256,
                "count": 119,
            },
            "development_lock": {
                "release": "0.58.7",
                "commit": EXPECTED_DEV_COMMIT,
                "tree": EXPECTED_DEV_TREE,
                "development_manifest": {
                    "path": "spec/development-manifest.json",
                    "sha256": EXPECTED_DEV_MANIFEST_SHA256,
                },
                "catalog_sha256_matches_release": True,
                "note": (
                    "The LinxISA v0.58 authority locked the PTO development "
                    "golden; its Tile catalog bytes equal the 0.59.0.0 release "
                    "Tile catalog (both SHA-256 533db9b8...), so the v0.58 "
                    "engine projection remains valid against the published "
                    "release."
                ),
            },
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
        "deleted_tile_names": linx_data["retired_encodings"]["deleted_tile_names"],
    }
    return projection, tile_catalog_path.read_bytes()


def render(projection: dict[str, object]) -> str:
    return json.dumps(projection, indent=2, sort_keys=True) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="reviewed isa/v0.58/linxisa-v0.58.json")
    parser.add_argument(
        "--pto-root",
        type=Path,
        required=True,
        help="clean checkout of PTO-ISA/pto-spec at the v0.59.0.0 release commit",
    )
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--pto-output", type=Path, default=DEFAULT_PTO_OUTPUT)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    projection, tile_catalog = build_projection(args.source, args.pto_root)
    expected = render(projection)
    if args.check:
        if not args.output.exists() or args.output.read_text(encoding="utf-8") != expected:
            raise SystemExit(f"stale LinxISA v0.58 projection: {args.output}")
        if not args.pto_output.exists() or args.pto_output.read_bytes() != tile_catalog:
            raise SystemExit(f"stale PTO 0.59.0.0 release Tile catalog: {args.pto_output}")
        return 0

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(expected, encoding="utf-8")
    args.pto_output.parent.mkdir(parents=True, exist_ok=True)
    args.pto_output.write_bytes(tile_catalog)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
