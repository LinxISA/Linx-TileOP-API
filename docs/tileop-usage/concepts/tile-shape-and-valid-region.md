# Shape、valid region 与 SizeCode

物理 Tile shape、valid region 和 SizeCode 是不同概念。valid rows/columns 不得超过物理
rows/columns，SizeCode 表示容量而不是重新定义逻辑矩阵维度。

## 分区与组装

对于需要按连续区域处理一个 parent Tile 的算法，可以使用 TileOP 的分区接口：

```cpp
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

Parent parent;
auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
auto part = parts[0][j];

TileArray<Fragment, 1, 4> output;
auto slot = output[0][j];
```

`TPARTVIEW` 返回 parent 的借用分区视图，不复制 parent Tile；`j` 可以是运行时
索引。分区维度、布局、dtype、物理 shape、valid shape 和 byte coverage 在编译期
检查。当前 inline-asm 快速路径要求 Local、rank-2、连续 RowMajor、编译期固定的
分区尺寸。

`TileArray<SubTile, Rows, Cols>` 是组装过程中的二维 slot 容器，`array[row][col]`
返回 `TileArrayOutputRef<SubTile>`。slot 通常作为写入型 TileOP 的 destination，
例如：

```cpp
using OutputParent = TileLeft<__bf16, 32, 64>;
using OutputFragment = TileLeft<__bf16, 32, 16>;
using InputFragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

TileArray<OutputFragment, 1, 4> fragments;
InputFragment input;
for (int j = 0; j < 4; ++j) {
  auto slot = fragments[0][j];
  TCVT(slot, input);
}

OutputParent output = TASSEMBLY<OutputParent>(std::move(fragments));
```

`TASSEMBLY` 要求所有 slot 覆盖并组成目标 parent；它返回完整 parent Tile，
不额外生成独立的 `B.ASSEMBLE`。在当前实现中，`TCVT` 写入 slot 时负责生成
对应的 `B.ASSEMBLE`，slot ordinal 自动决定 INIT/MIDDLE/LAST 生命周期。

`TPARTVIEW/TASSEMBLY` 的 ISA modifier 编码和 binder 附着规则见
[B.SUBVIEW/B.ASSEMBLE range modifiers](../range-modifiers.md)。
