#!/usr/bin/env python3
"""Generate developer-oriented TileOP pages from the API and PTO-SPEC."""
from __future__ import annotations
import json
import re
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC_ROOT = ROOT.parent / "pto-spec"
if not (SPEC_ROOT / "spec/catalog/tile-operations.json").exists():
    SPEC_ROOT = ROOT.parent.parent / "pto-spec"
CATALOG = SPEC_ROOT / "spec/catalog/tile-operations.json"
HEADER = ROOT / "include/jcore/template_asm.hpp"
DOC_ROOT = ROOT / "docs/tileop-usage"
SPEC_DOC_ROOT = SPEC_ROOT / "docs/tile"
PTO_SPEC_REF = "v0.58.4.1"

def section(text: str, title: str) -> str:
    m = re.search(rf"^## {re.escape(title)}\s*$", text, re.M)
    if not m: return ""
    rest = text[m.end():]
    n = re.search(r"^## ", rest, re.M)
    return rest[:n.start() if n else len(rest)].strip()

def clean(text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", text, flags=re.S)
    text = re.sub(r"`([^`]+)`", r"\1", text)
    return re.sub(r"\s+", " ", text).strip()

def spec_doc(name: str) -> Path:
    matches = list(SPEC_DOC_ROOT.rglob(f"{name}.md"))
    if not matches: raise FileNotFoundError(f"PTO-SPEC page not found: {name}")
    return matches[0]

def output_doc(name: str) -> Path:
    matches = [p for p in DOC_ROOT.rglob(f"{name}.md")
               if "concepts" not in p.parts and "generated" not in p.parts]
    if not matches: raise FileNotFoundError(f"usage page not found: {name}")
    return matches[0]

def signatures(name: str, header: str) -> list[str]:
    result = []
    for m in re.finditer(rf"\b{re.escape(name)}\s*\(", header):
        opening = header.find("(", m.start(), m.end())
        depth = 0; closing = None
        for i in range(opening, len(header)):
            if header[i] == "(": depth += 1
            elif header[i] == ")":
                depth -= 1
                if depth == 0: closing = i; break
        if closing is None: continue
        start = header.rfind("\n", 0, m.start()) + 1
        template = header.rfind("template", 0, start)
        if template >= 0 and "\n\n" not in header[template:start] and "{" not in header[template:start]:
            start = template
        value = re.sub(r"\s+", " ", header[start:closing + 1]).strip() + ";"
        if value.startswith(("void ", "inline ", "PTO_SHARED_INLINE", "template ")) and value not in result:
            result.append(value)
    return result

def split_top_level(value: str, delimiter: str = ",") -> list[str]:
    """Split a C++-like list without splitting nested template arguments."""
    result = []
    depth = 0
    start = 0
    pairs = {"<": ">", "(": ")", "[": "]", "{": "}"}
    openings = set(pairs)
    closings = set(pairs.values())
    for index, char in enumerate(value):
        if char in openings:
            depth += 1
        elif char in closings:
            depth -= 1
        elif char == delimiter and depth == 0:
            result.append(value[start:index].strip())
            start = index + 1
    result.append(value[start:].strip())
    return [item for item in result if item]

def format_signature(signature: str, width: int = 100) -> str:
    """Lay out generated declarations without changing their C++ meaning."""
    signature = re.sub(r"\s+", " ", signature).strip()
    template_prefix = ""
    if signature.startswith("template <"):
        opening = signature.find("<")
        depth = 0
        closing = None
        for index in range(opening, len(signature)):
            if signature[index] == "<":
                depth += 1
            elif signature[index] == ">":
                depth -= 1
                if depth == 0:
                    closing = index
                    break
        if closing is not None:
            template_items = split_top_level(signature[opening + 1:closing])
            compact = f"template <{', '.join(template_items)}>"
            if len(compact) <= width:
                template_prefix = compact + "\n"
            else:
                template_prefix = "template <\n    " + ",\n    ".join(template_items) + ">\n"
            signature = signature[closing + 1:].strip()

    function = re.search(r"\b[A-Z][A-Z0-9_]*\s*\(", signature)
    opening = function.end() - 1 if function else signature.find("(")
    closing = signature.rfind(")")
    if opening < 0 or closing < opening:
        return template_prefix + signature
    prefix = signature[:opening].rstrip()
    suffix = signature[closing + 1:].strip()
    requires = re.search(r"(?:^|\s)(requires\(.+?\))\s+", prefix)
    if requires and len(prefix) > width:
        before = prefix[:requires.start()].rstrip()
        prefix = ((before + "\n") if before else "") + requires.group(1) + "\n" + prefix[requires.end():].lstrip()
    parameters = split_top_level(signature[opening + 1:closing])
    compact = f"{prefix}({', '.join(parameters)}){suffix}"
    if len(compact) <= width:
        return template_prefix + compact
    return (template_prefix + prefix + "(\n    "
            + ",\n    ".join(parameters) + f"){suffix}")

def composition(text: str) -> str:
    body = section(text, "Block composition")
    m = re.search(r"```(?:asm)?\s*\n(.*?)```", body, re.S)
    value = m.group(1).replace("\r", "").strip() if m else clean(body)
    return format_composition(value)

def format_composition(value: str) -> str:
    """Format a bundle skeleton as one aligned instruction per line."""
    value = re.sub(r"^BSTART\s+\.", "BSTART.", value, flags=re.M)
    value = re.sub(r"^(B\.[A-Z]+)\s+/B\.", r"\1/B.", value, flags=re.M)
    logical_lines = []
    for raw_line in value.splitlines():
        line = raw_line.strip()
        if line and not line.startswith("B") and logical_lines:
            logical_lines[-1] += " " + line
        else:
            logical_lines.append(line)
    lines = []
    for line in logical_lines:
        if not line:
            continue
        match = re.match(
            r"(B(?:START(?:\.[A-Z0-9_.]+)?|STOP|\.[A-Z]+(?:/B\.[A-Z]+)?))\s*(.*)",
            line,
        )
        if not match:
            lines.append(line)
            continue
        mnemonic, operands = match.groups()
        dim = re.match(r"LB([0-2])=([A-Za-z][A-Za-z0-9_]*)\s*(.*)", operands)
        if mnemonic == "B.DIM" and dim:
            lb, name, remainder = dim.groups()
            register = name if name.startswith("r") else f"r{name}"
            operands = f"{register}, 0, ->LB{lb}"
            if remainder:
                operands += f"  ; {remainder}"
        operands = operands.replace("<last>", "last")
        separator = " " if mnemonic.startswith("BSTART") else " " * max(1, 12 - len(mnemonic))
        lines.append(mnemonic if not operands else f"{mnemonic}{separator}{operands}")
    return "\n".join(lines)

def call_arguments(signature: str) -> str:
    params = signature[signature.find("(") + 1:signature.rfind(")")]
    names = []
    depth = 0
    start = 0
    chunks = []
    for i, char in enumerate(params + ","):
        if char in "<({": depth += 1
        elif char in ">)}": depth -= 1
        elif char == "," and depth == 0:
            chunks.append(params[start:i].strip())
            start = i + 1
    for param in chunks:
        param = param.split("=")[0].strip()
        match = re.search(r"([A-Za-z_]\w*)\s*(?:\]|$)", param)
        if match:
            names.append(match.group(1))
    return ", ".join(names)

def render(name: str, spec: Path, header: str, existing: str = "") -> str:
    text = spec.read_text(encoding="utf-8")
    purpose = section(text, f"What {name} does") or section(text, "Purpose")
    intro = next((clean(p) for p in re.split(r"\n\s*\n", purpose)
                  if clean(p) and not clean(p).startswith("This operation")), f"执行 `{name}` 操作。")
    sigs = signatures(name, header)
    if not sigs: raise ValueError(f"No C++ declaration found: {name}")
    link = str(spec.relative_to(SPEC_ROOT)).replace("\\", "/")
    invocation = f"{name}({call_arguments(sigs[0])});"
    rendered = f'''# {name}

{intro}

## C++ 接口

当前 API 中可用的调用形式：

```cpp
{chr(10).join(format_signature(signature) for signature in sigs)}
```

## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

除通用 Tile 约束外，必须满足 PTO-SPEC 对本操作规定的操作数角色、数据类型组合、形状、布局、有效区域、容量、存储位置、PE mask 以及 alias 规则。对于需要 Shared Tile、标量、索引、scale、bias 或选项对象的重载，只能使用接口声明的参数形式；不能通过省略参数来伪造另一种操作数组合。

## 默认值

未显式传入的可选参数使用该 C++ 重载和 PTO-SPEC contract 规定的默认值。默认选项、维度、布局、padding、scale mask 和属性字段可能与显式编码的零值不同；调用者不得把“省略”与“传入零值”自动等同。

## 异常和边界行为

类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，可能在编译期或运行前检查阶段被拒绝。有效区域为空、部分有效区域、边界坐标、padding、数值溢出、NaN/无穷值、输入输出 alias、内存 fault 以及 `PE_MASK=0000` 的行为均以该操作的 PTO-SPEC contract 为准；失败时不应假定已经产生部分输出或其他副作用。

## 结果说明

成功调用后，`{name}` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
{composition(text)}
```

## 使用示例

```cpp
#include "jcore/template_asm.hpp"

// 使用满足 {name} 约束的具体 Tile 类型和参数。
{invocation}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`{name}.md`](https://github.com/PTO-ISA/pto-spec/blob/{PTO_SPEC_REF}/{link})。
'''
    # Keep hand-written examples.  The generated fallback is useful for new
    # pages, but must not erase a verified data-flow example on regeneration.
    old_example = section(existing, "使用示例")
    if old_example and "使用满足" not in old_example and "请替换" not in old_example:
        rendered = re.sub(
            r"## 使用示例\n.*?(?=\n## 完整语义)",
            "## 使用示例\n\n" + old_example,
            rendered,
            flags=re.S,
        )
    # Keep reviewed instruction skeletons, especially for operations whose
    # specification describes Local/Shared variants in prose instead of an
    # assembly code block.
    old_composition = section(existing, "Bundle composition")
    old_asm = re.search(r"```asm\s*\n(.*?)```", old_composition, re.S)
    if old_asm and "BSTART" in old_asm.group(1):
        reviewed = format_composition(old_asm.group(1))
        rendered = re.sub(
            r"(## Bundle composition\n.*?```asm\n).*?(\n```)",
            lambda match: match.group(1) + reviewed + match.group(2),
            rendered,
            count=1,
            flags=re.S,
        )
    return rendered

def main() -> None:
    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    header = HEADER.read_text(encoding="utf-8")
    for op in catalog["operations"]:
        target = output_doc(op["name"])
        existing = target.read_text(encoding="utf-8")
        target.write_text(render(op["name"], spec_doc(op["name"]), header, existing), encoding="utf-8")
        print(target.relative_to(ROOT))

if __name__ == "__main__": main()