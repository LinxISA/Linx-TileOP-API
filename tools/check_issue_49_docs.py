#!/usr/bin/env python3
"""Cheap regression checks for the Issue #49 documentation contracts."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs" / "tileop-usage"

def main() -> int:
    text = "\n".join(p.read_text(encoding="utf-8") for p in DOCS.rglob("*.md"))
    checks = [
        (not re.search(r"ByteOffsets\s*=.*uint16_t|global_tensor<uint16_t>[^\n]*offset", text),
         "offset examples must use uint32_t"),
        ("TSTORE_PART" in text and "TMOV_L2S_INSERT" in text,
         "Shared partial-store and Local-to-Shared contracts must be documented"),
        ("TMRGSORT" in text and "physical columns" in text,
         "TMRGSORT physical-shape contract must be documented"),
        ("TROWSUM" in text and "R x 1" in text and "TCOLSUM" in text and "1 x C" in text,
         "reduction output shapes must be documented"),
        (all(token in text for token in (
            "TMOV_L2S_INSERT", "TMOV_L2S_PUBLISH",
            "TMOV_S2L_BROADCAST", "TMOV_S2L_EXTRACT", "SharedTile<")),
         "all Shared TMOV variants and SharedTile construction must be documented"),
        ("void TLOAD_CUBE" in text and "void TSTORE_CUBE" in text and
         "(cube_tile, global_tensor)" in text and "(global_tensor, cube_tile)" in text,
         "explicit CUBE load/store signatures and argument order must be documented"),
        ("QF322S16Pre" in text and "FP32 accumulator" in text and "S32" in text,
         "pre-quantization accumulator compatibility must be documented"),
        ("source_tile" in text and "std::move(destinations)" in text and
         "ParentSizeCode=12" not in text.split("## Compile-time rejection", 1)[0],
         "TileArray and valid range examples must be directly usable"),
    ]
    failed = [message for passed, message in checks if not passed]
    if failed:
        for message in failed:
            print(f"FAIL: {message}")
        return 1
    print(f"PASS: {len(checks)} Issue #49 documentation regressions")
    return 0

if __name__ == "__main__":
    sys.exit(main())