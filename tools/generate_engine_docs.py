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
        "# LinxISA / PTO ISA v0.58.3 执行引擎",
        "",
        "架构定义的引擎类别只有 **VEC**、**TLSU**、**CUBE** 和 **SFU**。",
        "VEC 只包含逐元素操作；SFU 包含归约、广播、变换、排序以及其他需要更复杂硬件的操作。",
        "TEPL 仍是唯一的编译 carrier 标识。`BSTART.VEC` 和 `BSTART.SFU` 是特定引擎的汇编别名；",
        "inline wrapper 保留 `BSTART.TEPL`，以兼容之前的工具链源码。",
        "",
        "下表根据固定版本的 LinxISA 权威数据生成，数据记录在",
        "[`contracts/linxisa-v0.58-engine-ops.json`](../../contracts/linxisa-v0.58-engine-ops.json) 中。",
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
            lines.append(f"| `{row['name']}` | `{row['mnemonic']}` | {row['function']} |")
        lines.append("")

    lines.extend(
        [
            "## 分类语义",
            "",
            "PTO ISA 0.58.3（ADR 0057）将执行引擎与操作分类解耦。`elementwise-tile-tile`",
            "和 `tile-scalar-and-immediate` 类别在 VEC 上执行逐元素操作，但其中的",
            "`TEXP`、`TLOG`、`TRECIP`、`TSQRT`、`TRSQRT` 由 SFU 执行。",
            "`reduce-and-expand`、`layout-and-rearrangement` 和 `irregular-and-complex` 类别由 SFU 执行。",
            "所有 TLSU 和 CUBE 操作均由对应的引擎类别执行。`BSTART.VEC` / `BSTART.SFU`",
            "拼写是唯一 `BSTART.TEPL` 编译 carrier 的规范别名（ADR 0057）。",
            "",
            "## 使用要求",
            "",
            "优先根据操作页面中的 C++ API 调用 TileOP；本页仅用于按执行引擎查找操作。不要仅凭分类名称推断执行引擎，具体 engine 和支持的 Tile location 以操作页面为准。",
            "",
            "## 约束、默认值、异常和边界行为",
            "",
            "每个 engine 都有自己的 Tile、PE participation、容量和 bundle 约束。省略的 bundle 字段采用具体操作 contract 的默认值；非法 engine、binder、Tile location、mask 或属性组合会在执行前被拒绝。操作的边界、padding、异常和结果语义不由本索引页定义。",
            "",
            "## 使用示例",
            "",
            "```cpp",
            "// 例如从 VEC 操作页面调用 C++ wrapper。",
            "TADD(dst, lhs, rhs);",
            "```",
            "",
            "## 完整语义",
            "",
            "各操作的完整语义请参阅其操作页面末尾的 PTO-SPEC 链接，规范总目录见 [PTO-SPEC tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。",
            "",
        ]
    )

    deleted = contract.get("deleted_tile_names", [])
    if deleted:
        lines.extend(
            [
                "## 早期版本中移除的操作",
                "",
                "0.58 之前的版本还提供了一些已从当前目录移除的 Tile 操作（例如 ACC 风格的后处理辅助操作）。",
                "本库不会生成任何已移除的操作；废弃名称的规范列表记录在 contract 的 `deleted_tile_names` 字段中。",
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
