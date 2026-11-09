#!/usr/bin/env python3
"""Generate the active engine inventory from the pinned PTO main catalog.

Historical LinxISA projections remain inputs only to migration contract tests.
API subsets and backend limits are documented in support-status.md.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "contracts" / "pto-isa-0.59.0-tile-operations.json"
OUTPUT = ROOT / "docs" / "tileop-usage" / "generated" / "engines.md"


def render() -> str:
    contract = json.loads(CONTRACT.read_text(encoding="utf-8"))
    deleted_names = set(contract.get("deleted_names", []))
    operations = contract["operations"]
    contract["tepl_ops"] = [dict(op, logical_selector=int(op["selector"], 16))
                            for op in operations if op["family"] == "TEPL"]
    for family, key in (("TLSU", "tlsu_ops"), ("CUBE", "cube_ops")):
        contract[key] = [dict(op, mnemonic=op["command_mnemonic"] + " " + op["name"])
                         for op in operations if op["family"] == family]
    lines = [
        "# LinxISA / PTO ISA 0.59.0 wrapper engine compatibility view",
        "",
        "架构定义的引擎类别只有 **VEC**, **TLSU**, **CUBE**, and **SFU**。",
        "VEC 只包含逐元素操作；SFU 包含归约、广播、变换、排序以及其他需要更复杂硬件的操作。",
        "TEPL 仍是唯一的编译 carrier 标识。`BSTART.VEC` 和 `BSTART.SFU` 是特定引擎的汇编别名；",
        "inline wrapper 保留 `BSTART.TEPL`，以兼容之前的工具链源码。",
        "",
        "下表根据本仓库固定的 PTO-SPEC main (`42067d0f2890`) catalog 生成，数据记录在",
        "[`contracts/pto-isa-0.59.0-tile-operations.json`](../../../contracts/pto-isa-0.59.0-tile-operations.json) 中。",
        "",
    ]

    for engine in ("VEC", "SFU"):
        rows = [row for row in contract["tepl_ops"] if row["engine"] == engine]
        lines.extend(
            [
                f"## {engine}",
                "",
                "| API / 操作 | 规范汇编 | 逻辑 selector | 分类 |",
                "| --- | --- | ---: | --- |",
            ]
        )
        for row in rows:
            if row["name"] in deleted_names:
                continue
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
                "| 操作 | 规范 block 起始指令 | Function |",
                "| --- | --- | ---: |",
            ]
        )
        for row in contract[key]:
            if row["name"] in deleted_names:
                continue
            lines.append(f"| `{row['name']}` | `{row['mnemonic']}` | {row['function']} |")
        lines.append("")

    lines.extend(
        [
            "## 分类语义",
            "",
            "PTO ISA 0.59.0 将执行引擎与操作分类解耦。本表是 wrapper compatibility view；",
            "其操作清单固定在 42067d0f2890。`elementwise-tile-tile`",
            "和 `tile-scalar-and-immediate` 类别在 VEC 上执行逐元素操作，但其中的",
            "`TEXP`、`TLOG`、`TRECIP`、`TSQRT`、`TRSQRT` 由 SFU 执行。",
            "`reduce-and-expand`、`layout-and-rearrangement` 和 `irregular-and-complex` 类别由 SFU 执行。",
            "所有 TLSU 和 CUBE 操作均由对应的引擎类别执行。`BSTART.VEC` / `BSTART.SFU`",
            "拼写是唯一 `BSTART.TEPL` 编译 carrier 的规范别名（ADR 0057）。",
            "",
        ]
    )

    deleted = contract.get("deleted_names", [])
    if deleted:
        lines.extend(
            [
                "## 早期版本中移除的操作",
                "",
                "历史版本还提供了一些已从 PTO 0.59.0 active catalog 移除的 Tile 操作（例如 ACC 风格的后处理辅助操作）。",
                "本库不会把已退役的操作生成到当前目录；退役名称的规范列表记录在 contract 的 `deleted_names` 字段中。",
                "",
            ]
        )

    lines.extend(
        [
            "## 使用示例",
            "",
            "```cpp",
            "#include <common/pto_tileop.hpp>",
            "",
            "using namespace pto;",
            "using TileT = Tile<Location::Vec, float, 8, 32>;",
            "",
            "// 例如从 VEC 操作页面调用 C++ wrapper。",
            "void add(TileT &dst, TileT &lhs, TileT &rhs) {",
            "  TADD(dst, lhs, rhs);",
            "}",
            "```",
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
