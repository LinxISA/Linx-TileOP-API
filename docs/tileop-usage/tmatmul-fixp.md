# Fixed-point matrix wrappers

Fixed-point matrix variants are CUBE operations. The wrapper encodes fixed-point attributes with
`B.FPATR`, binds matrix and parameter tiles in their architectural order, and writes ordinary Local
tile destinations.

Requirements:

- M, N, and K are powers of two.
- Each Local or Shared tile uses a per-PE `TSize` from 128 B through 8 KiB.
- Valid rows and columns do not exceed allocated rows and columns.
- Shared matrix operands use absolute `S0` through `S255` names allocated by the compiler.
- Local and Shared inputs may be mixed only in combinations supported by the corresponding wrapper.

The concrete matrix operation name, extra parameter ordering, and result arity must match the pinned
LinxISA v0.58 contract. No compatibility wrapper may synthesize a removed machine operation.
