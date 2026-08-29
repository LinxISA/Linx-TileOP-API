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
  provide the explicit GM conversion boundary. The unified `TLOAD` and
  `TSTORE` entry points now dispatch to that conversion automatically for
  CUBE Tiles while retaining the explicit names for diagnostic and expert use.
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

## Unified GM transport entry points

New kernel code can use `TLOAD` and `TSTORE` for both ordinary and CUBE Local
Tiles. The wrapper selects the transport from the Tile layout:

```cpp
using A = CubeTileM32<float, 32, 32>;
using B = CubeTileN8<float, 32, 32>;
using C = CubeAccumulatorM32<float, 32, 32>;
using GM = global_tensor<float, RowMajor<32, 32>>;

void matmul_step(GM &ga, GM &gb, GM &gc, A &a, B &b, C &c) {
  TLOAD(a, ga);       // dispatches to ND2M32 CUBE transport
  TLOAD(b, gb);       // dispatches to ND2N8 CUBE transport
  TMATMUL(c, a, b);
  TSTORE(gc, c);      // dispatches to M322ND CUBE transport
}
```

For a non-CUBE Tile, the same spelling remains the normal `B.IOT` transport:

```cpp
using TileT = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<32, 32>>;

void vector_step(GM &gm, TileT &tile) {
  TLOAD(tile, gm);
  TSTORE(gm, tile);
}
```

`TLOAD_CUBE` and `TSTORE_CUBE` remain available when the layout conversion must
be explicit in source code, diagnostics, or compatibility tests. The unified
entry points do not remove the CUBE checks or conversion descriptors; they only
select the existing implementation based on `Tile::IsCubeLayout`.
