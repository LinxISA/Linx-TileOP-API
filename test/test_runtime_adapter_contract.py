#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ADAPTER = ROOT / "test/common/gfrun_runtime_adapter.c"
MAKEFILE = ROOT / "test/common/Makefile.common"


def significant_lines(text: str) -> list[str]:
    return [
        line.strip().lower()
        for line in text.splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]


def main() -> None:
    source = ADAPTER.read_text()
    lines = significant_lines(source)
    acrc_index = next(i for i, line in enumerate(lines) if '"acrc 0\\n"' in line)

    preceding_start = next(
        line for line in reversed(lines[:acrc_index]) if "bstart." in line
    )
    if "bstart.sys" not in preceding_start:
        raise SystemExit(f"ACRC is not in a SYS block: {preceding_start}")
    if '"bstop\\n"' not in lines[acrc_index + 1]:
        raise SystemExit("ACRC must be the terminal scalar operation")
    required_contract = {
        "__linx_group_worker_start": "compiler-rt group worker entry",
        "__linx_group_pe_main": "per-PE main callback",
        "return main();": "benchmark result propagation",
        ".note.ssm.pto": "explicit hosted ABI admission note",
        "0x50544f01u": "hosted completion service tag",
        '"a0", "a1", "x1", "memory"': "completion register contract",
    }
    for token, description in required_contract.items():
        if token not in source:
            raise SystemExit(f"runtime adapter lacks {description}: {token}")

    makefile = MAKEFILE.read_text()
    if "LINX_RUNTIME_ADAPTER := $(ROOT)/test/common/gfrun_runtime_adapter.c" not in makefile:
        raise SystemExit("Linx fixtures do not select the runtime adapter")
    if "COMM_SRC_FILE += $(LINX_RUNTIME_ADAPTER)" not in makefile:
        raise SystemExit("runtime adapter is not linked by default")

    print("PASS: benchmark runtime adapter uses the SYS/ACRC completion path")


if __name__ == "__main__":
    main()
