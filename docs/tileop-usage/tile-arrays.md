# Tile partition and assembly views

`TPARTVIEW` and `TASSEMBLY` provide a high-level C++ interface for splitting a
parent Tile into aligned views and assembling independently produced fragments
back into a parent Tile. They are the TileOP-side interface for the
compiler-derived region model; users do not spell `B.SUBVIEW` or `B.ASSEMBLE`
directly.

## 接口身份

| 项目 | 内容 |
| --- | --- |
| API 名称 | `TPARTVIEW`, `TASSEMBLY` |
| ISA 关系 | `B.SUBVIEW`, `B.ASSEMBLE` range modifiers |
| 典型协同操作 | `TCVT`, `TROWMAX`, `TROWSUM`, `TEXP` |
| 当前 inline-asm carrier | `TEPL` |
| C++ 定义位置 | `include/common/pto_tile_region.hpp` |

`TPARTVIEW` 产生借用的 source-side partition view；`TASSEMBLY` 消费一个
完成填充的 `TileArray` 并返回完整 parent Tile。对于当前 Linx inline-asm
路径，`B.SUBVIEW` 紧跟产生 view 的 Tile binder，`B.ASSEMBLE` 紧跟写入
assembly slot 的 destination binder。

## C++ 接口

```cpp
template <typename SubTile, int Rows, int Cols, typename Parent>
auto TPARTVIEW(Parent &parent)
    -> BorrowedTileArray<Parent, SubTile, Rows, Cols>;

template <typename Parent, typename SubTile, int Rows, int Cols>
auto TASSEMBLY(TileArray<SubTile, Rows, Cols> &&array) -> Parent;
```

`TileArray<SubTile, Rows, Cols>` 是 assembly 的线性二维 slot 容器；
`array[row][col]` 返回 `TileArrayOutputRef<SubTile>`，可作为写入型 TileOP
的 destination。slot 的 assembly phase 由接口根据 ordinal 推导，不需要在
调用处传入 `INIT`、`LAST` 或 `ParentSizeCode`。

## 基本用法

下面的例子将一个 `32x64` parent 分成四个连续的 `32x16` fragment：

```cpp
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

Parent parent;
auto source_array = TPARTVIEW<Fragment, 1, 4>(parent);
auto source_fragment = source_array[0][j];

TileArray<Fragment, 1, 4> destination_array;
auto destination_slot = destination_array[0][j];
```

`j` 可以是运行时整数。对于 RowMajor 连续分区，接口根据 slot 的线性
ordinal 计算 fragment 的 byte offset；分区尺寸和 parent 覆盖关系仍在
编译期检查。

## 转换后组装

常见用法是对每个 fragment 执行 `TCVT`，然后将四个目标 slot 组装为一个
完整 parent：

```cpp
using InputFragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using OutputFragment = TileLeft<__bf16, 32, 16>;
using OutputParent = TileLeft<__bf16, 32, 64>;

InputFragment input;
TileArray<OutputFragment, 1, 4> output_array;

for (int j = 0; j < 4; ++j) {
  auto output_slot = output_array[0][j];
  TCVT(output_slot, input);
}

OutputParent output = TASSEMBLY<OutputParent>(std::move(output_array));
```

每个 `TCVT` 都是一个独立的 destination-side slot 写入：

- 第一个 slot 使用 `B.ASSEMBLE INIT=1, LAST=0`；
- 中间 slot 使用 `B.ASSEMBLE INIT=0, LAST=0`；
- 最后一个 slot 使用 `B.ASSEMBLE INIT=0, LAST=1`；
- 只有一个 slot 时使用 `INIT=1, LAST=1`。

`TASSEMBLY` 本身负责返回 parent Tile，不额外产生一条独立的
`B.ASSEMBLE` 指令。调用者必须保证所有 slot 已经按顺序完成写入。

## 编译期约束

类型接口检查以下条件：

- positive rank-2 partition dimensions;
- matching dtype, location, and layouts;
- fragment capacity of at least one 128-byte CELL;
- exact physical-shape, valid-shape, and byte coverage of the parent;
- non-copyable assembly arrays.

当前 inline-asm 快速路径支持 `Local`、rank-2、连续 `RowMajor` partition，
且 partition extents 必须是编译期常量。CUBE/boxed layout 的 region 偏移和
compiler intrinsic lowering 不属于当前快速路径；未来实现切换时，上述源代码
接口保持不变。

## 相关章节

- [Range modifiers](range-modifiers.md) — `B.SUBVIEW/B.ASSEMBLE` 的 ISA 编码和生命周期。
- [TCVT](elementwise-tile-tile/format-conversion/TCVT.md) — fragment 转换和 slot destination 用法。
- [Tile constraints](concepts/tile-constraints.md) — shape、valid region 和容量约束。
