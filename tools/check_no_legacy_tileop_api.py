#!/usr/bin/env python3
"""Reject legacy TileOP interfaces from active include/docs/test paths.

The gate is intentionally scope-aware:
- it scans active library headers under include/
- it scans active documentation under docs/tileop-usage/
- it scans only the currently active TileOP fixtures from test/tileop_api/compile.all

Legacy cpu_sim-only fixtures are not part of the active Linx target surface and
are therefore excluded from this gate.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPILE_ALL = ROOT / "test" / "tileop_api" / "compile.all"
ACTIVE_TEST_ROOT = ROOT / "test" / "tileop_api" / "src"

BANNED = (
    r"\bMATMULMXB\s*\(",
    r"\bMATMACCMXB\s*\(",
    r"\bMATMACCMX\s*\(",
    r"\bMATMULMX\s*\(",
    r"\bMATMACC\s*\(",
    r"\bMATMUL\s*\(",
    r"\bTCAST\s*\(",
    r"\bTReshape\s*\(",
    r"\bTCOPY\s*\(",
)


def active_test_files() -> list[Path]:
    text = COMPILE_ALL.read_text(encoding="utf-8")
    match = re.search(r"ACTIVE_FIXTURES=\((.*?)\)\s*\n", text, re.S)
    if not match:
        raise SystemExit("failed to locate ACTIVE_FIXTURES in compile.all")
    fixtures = re.findall(r"[A-Za-z0-9_]+", match.group(1))
    return [ACTIVE_TEST_ROOT / f"{name}.cpp" for name in fixtures]


def active_files() -> list[Path]:
    files: list[Path] = []
    for root in (ROOT / "include", ROOT / "docs" / "tileop-usage"):
        files.extend(path for path in root.rglob("*") if path.is_file())
    files.extend(path for path in active_test_files() if path.exists())
    return files


def main() -> int:
    failures: list[str] = []
    patterns = [re.compile(pattern) for pattern in BANNED]

    for path in active_files():
        text = path.read_text(encoding="utf-8", errors="ignore")
        for pattern in patterns:
            for match in pattern.finditer(text):
                line = text.count("\n", 0, match.start()) + 1
                failures.append(f"{path.relative_to(ROOT)}:{line}: {pattern.pattern}")

    if failures:
        print("FAIL: legacy TileOP interfaces detected in active paths:", file=sys.stderr)
        for item in failures:
            print(f"  {item}", file=sys.stderr)
        return 1

    print("PASS: no legacy TileOP interfaces detected in active paths")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())