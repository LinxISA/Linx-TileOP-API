#!/usr/bin/env python3
"""Fail-closed validation of the exact PTO 0.58.3 ELF note."""

from __future__ import annotations

import struct
import sys
from pathlib import Path


ELF_MACHINE_LINX = 0xE9
PT_NOTE = 4
SHT_NOTE = 7
EXPECTED_DESC = (
    b'{"encoding_abi":"pto-isa-0.58.3-mode-function-v1",'
    b'"encoding_projection_sha256":'
    b'"8a48b80e04484c70870f155bf9efc79d2a805cf99e809f4e4e8a7e6a7eb34172",'
    b'"release":"0.58.3"}'
)


class IdentityError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise IdentityError(message)


def bounded(data: bytes, offset: int, size: int, label: str) -> bytes:
    require(offset >= 0 and size >= 0 and offset + size <= len(data),
            f"{label} is outside the ELF file")
    return data[offset:offset + size]


def c_string(table: bytes, offset: int) -> bytes:
    require(0 <= offset < len(table), "section name offset is invalid")
    end = table.find(b"\0", offset)
    require(end >= 0, "section name is not NUL-terminated")
    return table[offset:end]


def parse_notes(contents: bytes) -> list[tuple[bytes, int, bytes]]:
    notes: list[tuple[bytes, int, bytes]] = []
    cursor = 0
    while cursor < len(contents):
        require(cursor + 12 <= len(contents), "truncated ELF note header")
        namesz, descsz, note_type = struct.unpack_from("<III", contents, cursor)
        cursor += 12
        name = bounded(contents, cursor, namesz, "ELF note owner")
        cursor += (namesz + 3) & ~3
        desc = bounded(contents, cursor, descsz, "ELF note descriptor")
        cursor += (descsz + 3) & ~3
        require(namesz > 0 and name.endswith(b"\0"),
                "ELF note owner is not NUL-terminated")
        notes.append((name, note_type, desc))
    require(cursor == len(contents), "ELF note padding exceeds its container")
    return notes


def validate(path: Path) -> None:
    data = path.read_bytes()
    require(len(data) >= 64, "truncated ELF header")
    require(data[:4] == b"\x7fELF", "missing ELF magic")
    require(data[4] == 2, "PTO identity requires ELF64")
    require(data[5] == 1, "PTO identity requires little-endian ELF")
    require(data[6] == 1, "unsupported ELF identification version")

    machine = struct.unpack_from("<H", data, 18)[0]
    require(machine == ELF_MACHINE_LINX, "ELF machine is not Linx")
    phoff, shoff = struct.unpack_from("<QQ", data, 32)
    ehsize, phentsize, phnum, shentsize, shnum, shstrndx = struct.unpack_from(
        "<HHHHHH", data, 52)
    require(ehsize == 64, "unexpected ELF64 header size")
    require(phentsize == 56 and phnum > 0, "missing ELF64 program headers")
    require(shentsize == 64 and shnum > 0, "missing ELF64 section headers")
    require(shstrndx < shnum, "invalid section-name table index")

    program_headers = []
    for index in range(phnum):
        entry = bounded(data, phoff + index * phentsize, phentsize,
                        "program header")
        program_headers.append(struct.unpack("<IIQQQQQQ", entry))

    section_headers = []
    for index in range(shnum):
        entry = bounded(data, shoff + index * shentsize, shentsize,
                        "section header")
        section_headers.append(struct.unpack("<IIQQQQIIQQ", entry))

    shstr = section_headers[shstrndx]
    names = bounded(data, shstr[4], shstr[5], "section-name table")
    pto_sections = []
    all_pto_notes = []
    for section in section_headers:
        name = c_string(names, section[0]) if section[0] else b""
        if section[1] != SHT_NOTE:
            continue
        contents = bounded(data, section[4], section[5], "SHT_NOTE section")
        notes = parse_notes(contents)
        all_pto_notes.extend(note for note in notes if note[0] == b"PTO\0")
        if name == b".note.pto.isa":
            pto_sections.append((section, notes))

    require(len(pto_sections) == 1, "expected exactly one .note.pto.isa section")
    section, section_notes = pto_sections[0]
    require(section[8] == 4, ".note.pto.isa must have 4-byte alignment")
    matching_segments = [
        header for header in program_headers
        if header[0] == PT_NOTE and header[2] == section[4]
        and header[5] == section[5]
    ]
    require(len(matching_segments) == 1,
            ".note.pto.isa must have one exact PT_NOTE segment")
    require(matching_segments[0][7] == 4, "PTO PT_NOTE p_align must be 4")

    pto_notes = [note for note in section_notes if note[0] == b"PTO\0"]
    require(len(pto_notes) == 1 and len(all_pto_notes) == 1,
            "PTO identity note is missing or conflicting")
    owner, note_type, desc = pto_notes[0]
    require(owner == b"PTO\0", "PTO note owner must be PTO\\0")
    require(note_type == 1, "PTO note type must be 1")
    require(len(desc) == 165, "PTO note descriptor size must be 165")
    require(not desc.endswith(b"\0"), "PTO JSON must not have a trailing NUL")
    require(desc == EXPECTED_DESC, "PTO identity JSON is not exact 0.58.3")


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {argv[0]} ELF", file=sys.stderr)
        return 2
    try:
        validate(Path(argv[1]))
    except (OSError, IdentityError, struct.error) as error:
        print(f"FAIL PTO identity: {error}", file=sys.stderr)
        return 1
    print("PTO ISA 0.58.3 ELF identity: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
