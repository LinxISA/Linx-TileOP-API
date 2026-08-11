# CUBE operations

CUBE owns matrix multiply and matrix-vector families. The canonical operation list is generated in
[engines.md](engines.md). TileOP wrappers select the corresponding CUBE-classified named form such as
`BSTART.TMATMUL` and bind Local
operands with `B.IOT` or Shared operands with `B.IOS`.

```cpp
TileLeft<float, 32, 32> a;
TileRight<float, 32, 32> b;
TileLeft<float, 32, 32> out;
TMATMUL(out, a, b);
```

M, N, and K are powers of two. Valid rows and columns may describe a smaller active region but may
not exceed the allocated rows and columns. Fixed-point attributes and extra parameter tiles are
carried by the matching matrix operation; they do not introduce a separate conversion instruction.

For Shared operands, `B.IOS Sx, mask=1111` contributes the ordered Shared source while Local operands
continue to use `B.IOT`. The compiler allocates absolute Shared register names.
