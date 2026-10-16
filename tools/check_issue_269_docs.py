#!/usr/bin/env python3
"""Guard the Issue #269 reduction-source allocation documentation."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
REDUCTION_DOCS = ROOT / "docs" / "tileop-usage" / "reduce-and-expand"
EXPECTED_DOCS = tuple(
    REDUCTION_DOCS / axis / f"{prefix}{suffix}.md"
    for axis, prefix in (("row", "TROW"), ("column", "TCOL"))
    for suffix in ("SUM", "MAX", "MIN", "PROD", "ARGMAX", "ARGMIN")
)
OLD_LIMIT = "allocated capacity 不得超过 **2048 bytes**"
NEW_CONTRACT = (
    "源 Tile 的 allocated capacity 仅需满足通用 Tile allocation 规则；"
    "归约操作本身不额外施加 2048-byte 上限。"
)


def main() -> int:
    failures = []
    for path in EXPECTED_DOCS:
        if not path.is_file():
            failures.append(f"missing reduction page: {path.relative_to(ROOT)}")
            continue
        text = path.read_text(encoding="utf-8")
        if OLD_LIMIT in text:
            failures.append(f"obsolete 2048-byte reduction limit: {path.relative_to(ROOT)}")
        if NEW_CONTRACT not in text:
            failures.append(f"missing Issue #269 contract: {path.relative_to(ROOT)}")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    print(f"PASS: Issue #269 allocation contract in {len(EXPECTED_DOCS)} reduction pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
