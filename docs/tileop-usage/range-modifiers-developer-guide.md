# B.SUBVIEW / B.ASSEMBLE Developer Guide

本文面向 TileOP kernel 开发者，介绍当前可用的新版 range modifier C++ 接口。
这些接口将 PTO ISA 的 `B.SUBVIEW` 和 `B.ASSEMBLE` 封装为可传递的 range
carrier，避免在业务代码中直接拼接 ISA 字段。

## 适用范围

当前接口适用于以下场景：

- source Tile 需要绑定一个 `B.SUBVIEW` 区域；
- destination Tile 需要绑定一个 `B.ASSEMBLE` 区域；
- 需要在多个操作或多个 kernel 阶段复用相同的 range 描述；
- 需要让 `SubviewSizeCode`、`ParentSizeCode`、`INIT`、`LAST` 和 `RegSrc`
  在编译期固定。

当前实现通过 inline asm 生成 range modifier。它不是 LLVM intrinsic lowering，
但保持了后续切换到 compiler lowering 时的 C++ 调用形式。

## 接口与 ISA 的对应关系

| C++ 接口 | ISA modifier | 操作数角色 |
| --- | --- | --- |
| `range::subview` / `range::Subview` | `B.SUBVIEW` | source |
| `range::assemble` / `range::Assemble` | `B.ASSEMBLE` | destination |

modifier 必须紧跟同一 bundle 中对应的 `B.IOT` 或 `B.IOS` binder。调用者只需
把 carrier 传给消费该 Tile 的 TileOP；TileOP wrapper 负责发出 modifier。

不要混淆以下两层接口：

- `range::subview/assemble`：封装单个 Tile binder 的 ISA range modifier；
- `TPARTVIEW/TASSEMBLY`：对 parent Tile 做连续分区和组装的高层 Tile region API。

## 推荐写法

先记住一个简单规则：

```text
Subview  = source 侧的范围描述
Assemble = destination 侧的范围描述
```

二者都不是数据副本。它们只是把“哪个 Tile、使用哪个基地址寄存器、使用
哪个范围参数”打包成一个可以传给 TileOP 的对象。

### Source：`range::subview`

普通场景使用 factory。range size code 从 parent Tile 的 `TilesizeCode` 推导，
开发者只需要填写原始 Tile 和运行时基地址：

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using TileT = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<4, 8>>;

void store_subview(GM &gm, TileT &tile, uintptr_t base_addr) {
  auto view = range::subview(tile, base_addr);
  TSTORE(gm, view);
}
```

各个参数的含义如下：

| 参数 | 如何填写 | 含义 |
| --- | --- | --- |
| `tile` | 要写回的 Local/Shared Tile | 被描述的 source Tile，不会被复制 |
| `base_addr` | 运行时地址值，例如 GM buffer 地址 | 写入默认 `RegSrc=2` 对应的 base register |
| `SubviewSizeCode` | 通常不填写 | 由 `tile::TilesizeCode` 自动推导，表示 source range capacity |
| `Offset` | 默认是 `0` | 编码到 `B.SUBVIEW` 的 `uimm11` 立即数，范围 `0..2047` |
| `RegSrc` | 默认是 `2` | 编码使用的绝对 GPR 编号，范围 `0..23` |

因此这段代码表示：使用 `tile` 作为 source，使用 `base_addr` 作为 `r2` 的值，
在 source binder 后附加一个 offset 为 `0`、size code 自动推导的 `B.SUBVIEW`：

```asm
B.SUBVIEW 0, r2, 0, <TileT::TilesizeCode>
```

`range::subview` 默认使用 `RegSrc=2`。这里的 `base_addr` 是基地址寄存器的值，
不是直接编码到指令中的立即数。

`base_addr` 和 `Offset` 不要混淆：

```text
最终范围地址 = GPR[RegSrc] + ZeroExtend(Offset)
```

也就是说，`base_addr` 是运行时传入的地址，`Offset` 是编译期写入指令的
小立即数。

### Destination：`range::assemble`

普通 destination 使用 `range::assemble`。它的参数填写方式与 `subview` 类似，
区别是它描述 destination，而不是 source：

```cpp
void load_assembled(GM &gm, TileT &tile, uintptr_t base_addr) {
  auto destination = range::assemble(tile, base_addr);
  TLOAD(destination, gm);
}
```

默认形式等价于 `INIT=1, LAST=0`。这表示当前写入是一次 assembly session
的第一个 fragment。对应的 modifier 形式类似：

```asm
B.ASSEMBLE 1, 0, r2, 0, <TileT::TilesizeCode>
```

## 生命周期 helper

`B.ASSEMBLE` 的生命周期不要通过裸的布尔模板参数表达，优先使用命名 helper：

```cpp
auto first = range::assemble(tile, base_addr);          // INIT=1, LAST=0
auto only = range::assemble_init_last(tile, base_addr);  // INIT=1, LAST=1
auto middle = range::assemble_middle(tile, base_addr);  // INIT=0, LAST=0
auto last = range::assemble_last(tile, base_addr);      // INIT=0, LAST=1
```

这些 carrier 应分别传给对应的 destination TileOP。生命周期必须与实际写入
顺序匹配：

```text
single fragment: INIT_LAST
multiple fragments: INIT -> MIDDLE* -> LAST
```

非 `INIT` 形式的 `ParentSizeCode` 按 ISA 合同编码为 `0`，不要手动把 parent
size code 传给 `assemble_middle` 或 `assemble_last`。

实际填写时按写入顺序选择 helper：

```cpp
// 只有一个 fragment：
auto only = range::assemble_init_last(tile, base_addr);
TLOAD(only, gm);

// 多个 fragment：
auto first = range::assemble(tile, base_addr);
TLOAD(first, gm0);

auto middle = range::assemble_middle(tile, base_addr);
TLOAD(middle, gm1);

auto last = range::assemble_last(tile, base_addr);
TLOAD(last, gm2);
```

这里的 `first/middle/last/only` 只是示例变量名。真正重要的是它们对应的
`INIT/LAST` 状态，以及调用顺序：

```text
一个 fragment： assemble_init_last
两个 fragment： assemble -> assemble_last
三个及以上：  assemble -> assemble_middle* -> assemble_last
```

`assemble()`、`assemble_init_last()` 的 INIT 形式会携带 parent size code；
`assemble_middle()`、`assemble_last()` 的非 INIT 形式使用 size code `0`。

## Offset 与 base register

range descriptor 中的 `Offset` 和 `RegSrc` 是编译期字段；基地址值是运行时参数。
当需要指定 offset 或 base register 时，使用 `_at` helper。注意模板参数是
编译期 descriptor 字段，括号中的 `base_addr` 仍然是运行时基地址：

```cpp
auto source = range::subview_at<128, 23>(tile, base_addr);
auto destination = range::assemble_at<128, 23>(tile, base_addr);
```

模板参数含义为：

```text
subview_at<Offset, RegSrc>(parent, range_base)
assemble_at<Offset, RegSrc>(parent, range_base)
```

其中：

- `Offset` 是加到 `GPR[RegSrc]` 上的 byte offset，合法范围是 `0..2047`；
- `RegSrc` 是绝对 GPR 编号，合法范围是 `0..23`；
- `base_addr` 是要放入该 GPR 的运行时值。

例如：

```cpp
auto view = range::subview_at<128, 23>(tile, base_addr);
```

表示：使用 `r23 = base_addr`，再加上立即数 offset `128`，生成：

```asm
B.SUBVIEW 0, r23, 128, <TileSizeCode>
```

只有在 Tile 的默认 capacity 不符合目标 contract 时，才需要同时指定 source
size code：

```cpp
auto source = range::subview_sized_at<12, 2047, 23>(tile, base_addr);
```

其模板参数顺序为：

```text
subview_sized_at<SubviewSizeCode, Offset, RegSrc>(parent, range_base)
```

`SubviewSizeCode` 表示 source binder 的容量编码，不是矩阵的逻辑行数或列数。
除非确实需要覆盖默认 size code，否则不要使用 sized helper。

## 显式 carrier 类型

factory 无法表达特殊的 descriptor 组合时，可以直接使用 carrier 类型：

```cpp
using SourceView = range::Subview<TileT, 1, 0, 0>;
using DestinationView = range::Assemble<TileT, 1, true, false, 0, 0>;

SourceView source_view(tile, base_addr);
DestinationView destination_view(tile, base_addr);

TSTORE(gm, source_view);
TLOAD(destination_view, gm);
```

显式 carrier 适合测试边界、固定 ABI 或需要在类型系统中暴露完整 descriptor
的代码。普通 kernel 代码应优先使用 factory 和生命周期 helper。

## Local 与 Shared

Local Tile carrier 通过 `B.IOT` 绑定，Shared Tile carrier 通过 `B.IOS` 绑定：

```cpp
using LocalTile = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using SharedTileT = SharedTile<LocalTile>;
using GM = global_tensor<float, RowMajor<4, 8>>;

void shared_source(GM &gm, SharedTileT &shared, uintptr_t base_addr) {
  auto view = range::subview(shared, base_addr);
  TSTORE(gm, view);  // B.IOS ... / B.SUBVIEW ...
}

void shared_destination(GM &gm, SharedTileT &shared, uintptr_t base_addr) {
  auto destination = range::assemble(shared, base_addr);
  TLOAD(destination, gm);  // B.IOS ... / B.ASSEMBLE ...
}
```

Shared carrier 使用 Shared handle，不提供 Local Tile 的普通 `data()` 语义。
不要把 `Subview` 用在 destination，也不要把 `Assemble` 用在 source；这两种
角色错误应在编译期被拒绝。

## 约束与失败方式

以下字段会在模板实例化阶段检查：

- `SubviewSizeCode`：`1..12`；
- `ParentSizeCode`：`0..12`；
- `INIT=1` 时 parent size code 必须为 `1..12`；
- `INIT=0` 时 parent size code 必须为 `0`；
- `Offset`：`0..2047`；
- `RegSrc`：`0..23`。

典型非法写法：

```cpp
range::Subview<TileT, 0>(tile, 0);             // size code 0 非法
range::Subview<TileT, 13>(tile, 0);            // 13..15 保留
range::Subview<TileT, 1, 2048>(tile, 0);       // offset 溢出
range::Subview<TileT, 1, 0, 24>(tile, 0);      // RegSrc 溢出
range::Assemble<TileT, 0, true>(tile, 0);      // INIT 需要 parent size
range::Assemble<TileT, 12, false>(tile, 0);    // 非 INIT 必须使用 size 0
```

此外，range modifier 不会改变 wrapped Tile 的 dtype、layout、shape、valid
region、PE mask 或 Tile size contract。若这些属性本身不满足消费该 TileOP 的
要求，错误仍由对应 TileOP 的约束报告。

## 与 Tile region API 配合

`TPARTVIEW` 和 `TASSEMBLY` 适合 parent Tile 的连续分区场景：

```cpp
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

Parent parent;
auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
auto fragment = parts[0][j];

TileArray<Fragment, 1, 4> fragments;
auto slot = fragments[0][j];
```

这类 region API 会根据 fragment ordinal 管理连续区域和 assembly slot；开发者
不需要手动创建 `range::Subview` 或 `range::Assemble`。两种 API 可以在同一
kernel 中共存，但用途不同：range carrier 面向单个 binder，Tile region API
面向 parent/fragment 生命周期。

## Tile region 接口逐项说明

本节把 Tile region 相关的公开接口逐项列出。它们与前面的
`range::subview/assemble` 不同：range carrier 直接描述一条 binder 的 ISA
modifier，而 Tile region API 描述 parent Tile 与多个 fragment 之间的分区关系。

### `TPARTVIEW`

接口形式：

```cpp
template <typename SubTile, int Rows, int Cols, typename Parent>
auto TPARTVIEW(Parent &parent)
    -> BorrowedTileArray<Parent, SubTile, Rows, Cols>;
```

参数填写规则：

| 参数 | 如何填写 | 含义 |
| --- | --- | --- |
| `parent` | 传入完整 Parent Tile | 被切分的 Tile，不会被复制 |
| `SubTile` | 填写一个 fragment Tile 类型 | 每个分片的物理 shape、valid shape、dtype 和 layout |
| `Rows` | 填分区行数 | Parent 在行方向分成多少个 fragment |
| `Cols` | 填分区列数 | Parent 在列方向分成多少个 fragment |

例如将 `32 x 64` 的 Tile 按列切成四个 `32 x 16` 的 fragment：

```cpp
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

Parent parent;
auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
```

`Rows`、`Cols` 和 `SubTile` 必须在编译期确定。接口会检查分片是否完整覆盖
Parent：

```text
Parent::Rows = Rows * SubTile::Rows
Parent::Cols = Cols * SubTile::Cols
Parent::LogicalTileBytes = Rows * Cols * SubTile::LogicalTileBytes
```

还会检查 dtype、location、layout、storage layout 和 valid region 是否匹配。
例如，`32 x 64` 不能用四个 `32 x 20` 的 fragment 覆盖，因为总列数变成了
`80`。

### `BorrowedTileArray` / `SubTileView`

`TPARTVIEW` 返回 `BorrowedTileArray<Parent, SubTile, Rows, Cols>`。它只保存
Parent 的借用关系，不拥有独立 Tile 存储：

```cpp
auto parts = TPARTVIEW<Fragment, 1, 4>(parent);

auto first = parts[0][0];
auto third = parts[0][2];
```

`parts[row][col]` 的返回类型是：

```cpp
SubTileView<Parent, Fragment>
```

它表示 Parent 中第 `row` 行、第 `col` 列的 fragment。也可以使用命名的行访问：

```cpp
auto row = parts.row(0);
auto fragment = row[1];
```

常用查询接口：

```cpp
int row() const;
int col() const;
int GetValidRow() const;
int GetValidCol() const;
uintptr_t GetRangeBase() const;
Parent &parent() const;
```

`GetRangeBase()` 返回 fragment 相对于 Parent 起始位置的 byte offset，不是新的
GM 地址。当前 inline-asm 路径在 fragment 被 TileOP 消费时，将这个 offset 放入
范围基地址寄存器，并在 source binder 后生成 `B.SUBVIEW`。例如：

```cpp
auto fragment = parts[0][2];
TMULS(result, fragment, scale);
```

逻辑上会产生：

```asm
B.IOT <parent-source>, ...
B.SUBVIEW 0, r2, 0, <Fragment::TilesizeCode>
```

其中 `r2` 携带 fragment 的运行时 offset。`SubTileView` 不是普通拥有存储的 Tile，
不要对它调用需要独立 Tile 存储的接口，也不要手动释放它。

### `TileArray`

`TileArray` 是组装阶段的 destination 容器：

```cpp
template <typename SubTile, int Rows, int Cols>
class TileArray;
```

创建一个 `1 x 4` 的 fragment 输出数组：

```cpp
TileArray<Fragment, 1, 4> fragments;
```

通过下标取得 destination slot：

```cpp
auto slot0 = fragments[0][0];
auto slot2 = fragments[0][2];
```

也可以先取得某一行：

```cpp
auto row = fragments.row(0);
auto slot = row[1];
```

`TileArray` 的存储容量由下面的关系自动计算：

```text
ParentBytes = SubTile::LogicalTileBytes * Rows * Cols
```

因此 `TileArray` 的分片数量和 fragment 容量必须能形成一个合法的 parent
SizeCode。`TileArray` 不可复制，但可以移动；完成所有 slot 写入后，必须用
`std::move` 传给 `TASSEMBLY`。

### `TileArrayOutputRef`

`TileArrayOutputRef<SubTile>` 是 `TileArray` 返回的 destination proxy：

```cpp
using Slot = TileArrayOutputRef<Fragment>;
Slot slot = fragments[0][0];
```

它不拥有存储，不能单独作为最终 Tile 使用。它的用途是作为写入型 TileOP 的
destination，例如格式转换：

```cpp
InputFragment input;
auto slot = fragments[0][col];
TCVT(slot, input);
```

常用查询接口：

```cpp
int row() const;
int col() const;
int ordinal() const;     // row * Cols + col
int slot_count() const; // Rows * Cols
int GetValidRow() const;
int GetValidCol() const;
```

`ordinal()` 表示 slot 在 assembly session 中的顺序。当前 inline-asm 实现根据
slot 顺序生成 `B.ASSEMBLE` 的 `INIT`、`MIDDLE` 和 `LAST` 形式，因此 slot 应按
从 `0` 到 `slot_count() - 1` 的顺序写入。

### `TASSEMBLY`

接口形式：

```cpp
template <typename Parent, typename SubTile, int Rows, int Cols>
auto TASSEMBLY(TileArray<SubTile, Rows, Cols> &&array) -> Parent;
```

典型用法：

```cpp
using Parent = TileLeft<__bf16, 32, 64>;
using Fragment = TileLeft<__bf16, 32, 16>;

TileArray<Fragment, 1, 4> fragments;

for (int col = 0; col < 4; ++col) {
  auto slot = fragments[0][col];
  TCVT(slot, input[col]);
}

Parent result = TASSEMBLY<Parent>(std::move(fragments));
```

`Parent` 是最终返回的完整 Tile；`SubTile`、`Rows` 和 `Cols` 必须与
`TileArray` 一致。调用前应完成所有 slot 的写入，并保证：

```text
Parent 的物理 shape = Rows x Cols 个 SubTile 的物理 shape
Parent 的容量       = Rows x Cols 个 SubTile 的容量
```

`TASSEMBLY` 本身负责构造并返回完整 Parent，不需要再手写独立的
`B.ASSEMBLE`。在当前 inline-asm 路径中，写入 `TileArrayOutputRef` 的 TileOP
（例如 `TCVT`）负责生成对应的 `B.ASSEMBLE`。

### 两套接口的选择

```text
给一次 source TileOP 附加范围：range::subview
给一次 destination TileOP 附加组装范围：range::assemble
把一个 parent 切成多个 source fragment：TPARTVIEW
把多个 destination fragment 合成 parent：TileArray + TASSEMBLY
```

不要把 `TPARTVIEW` 得到的 `SubTileView` 再包装成 `range::Subview`，也不要把
`TileArrayOutputRef` 当作普通 Tile 使用。它们已经分别是 region source view 和
region destination slot。

## 生成代码检查

开发新接口或修改 wrapper 时，建议同时检查语法和生成汇编：

```bash
clang++ --target=linx64v5-unknown-linux-musl \
  -mlxbc -fenable-matrix -O2 -std=c++20 \
  -Iinclude -D__linx -S kernel.cpp -o kernel.s

rg 'B\\.SUBVIEW|B\\.ASSEMBLE' kernel.s
```

确认事项：

1. `B.SUBVIEW` 位于 source binder 之后；
2. `B.ASSEMBLE` 位于 destination binder 之后；
3. source/destination 角色没有反用；
4. assembly lifecycle 与 slot 写入顺序一致；
5. `RegSrc` 对应的 runtime base value 已正确绑定。
