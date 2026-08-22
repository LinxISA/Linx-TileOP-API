# PTO ISA 0.58.3 migration

This API revision is derived from LinxISA authority commit
`dd52a2e579d8058c0d8e33043e705122b340e73f`, which locks PTO-SPEC release
`v0.58.3` at commit `e599a3d36ebfad43362ff591ea5e128816c684c7`.
The generated contract records both repositories' exact commits, trees, and
content hashes.

## Source changes

- The semantic engine inventory is exactly 31 VEC, 56 SFU, 10 TLSU, and 12
  CUBE operations. TEPL remains the single compiled VEC/SFU carrier.
- `PEMask` template arguments are limited to the masks produced by the fixed
  PEMode decoder: `0000`, `1000`, `0100`, `0010`, `0001`, `1100`, `1110`,
  and `1111`. APIs that require participation reject zero.
- Local SizeCode accepts 128 B through 64 KiB; Shared SizeCode accepts 128 B
  through 256 KiB.
- TLOAD/TSTORE pass `B.IOR.RegSrc1` in bytes. Existing `global_tensor`
  constructors still accept element strides.
- `B.FPATR` has nine fields. `transpose_a()` and `transpose_b()` are legal
  only for the corresponding cooperative Shared matrix primary.
- `CubeTileM16`, `CubeTileM32`, `CubeTileN8`, and the accumulator aliases
  model persistent 128-byte CELL storage. `TLOAD_CUBE` and `TSTORE_CUBE`
  provide the assigned GM conversion boundary.
- Matrix ACC forms retain explicit C input and an early-clobbered D output;
  the compiler must allocate distinct Local Tile indices.

## Compatibility boundary

The source remains compatible with the preceding TEPL carrier spelling, but
new 0.58.3 descriptors cannot be assembled by a 0.58.1 LLVM. Passing
`make check` proves the header-level and provenance contracts only. Promotion
also requires the exact 0.58.3 compiler to pass `test/tileop_api/compile.all`,
the negative corpus, and disassembly checks.

At the reviewed LLVM integration point, the MC/CodeGen compiler implements the
0.58.3 instruction encodings but does not ship the C++ Tile register frontend
used by this header (`linx_blkc.h`, `tile_size`, or the `Tr` constraint).
`verify_pto0583_asm.sh` therefore proves the integrated assembler,
disassembler, SizeCode/PEMode, layout, and FPATR contracts directly. The full
C++ target corpus remains a compiler packaging/frontend blocker until those
Tile ABI definitions are supplied; `make check` still covers all public C++
types and builders with a host syntax gate.
