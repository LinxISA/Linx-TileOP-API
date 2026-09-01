#!/usr/bin/env python3
"""Compile every C++ block in a TileOP documentation usage-example section.

By default this check uses the host C++ compiler and a temporary copy of the
Linx header with inline-assembly statements replaced by no-ops.  This still
instantiates the real public templates and validates C++ syntax, names,
overloads, concepts, and static assertions.  Pass --target-cxx to additionally
compile the unchanged source with the Linx compiler.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOC_ROOT = ROOT / "docs/tileop-usage"
USAGE_SECTION = re.compile(
    r"^## (?:\d+\.\s*)?使用示例\s*$([\s\S]*?)(?=^## |\Z)", re.M
)
CPP_BLOCK = re.compile(r"^```cpp\s*\n([\s\S]*?)^```\s*$", re.M)
FIXED_REGISTER = re.compile(
    r'\bregister\s+([^;\n]*?)\s+asm\s*\('
    r'\s*"r"\s*#\s*[A-Za-z_]\w*\s*\)'
    r'|\bregister\s+([^;\n]*?)\s+asm\s*\(\s*"r\d+"\s*\)'
)
FUNCTION_DEFINITION = re.compile(
    r"^\s*(?:template\s*<[^;]+>\s*)?(?:void|int|auto)\s+"
    r"[A-Za-z_]\w*\s*\([^;]*\)\s*\{",
    re.M | re.S,
)


@dataclass(frozen=True)
class Example:
    document: Path
    block: int
    source: str


def examples() -> list[Example]:
    result: list[Example] = []
    for document in sorted(DOC_ROOT.rglob("*.md")):
        match = USAGE_SECTION.search(document.read_text(encoding="utf-8"))
        if not match:
            continue
        for number, block in enumerate(CPP_BLOCK.finditer(match.group(1)), 1):
            result.append(Example(document, number, block.group(1)))
    return result


def strip_inline_asm(source: str) -> str:
    """Replace each ``asm volatile (...)`` statement without parsing C++."""
    output: list[str] = []
    cursor = 0
    while True:
        start = source.find("asm volatile", cursor)
        if start < 0:
            output.append(source[cursor:])
            return "".join(output)
        output.append(source[cursor:start])
        line_start = source.rfind("\n", 0, start) + 1
        previous_line_end = max(0, line_start - 1)
        previous_line_start = source.rfind("\n", 0, previous_line_end) + 1
        in_continued_macro = source[
            previous_line_start:previous_line_end
        ].rstrip().endswith("\\")
        opening = source.find("(", start)
        brace = source.find("{", start)
        semicolon = source.find(";", start)
        # Some comments mention "asm volatile" before the actual statement.
        if opening < 0 or (brace >= 0 and brace < opening) or (
                semicolon >= 0 and semicolon < opening):
            output.append(source[start:start + len("asm volatile")])
            cursor = start + len("asm volatile")
            continue
        if opening < 0:
            line = source.count("\n", 0, start) + 1
            raise ValueError(f"unterminated asm volatile statement at line {line}")
        depth = 0
        quote: str | None = None
        escaped = False
        position = opening
        while position < len(source):
            char = source[position]
            if quote is not None:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == quote:
                    quote = None
            # Inline-assembly templates and constraints are string literals.
            # Ignoring apostrophes also avoids treating prose such as "A's"
            # in a nearby comment as the start of a character literal.
            elif char == '"':
                quote = char
            elif char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    position += 1
                    while position < len(source) and source[position].isspace():
                        position += 1
                    if position < len(source) and source[position] == ";":
                        cursor = position + 1
                        output.append("(void)0;")
                        break
                    # A few function-like macros intentionally omit the
                    # semicolon from their replacement list; the invocation
                    # supplies it.  Preserve the continuation and replace only
                    # the asm expression in that case.
                    if position < len(source) and source[position] == "\\":
                        cursor = position
                        output.append("(void)0")
                        break
                    if in_continued_macro:
                        # The asm is the final replacement-list token.  The
                        # closing ')' is followed by the next source line, so
                        # put back the newline consumed as whitespace above.
                        cursor = position
                        output.append("(void)0\n")
                        break
                    else:
                        line = source.count("\n", 0, start) + 1
                        context = source[position:position + 80].replace("\n", "\\n")
                        raise ValueError(
                            f"asm volatile statement at line {line} has no semicolon: {context}")
            position += 1
        else:
            line = source.count("\n", 0, start) + 1
            raise ValueError(f"unterminated asm volatile statement at line {line}")


def host_include_tree(destination: Path) -> None:
    shutil.copytree(ROOT / "include", destination)
    for header in destination.rglob("*.hpp"):
        source = header.read_text(encoding="utf-8")
        if "asm volatile" not in source and not FIXED_REGISTER.search(source):
            continue
        # Keep preprocessor continuations intact.  strip_inline_asm stops after
        # the statement's semicolon, so a macro's trailing backslash remains in
        # place.  Joining continuations would flatten a whole macro onto one
        # line and could replace its declaration/body along with the asm.
        source = strip_inline_asm(source)
        # Linx permits locals bound to architectural registers (r0-r7).  Host
        # compilers cannot validate those target-specific names, but can still
        # check the surrounding declarations, initializers, and control flow.
        source = FIXED_REGISTER.sub(lambda match: next(
            group for group in match.groups() if group is not None
        ), source)
        # A no-op replacement can leave a namespace-scope expression statement
        # when an operation is instantiated by a documentation snippet at
        # namespace scope.  Make the replacement a declaration in both block
        # and namespace contexts.
        source = source.replace("(void)0;", "static_assert(true);")
        header.write_text(source, encoding="utf-8")


def compilation_unit(example: Example) -> str:
    """Put statement-style examples in a function while leaving includes out."""
    if FUNCTION_DEFINITION.search(example.source):
        return example.source
    includes: list[str] = []
    body: list[str] = []
    for line in example.source.splitlines(keepends=True):
        (includes if line.lstrip().startswith("#include") else body).append(line)
    return ("".join(includes) + "\nvoid tileop_documentation_example() {\n" +
            "".join(body) + "\n}\n")


def compile_examples(items: list[Example], compiler: str, include: Path,
                     work: Path, target_flags: list[str], jobs: int,
                     timeout: float) -> list[str]:
    def compile_one(index: int, example: Example) -> tuple[int, str | None]:
        source = work / f"example-{index:03d}.cpp"
        source.write_text(compilation_unit(example), encoding="utf-8")
        command = [compiler, "-std=c++20", "-D__linx", "-fsyntax-only",
                   "-I", str(include), *target_flags, str(source)]
        try:
            completed = subprocess.run(
                command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, check=False, timeout=timeout,
            )
        except subprocess.TimeoutExpired as error:
            output = error.stdout or ""
            if isinstance(output, bytes):
                output = output.decode(errors="replace")
            relative = example.document.relative_to(ROOT)
            return index, (f"{relative} (使用示例 block {example.block})\n"
                           f"$ {' '.join(command)}\n"
                           f"timed out after {timeout:g}s\n{output.rstrip()}")
        if completed.returncode:
            relative = example.document.relative_to(ROOT)
            return index, (f"{relative} (使用示例 block {example.block})\n"
                           f"$ {' '.join(command)}\n{completed.stdout.rstrip()}")
        return index, None

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
        results = list(executor.map(
            lambda pair: compile_one(*pair), enumerate(items),
        ))
    return [failure for _, failure in sorted(results) if failure is not None]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"),
                        help="host compiler (default: CXX or c++)")
    parser.add_argument("--target-cxx", help="optional Linx clang++")
    parser.add_argument("--target", default="linx64v5-unknown-linux-musl")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1),
                        help="maximum concurrent compiler processes (default: %(default)s)")
    parser.add_argument("--timeout", type=float, default=30,
                        help="per-example compiler timeout in seconds (default: %(default)s)")
    args = parser.parse_args()
    if args.jobs < 1 or args.timeout <= 0:
        parser.error("--jobs and --timeout must be greater than zero")
    items = examples()
    if not items:
        print("error: no 使用示例 C++ blocks found", file=sys.stderr)
        return 1

    with tempfile.TemporaryDirectory(prefix="tileop-doc-examples-") as temp:
        work = Path(temp)
        include = work / "include"
        host_include_tree(include)
        host_work = work / "host"
        host_work.mkdir()
        failures = compile_examples(
            items, args.cxx, include, host_work,
            ["-include", str(ROOT / "test/linx_host_type_shim.hpp")],
            args.jobs, args.timeout,
        )
        if args.target_cxx:
            target_work = work / "target"
            target_work.mkdir()
            failures.extend(compile_examples(
                items, args.target_cxx, ROOT / "include", target_work,
                [f"--target={args.target}", "-mlxbc", "-fenable-matrix"],
                args.jobs, args.timeout,
            ))

    if failures:
        print("\n\n".join(failures), file=sys.stderr)
        print(f"\nFAILED: {len(failures)} compilation(s)", file=sys.stderr)
        return 1
    modes = "host surface + Linx target" if args.target_cxx else "host surface"
    print(f"PASS: compiled {len(items)} usage examples ({modes})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())