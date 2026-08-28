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
- Ordinary Matrix inputs must share one supported numeric class. Their
  accumulator is FP32, S32, or U32 for floating, signed, or unsigned inputs;
  `PreQuant=None` preserves that exact type. C, Bias, D, RowMax, and GroupMax
  are checked against the same central type contract. Scale-bearing MX
  overloads require assigned MX inputs and E8M0 scale shapes.
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

Target compilation defaults to the hosted musl triple
`linx64-unknown-linux-musl`. Override it with `LINX_TARGET` only when testing
another complete Linx runtime profile. `compile.all objects` compiles every
active Linx fixture without requiring a `main`; `compile.all link-smoke` links
the explicit hosted subset and rejects ELFs that do not carry the exact PTO
0.58.3 encoding identity. Pre-v0.58 wrapper fixtures remain cpu_sim-only.

`verify_pto0583_asm.sh` independently gates the integrated assembler,
disassembler, SizeCode/PEMode, layout, DTYPE_NONE, and FPATR contracts. It
must fail on a compiler that silently renders the required CUBE conversion
dtype as FP64.
