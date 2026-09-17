# B.SUBVIEW / B.ASSEMBLE 使用指南

`B.SUBVIEW` 和 `B.ASSEMBLE` 是两条 block range modifier，分别描述 source
binder 的读取范围和 destination binder 的组装范围。本文是这两个 modifier
在 TileOP C++ 接口上的唯一使用文档：先给出选择接口的决策表，再按
"TileArray 优先 → range factory → 显式 carrier" 的顺序给出每一层的完整
使用示例。指令编码（位域、合法域、match/mask）见 PTO-ISA 0.58.4
ADR-0098 与 `asl/block/operands/` 下的 owning ASL，不在本文重复。

先记住两条核心规则：

```text
B.SUBVIEW  附在 source binder 之后，描述"从 parent 的哪一段读"
B.ASSEMBLE 附在 destination binder 之后，描述"写到 parent 的哪一段、
           是 assembly session 的第几个 slot"
```

两者都不复制数据，只是把"哪个 Tile、哪个基址、哪个范围"打包成传给
TileOP 的对象，由 wrapper 在 binder 后发射 modifier。

## 接口选择决策表

```text
把一个 parent Tile 切成多个 source fragment：      TPARTVIEW（优先）
把多个 destination fragment 组装成一个 parent：    TileArray + TASSEMBLY（优先）
给一次 source TileOP 附加范围（Shared/兼容/测试）：range::subview
给一次 destination TileOP 附加组装范围：           range::assemble
固定 ABI / ISA 编码测试需要指定寄存器：            *_at_reg 专家接口
```

普通业务代码只用前两行。`range::subview/assemble` 服务于 Shared `B.IOS`
source、兼容代码和编码回归测试；`*_at_reg` 接口只出现在固定 ABI 测试中。
不要混用两层：不要把 `TPARTVIEW` 得到的 view 再包成 `range::Subview`，
也不要把 `TileArray` 的 slot 当普通 Tile 使用。

## TileArray + TASSEMBLY：组装一个 parent（推荐）

把多个 destination fragment 按顺序写满一个 `TileArray`，再用
`TASSEMBLY` 取回完整 parent。`TileArray` 拥有 parent 存储；写入每个 slot
的 TileOP 会自动发出对应 `INIT → MIDDLE* → LAST` 生命周期的
`B.ASSEMBLE`；`TASSEMBLY` 本身不发独立指令，只返回组装好的 parent。

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using Parent   = TileLeft<__bf16, 32, 64>;   // 组装结果 [32, 64]
using Fragment = TileLeft<__bf16, 32, 16>;   // 每个分片 [32, 16]
using Input    = TileLeft<__bf16, 32, 16>;
using Result   = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

void assemble_four(Input (&inputs)[4], Parent &out_storage) {
  TileArray<Fragment, 1, 4> destinations;    // 1 行 x 4 列 slot

  for (int col = 0; col < 4; ++col) {
    auto slot = destinations[0][col];        // TileArrayOutputRef
    TCVT(slot, inputs[col]);                 // 每个 slot 的写入自动带 B.ASSEMBLE
  }

  Parent result = TASSEMBLY<Parent>(std::move(destinations));
  (void)result;
}

void source_fragment_example(Parent &parent, Result &out) {
  // source 侧对应物：TPARTVIEW 切片，取一片作为 source_tile 消费
  auto source_tile = TPARTVIEW<Fragment, 1, 4>(parent)[0][2];
  TABS(out, source_tile);
}
```

要点：

- **slot 必须按 ordinal 顺序写入**（`row * Cols + col`，从 0 开始）。第 0 个
  写入的 slot 发出 `INIT` 形式，最后一个发出 `LAST`，中间是 `MIDDLE`；
  乱序写入会被生命周期约束拒绝。
- slot 类型是 `TileArrayOutputRef<Fragment>`，只作为写入型 TileOP 的
  destination，不拥有独立存储，也不能当普通 Tile 传给读取型接口。
- `TileArray` 不可复制；所有 slot 写完后必须 `std::move` 给 `TASSEMBLY`。
- `Parent` 的 shape/容量必须恰等于 `Rows x Cols` 个 `Fragment`：
  `Parent::Rows = Rows * Fragment::Rows`、`Parent::Cols = Cols * Fragment::Cols`。
- fragment 与 parent 的 dtype、location、`BLayout`、storage layout 必须
  一致；每个 fragment 容量是 128 B 的整数倍。

`BorrowedTileArray` / `SubTileView`（`TPARTVIEW` 的返回）与
`TileArrayOutputRef` 的常用查询接口：

```cpp
int row() const;          // fragment 的行号
int col() const;          // fragment 的列号
int ordinal() const;      // row * Cols + col（仅 OutputRef）
int slot_count() const;   // Rows * Cols（仅 OutputRef）
int GetValidRow() const;
int GetValidCol() const;
uintptr_t GetRangeBase() const;  // fragment 相对 parent 的 128B 单位偏移
Parent &parent() const;          // 仅 BorrowedTileArray 的 view
```

## TPARTVIEW：把 parent 切成多个 source fragment（推荐）

`TPARTVIEW<Fragment, Rows, Cols>(parent)` 返回 `BorrowedTileArray`——对
parent 的借用视图，不分配新 Tile 寄存器。取出的每个 fragment 传给读取型
TileOP 时，wrapper 自动在 source binder 后附加 `B.SUBVIEW`：

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using Parent   = CubeTileM32<float, 32, 64>;
using Fragment = CubeTileM32<float, 32, 16>;
using Result   = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

void consume_fragment(Parent &parent, Result &result) {
  auto parts = TPARTVIEW<Fragment, 1, 4>(parent);  // 按列切 4 片
  auto fragment = parts[0][2];                     // 第 0 行第 2 列

  TABS(result, fragment);  // source B.IOT + B.SUBVIEW 0, <gpr>, 0, <code>
}
```

注意：`SubTileView` 满足 region view 约束，可直接喂给 TEPL 逐元素、归约、
CUBE 等接受 region source 的操作；`TSTORE` 等要求普通 Tile 数据流的接口
不在其列。

分片必须完整覆盖 parent：

```text
Parent::Rows  = Rows * Fragment::Rows
Parent::Cols  = Cols * Fragment::Cols
Parent 容量   = Rows * Cols * Fragment::LogicalTileBytes
```

dtype、location、`BLayout`、storage layout 也要一致；`32 x 64` 不能用四个
`32 x 20` 覆盖（总列数 80 ≠ 64）。`Rows/Cols/SubTile` 全部编译期确定，
不匹配在 `static_assert` 阶段报错。

### fragment 的 range 计算

对连续分区，slot 的线性序与偏移是：

```text
ordinal      = row * Cols + col
offset_units = ordinal * (Fragment::LogicalTileBytes / 128)   // 单位 128B
```

当前 inline-asm 把 `offset_units` 放进编译器分配的基址 GPR，
`uimm11` 固定为 0，编码仍遵循 ISA 的 `GPR[RegSrc] + uimm11` 定义；开发者
不需要、也不能指定寄存器编号。**每个 slot 必须用自己的 range**，不能把
所有 slot 固定到 offset 0。

### 支持的 parent 类型

`TPARTVIEW` 当前只接受 Local Matrix+CUBE parent（`CubeTileM16` /
`CubeTileM32`，含 `Location::Vec` CUBE 载体）。Shared CUBE source 可先用
`range::subview` 走 `B.IOS` 低层路径；RowMajor 和 Vec+ND parent 仍会被
编译期拒绝。

## range::subview：单 binder 的 source 范围

适合一次 source TileOP 的范围描述：Shared `B.IOS` source、兼容代码、
固定 ABI 与编码测试。普通 parent/fragment 分区请用 `TPARTVIEW`。

### 基本形式

```cpp
using Src = CubeTileM16<float, 16, 16>;
using GM  = global_tensor<float, RowMajor<16, 16>>;

void store_full(Src &tile, GM &gm) {
  auto view = range::subview(tile);      // 全 Tile、zero 基址
  TSTORE(gm, view);
}

void store_runtime_base(Src &tile, GM &gm, uintptr_t base_units) {
  auto view = range::subview(tile, base_units);   // 编译器分配 GPR
  TSTORE(gm, view);
}
```

对应发射：

```asm
BSTART.TLSU TSTORE, F32
B.DIM <vcol>, 0, ->lb0
B.DIM <vrow>, 0, ->lb1
B.DIM zero, <cols>, ->lb2
B.IOT <src>, mask=1111, last
B.SUBVIEW <SrcSelect>, <zero-or-compiler-gpr>, <OffsetUnits>, <SizeCode>
B.IOR [<dst>,<stride>], []
```

### 长度与偏移

两个模板参数（长度与偏移）都以 **128 B 为单位**，与编码的 `uimm11` 粒度
一致；factory 自动换算成 ISA size code：

```cpp
auto full    = range::subview(tile);           // 默认全 Tile，offset 0
auto sized   = range::subview<4>(tile);        // 4*128B = 512 B，offset 0
auto shifted = range::subview<4, 3>(tile);     // 512 B，offset 3*128B=384B
auto based   = range::subview<4, 3>(tile, base_units);  // 运行时基址 + 384B
```

长度（128B 单位）→ `SubviewSizeCode` 换算表（SUBVIEW 与 INIT ASSEMBLE
共用）：

| LengthUnits | 字节数 | SizeCode | | LengthUnits | 字节数 | SizeCode |
| ---: | ---: | ---: | --- | ---: | ---: | ---: |
| 1 | 128 B | 1 | | 128 | 16 KB | 8 |
| 2 | 256 B | 2 | | 256 | 32 KB | 9 |
| 4 | 512 B | 3 | | 512 | 64 KB | 10 |
| 8 | 1 KB | 4 | | 1024 | 128 KB | 11 |
| 16 | 2 KB | 5 | | 2048 | 256 KB | 12 |
| 32 | 4 KB | 6 | | | | |
| 64 | 8 KB | 7 | | | | |

省略长度（`0`）时使用 parent 的 `LogicalTileBytes`。长度必须是表中容量且
不超过 parent 容量，否则编译期 `static_assert`。

### base_units 与 OffsetUnits 的区别

```text
最终范围字节地址 = (GPR[RegSrc] + OffsetUnits) * 128B
```

- `base_units`：可选的**运行时**基址（128B 单位）。不填 → `zero` 寄存器；
  填 → 编译器分配一个 GPR 并写入该值。寄存器编号不暴露给开发者。
- `OffsetUnits`：**编译期**的 `uimm11` 立即数（`0..2047`），单位 128B。

## range::assemble：单 binder 的 destination 范围

与 `subview` 同构，但描述 destination。用于 Shared destination、兼容代码
和测试；普通组装用 `TileArray + TASSEMBLY`。

### 生命周期 helper

`B.ASSEMBLE` 的生命周期用命名 helper 表达，不要手写布尔模板参数：

```cpp
auto only   = range::assemble_init_last(tile, base_units); // INIT=1, LAST=1（单 fragment）
auto first  = range::assemble(tile, base_units);           // INIT=1, LAST=0（session 开始）
auto middle = range::assemble_middle(tile, base_units);    // INIT=0, LAST=0
auto last   = range::assemble_last(tile, base_units);      // INIT=0, LAST=1（session 结束）
```

按 fragment 数量选择：

```text
一个 fragment：  assemble_init_last
两个 fragment：  assemble → assemble_last
三个及以上：     assemble → assemble_middle* → assemble_last
```

第 5 字段是 **WriterSizeCode**，不是父 Tile 容量。INIT、MIDDLE、LAST
都记录当前 slot 写入的 fragment 容量；父容量只由 INIT destination 的
`TilesizeCode` 与 assembly session 绑定。例如
`TileArray<Fragment, 1, 4>` 且每个 `Fragment::LogicalTileBytes == 512 B`
时，四个 slot 的第 5 字段都应为 3，父 Tile 是 2 KB，offset 分别为
0、4、8、12 个 128 B CELL。LLVM 暂时接受 WriterSizeCode 0，以兼容旧
producer；新 API 不生成 0。

```cpp
using Dst = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM  = global_tensor<float, RowMajor<4, 8>>;

void load_one(GM &gm, Dst &tile, uintptr_t base_units) {
  auto only = range::assemble_init_last(tile, base_units);
  TLOAD(only, gm);   // B.ASSEMBLE 1, 1, <compiler-gpr>, <off>, <size>
}

void load_head(GM &gm, Dst &tile) {
  auto first = range::assemble(tile);
  TLOAD(first, gm);  // B.ASSEMBLE 1, 0, zero, 0, <size>
}
```

### INIT slot 与后续 slot 的接口约束（编译期强制）

一个 assembly session 的**第一个（INIT）slot 必须由普通分配型接口写入**
（`TLOAD`/`TLOAD_CUBE`/`TMOV_L2S_*` 等），**后续 MIDDLE/LAST slot 必须用
`_ASS` 接口追加**。`static_assert` 强制：

- 普通 `TLOAD` 收到非 INIT carrier（`assemble_middle`/`assemble_last`）→ 拒绝；
- `_ASS` 接口收到 INIT carrier（`range::assemble` 默认形式）→ 拒绝。

```cpp
using Src    = Tile<Location::Vec, float, 16, 16>;
using Parent = Tile<Location::Vec, float, 16, 1, BLayout::RowMajor, 16, 1>;
using GM     = global_tensor<float, RowMajor<16, 16>>;

void session(GM &gm_lo, GM &gm_hi, Src &a, Src &b) {
  Parent parent;
  auto d0 = range::assemble(parent);        // INIT slot
  TLOAD(d0, gm_lo);                         // 普通 TLOAD 打开 session

  auto d1 = range::assemble_middle(parent); // MIDDLE slot
  TADD_ASS(d1, a, b);                       // _ASS 追加

  auto d2 = range::assemble_last(parent);   // LAST slot
  TLOAD_ASS(d2, gm_hi);                     // _ASS 收尾
}
```

### 完整会话示例（多 fragment 拼装一个 parent）

```cpp
using Dst = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using GM  = global_tensor<float, RowMajor<4, 8>>;

void load_three(GM &gm0, GM &gm1, GM &gm2, Dst &tile) {
  auto first  = range::assemble(tile);            // INIT
  TLOAD(first, gm0);

  auto middle = range::assemble_middle(tile);     // MIDDLE
  TLOAD_ASS(middle, gm1);                        // 非 INIT slot 用 _ASS

  auto last   = range::assemble_last(tile);       // LAST
  TLOAD_ASS(last, gm2);
}
```

## `*_ASS`：TEPL 计算直接写入 assembled destination

TEPL 逐元素、归约、广播、比较接口提供 `_ASS` 后缀版本：destination 是
`range::assemble(...)` 返回的 carrier，参数顺序统一为
"assembled destination 在前、输入在后"。`_ASS` 把已关联的 destination 作为
input-only binder 消费，不分配新 Tile generation。

```cpp
using TileT = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

void fused(TileT &a, TileT &b, TileT &c, TileT &result) {
  auto dst = range::assemble_last(result); // 已由普通 producer 打开的 LAST slot
  TFMA_ASS(dst, a, b, c);
}
```

归约 `_ASS`（`TROWSUM_ASS` 等）在 destination binder 后同样发出
destination-only `B.ASSEMBLE`（携带 carrier 的 INIT/LAST/RegSrc/Offset/
WriterSizeCode），并且不要求 assembled destination 的物理 shape 等于逻辑归约
结果——例如 `TROWSUM_ASS` 的输入逻辑结果是 `R x 1`，destination 可以是
一个更大 parent 的 fragment carrier。

### 会话用法：归约结果拼进 parent

```cpp
using Src  = Tile<Location::Vec, float, 16, 16>;
using Slot = Tile<Location::Vec, float, 16, 1, BLayout::RowMajor, 16, 1>;

void tree_reduce(Src &src, Slot &parent) {
  auto init = range::assemble_init_last(parent);
  TROWSUM(init, src);                       // INIT：普通 producer 打开 session

  auto last = range::assemble_last(parent);
  TROWSUM_ASS(last, src);                   // LAST：_ASS 消费，发 B.ASSEMBLE 0,1,...
}
```

### 接口清单

| 类别 | 接口 |
| --- | --- |
| tile/tile | `TADD_ASS`、`TSUB_ASS`、`TMUL_ASS`、`TDIV_ASS`、`TREM_ASS`、`TAND_ASS`、`TOR_ASS`、`TXOR_ASS`、`TSHL_ASS`、`TSHR_ASS`、`TMAX_ASS`、`TMIN_ASS` |
| 一元 | `TABS_ASS`、`TNOT_ASS`、`TNEG_ASS`、`TEXP_ASS`、`TLOG_ASS`、`TRECIP_ASS` |
| tile/scalar | `TADDS_ASS`、`TSUBS_ASS`、`TMULS_ASS`、`TDIVS_ASS`、`TREMS_ASS`、`TANDS_ASS`、`TORS_ASS`、`TXORS_ASS`、`TSHLS_ASS`、`TSHRS_ASS`、`TMAXS_ASS`、`TMINS_ASS` |
| 比较 | `TCMP_ASS<Mode>`（省略 `Mode` 时为 `EQ`）、`TCMPS_ASS<Mode>` |
| 三元/转换/特殊 | `TFMA_ASS`、`TCVT_ASS`、`TSQRT_ASS`、`TRSQRT_ASS`、`TRELU_ASS`、`TTRANS_ASS` |
| 归约 | `TROWSUM_ASS`、`TROWMAX_ASS`、`TROWMIN_ASS`、`TROWPROD_ASS`、`TCOLSUM_ASS`、`TCOLMAX_ASS`、`TCOLMIN_ASS`、`TCOLPROD_ASS` |
| 归约取序 | `TROWARGMAX_ASS`、`TROWARGMIN_ASS`、`TCOLARGMAX_ASS`、`TCOLARGMIN_ASS` |
| 广播 | `TROWEXPAND_ASS`、`TCOLEXPAND_ASS` |
| 广播复合 | `TROWEXPAND{ADD,SUB,MUL,DIV,MAX,MIN,EXPDIF}_ASS`、`TCOLEXPAND{ADD,SUB,MUL,DIV,MAX,MIN,EXPDIF}_ASS` |

dtype 规则：`TCMP_ASS` 两个输入 dtype 必须相同；`TCVT_ASS` 允许转换 dtype
但物理 shape 必须相同；其余接口 source 与 destination dtype 匹配。

`_ASS` 生成的共同 bundle 结构（opcode 与额外 modifier 由操作决定）：

```asm
BSTART.TEPL <opcode>, <source-dtype>
B.DIM      <valid-col>, 0, ->lb0
B.DIM      <valid-row>, 0, ->lb1
B.DIM      zero, <physical-cols>, ->lb2
B.IOT      <source-operands>, mask=1111
B.IOT      <assembled-destination>, mask=1111, last
B.ASSEMBLE <INIT>, <LAST>, <RegSrc>, <OffsetUnits>, <WriterSizeCode>
```

## 显式 carrier 类型（专家）

factory 无法表达的特殊 descriptor 组合可以直接实例化 carrier：

```cpp
using SrcTile = CubeTileM16<float, 16, 16>;
using OutGM   = global_tensor<float, RowMajor<16, 16>>;
using DstTile = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using InGM    = global_tensor<float, RowMajor<4, 8>>;

void explicit_carriers(OutGM &out_gm, InGM &in_gm,
                       SrcTile &cube_tile, DstTile &row_tile,
                       uintptr_t base_units) {
  // Subview：CUBE parent + TSTORE 消费
  range::Subview<SrcTile, 1, 0, 0> source_view(cube_tile, base_units);
  TSTORE(out_gm, source_view);

  // Assemble：RowMajor parent + TLOAD 消费
  range::Assemble<DstTile, 1, true, false, 0, 0> destination_view(row_tile,
                                                                  base_units);
  TLOAD(destination_view, in_gm);
}
```

普通 kernel 优先使用 factory 和生命周期 helper；显式 carrier 适合固定 ABI、
边界测试和需要在类型系统暴露完整 descriptor 的代码。

### 固定 ABI 的寄存器选择（仅测试）

高层接口不暴露寄存器编号。需要固定 `RegSrc` 的 ISA 编码测试使用 `*_reg`
专家接口：

```cpp
using TileT = CubeTileM32<float, 32, 64>;   // 8 KiB parent，支持到 code 6
TileT tile;
uintptr_t base_units = 0;

auto source      = range::subview_at_reg<3, 23>(tile, base_units);
auto sized       = range::subview_sized_at_reg<6, 3, 23>(tile, base_units);
auto destination = range::assemble_at_reg<3, 23>(tile, base_units);
auto last        = range::assemble_last_at_reg<3, 23>(tile, base_units);
```

## Shared Tile 范围

`B.SUBVIEW` 允许 Shared source（0.58.5+ 逐 PE range-base 语义）：`SharedTile`
source 走 `B.IOS` 后紧跟 `B.SUBVIEW`，且 wrapped parent 必须保持 CUBE
layout。`B.ASSEMBLE` 也可通过 `B.IOS` 绑定 Shared destination：

```cpp
using Local  = Tile<Location::Vec, float, 4, 8, BLayout::RowMajor>;
using Shared = SharedTile<Local>;
using GM     = global_tensor<float, RowMajor<4, 8>>;

void shared_destination(GM &gm, Shared &shared, uintptr_t base_units) {
  auto destination = range::assemble(shared, base_units);
  TLOAD(destination, gm);   // B.IOS ... / B.ASSEMBLE 1, 0, <gpr>, 0, <size>
}
```

Shared 尺寸遵循 Shared `B.IOS` 合同（`128 B..256 KiB`，SizeCode `1..12`）。
`TPARTVIEW` 目前仅 Local；Shared source 分区继续用 `range::subview`。

## 编译期拒绝清单

以下写法在模板实例化阶段被拒绝（每行均为独立反例，不能出现在可编译
代码中，此处以注释形式列出）：

```cpp
// range::Subview<Src, 0>(tile, 0);        // SizeCode 0 保留
// range::Subview<Src, 13>(tile, 0);       // 13..15 保留
// range::Subview<Src, 1, 2048>(tile, 0);  // OffsetUnits > 2047
// range::Subview<Src, 1, 0, 25>(tile, 0); // RegSrc 超出 0..23
//
// range::Assemble<Dst, 0, true>(tile, 0);   // 仅用于兼容旧 producer
// range::Assemble<Dst, 12, false>(tile, 0); // 合法：continuation 也携带 writer size
// range::Assemble<Dst, 13, true>(tile, 0);  // WriterSizeCode 13..15 保留
//
// TROWSUM_ASS(range::assemble(parent), src); // _ASS 收 INIT carrier
// TLOAD(range::assemble_last(parent), gm);   // 普通接口收非 INIT carrier
//
// TSTORE(gm, range::assemble(tile));   // 角色反用：Assemble 不能当 source
// TLOAD(range::subview(tile), gm);     // 角色反用：Subview 不能当 destination
```

同一批非法编码在 LLVM 汇编器/反汇编器侧同样 fail closed
（`v5-subview-assemble{-neg,-encoding}.s`）。range modifier 不改变被包装
Tile 的 dtype、layout、shape、valid region 或 PE mask；这些不满足时由对应
TileOP 的约束报错。

## 开发自检

修改 wrapper 或排查生成代码时：

```bash
clang++ --target=linx64v5-unknown-linux-musl \
  -mlxbc -fenable-matrix -O2 -std=c++20 \
  -Iinclude -D__linx -S kernel.cpp -o kernel.s

grep -E 'B\.SUBVIEW|B\.ASSEMBLE' kernel.s
```

确认五点：

1. `B.SUBVIEW` 紧跟 source binder 之后；
2. `B.ASSEMBLE` 紧跟 destination binder 之后；
3. source/destination 角色没有反用；
4. assembly lifecycle（INIT/MIDDLE/LAST）与 slot 写入顺序一致；
5. 运行时 base value 已绑定到编译器分配的 GPR。
