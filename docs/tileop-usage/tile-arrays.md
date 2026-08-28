# Experimental aligned Tile arrays

`TPARTVIEW` and `TASSEMBLY` are the proposed compiler-semantic API for aligned
rank-2 Tile partitions. This header surface intentionally does not lower to
inline assembly and does not expose `SizeCode`, `RegSrc`, `INIT`, or `LAST`.

The currently available type layer supports the final source syntax:

```cpp
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

Parent parent;
auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
auto fragment = parts[0][j];

TileArray<Fragment, 1, 4> output;
auto destination = output[0][j];
```

The type contract currently verifies:

- positive rank-2 partition dimensions;
- matching dtype, location, and layouts;
- fragment capacity of at least one 128-byte CELL;
- exact physical-shape, valid-shape, and byte coverage of the parent;
- non-copyable assembly arrays.

`TASSEMBLY` remains a compile-time error until Clang emits the experimental
LLVM region/session operations and the architecture contract fixes the PTO
Local capacity boundary. No ordinary API silently falls back to a real Tile
array, memory, raw range wrappers, or inline assembly.
