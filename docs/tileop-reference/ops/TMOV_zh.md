# TMOV

**布局与重排** · **TLSU** · 0.59 活动目录

## 简介

`TMOV` 复制 Local Tile，或在 Local 和 Shared Tile 之间搬运数据。
源、目标类型决定搬运方向；支持的 CUBE ↔ Shared ND 布局转换也由类型选择。

## 数学语义

在支持的 Tile 存储与布局之间搬运 payload。

## C++ 接口

```cpp
template <is_local_tile_v Tile>
inline void TMOV(Tile &dst, const Tile &src);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、E2M1X2、E1M2X2、HiF4X2、S4X2、U4X2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |

## 约束与默认值

`dst` 和 `src` 必须是同一 Tile 类型（包括 location、dtype、layout、物理
shape 和 capacity），并且 `src` 必须已初始化。普通 `TMOV` 只支持 Local-to-Local，
布局限于 `SLayout::NoneBox` 的 RowMajor、CUBE_M16 和 CUBE_M32；N8、boxed、
ColumnMajor 和 range wrapper 不能调用此同类型重载。CUBE identity copy 显式
发出对应的 `B.DATR`，并将保留的 pad union 编为 Zero（0）；它不要求填充
复制数据中的 padding。RowMajor 保持 NORM 默认；所有分支显式传递物理列数 LB2。
逻辑元素分片另有 borrowed-part 重载，支持匹配 dtype 的 logical32/logical64 part。

### Local 与 Shared 之间的发布、插入及读取

`SharedTile` 的寄存器由编译器分配，在当前函数或内联调用中使用。编译器根据
操作数区分整块发布、按范围组装、整块读取与读取子视图；源和目标的布局决定
是否需要转换。

| 调用 | 用途 |
| --- | --- |
| `TMOV<PEMask>(shared, local)` | 一个 PE 将完整 Local 源发布到 Shared。 |
| `TMOV<PEMask>(writer, local)` | 按 `range::assemble*` 指定的位置组装 Shared；完成后发布。 |
| `TMOV(local, shared)` | 每个参与 PE 读取同一完整 Shared 源。 |
| `TMOV(local, view)` | 每个参与 PE 读取 `range::shared_nd_subview` 指定的范围。 |

完整发布显式指定一个 PE（`1, 2, 4, 8`），其余形式默认使用掩码 `15`。
PE 掩码只决定参与者，读取子区间要显式传入视图。RowMajor 整块搬运要求
源、目标的 dtype、物理列数与容量一致；子区间读取要求 Local 目标容量、
物理列数和有效形状匹配所选视图。RowMajor 组装要求 Shared 目标的有效形状
静态已知，并与各 PE 分片共同形成的有效区一致。Local 源使用与 Shared
目标相同的物理容量和列数，分片从 Local 源的第一行开始，写入范围按整行对齐。

例如，掩码 `12` 的两个 PE 各有一个物理 `8 × 32`、有效 `4 × 32` 的 FP32
Local 源。分别传入 `offset_units=0` 和 `4`，每个 PE 放置四个完整行：

```cpp
using Parent = Tile<Location::Vec, float, 8, 32>;
using WriterTile = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor, 4, 32>;
using Fragment = Tile<Location::Vec, float, 4, 32>;

void assemble_and_extract(Fragment &dst, const WriterTile &src,
                          uintptr_t offset_units) {
  SharedTile<Parent> shared;
  auto writer = range::assemble_init_last<4>(shared, offset_units);
  TMOV<12>(writer, src);
  TMOV<12>(dst, range::shared_nd_subview<4>(shared, offset_units));
}
```

### Local CUBE 到 Shared ND

目标为 RowMajor Shared ND、源为 Local `CUBE_M16/M32` 时，`TMOV` 根据
操作数类型选择布局转换。

```cpp
TMOV<PEMask>(shared_nd, src_cube);

// 多 PE cooperative 写入使用 range::assemble* destination carrier。
TMOV(assemble_writer, cube_fragment);
```

普通 Shared 目标必须显式传入单 bit `PEMask`（`1, 2, 4, 8`），源 capacity 等于完整
Shared parent capacity。多 PE 生产者必须用 `range::assemble`、
`range::assemble_middle`、`range::assemble_last` 描述明确的 128-byte range；
writer SizeCode 必须等于源 CUBE 的 SizeCode，最终 `LAST` 才发布完整 parent。
组装形式的 `PEMask` 默认 `15`。
每个 writer 的有效矩形会按其 `B.ASSEMBLE` 线性 offset 投影到父 RowMajor
几何，并合并为 pending valid rows/columns；`INIT`/`MIDDLE` 不替换此前已发布的
有效区，只有 `LAST` 才将完整 generation 的有效区和 payload 一起发布。因而
物理列为 4、有效列为 3 的整行 fragment 拼接后，父对象有效列仍为 3。

源、目标 backing dtype 必须相同，目标列数必须等于源 CUBE 的精确物理列数，
目标 RowMajor 几何必须完整覆盖其 SizeCode capacity，且 writer range 必须落在
parent 内。RCPE6M2 backing、CUBE_N8，以及在同一 Shared generation 中混用
NORM/M322ND/M162ND 都不合法。编码使用 Function 2 `BSTART.TMOV DTYPE_NONE`、
`B.DATR M322ND/M162ND, DTYPE_NONE, Zero`、Shared destination `B.IOS` 和 Local
source `B.IOT`。

参与同一 cooperative generation 的 PE 必须对有效区使用一致的 SPMD 元数据。
当前 C++ carrier 不对各 PE 私有的动态 valid shape 执行额外标量归约。

### Shared ND 到 Local CUBE

目标为 Local `CUBE_M16/M32`、源为 Shared ND 或其专用子视图时，`TMOV`
根据类型选择反向转换。配套 Linx LLVM 需支持 `ND2M32/ND2M16`。

```cpp
// PEMask 默认 15。完整 parent 形式可由多个 PE 读取同一个完整源；
// 每个参与 PE 都分配一个独立 Local 目标，不按 PE identity 自动切片。
TMOV<PEMask>(dst_cube, shared_nd);

// 长度、立即数偏移、运行时 base 全部以 128B 为单位。
auto view = range::shared_nd_subview<LengthUnits, OffsetUnits>(shared_nd,
                                                              base_units);
TMOV<PEMask>(dst_cube, view);
// 省略 base_units 使用 zero register，可直接传递临时 view。
```

源必须是具有函数内或 inline SSA 生命周期的 compiler-managed RowMajor
`SharedTile`；range view 借用源对象，不能比源存活更久，也不能从临时 Shared
对象构造。普通 `range::subview` 和 `B.ASSEMBLE` 目标不能传给此接口。
目标必须是 unboxed Local `CUBE_M32/M16`，且 backing dtype 与源
完全相同。RCPE6M2、N8 和 M16 b64 不支持；M32 b64 沿用双 CELL 映射。

完整 parent 的物理行数必须恰好为 M=32/16，容量等于目标容量。带范围的形式
必须选择恰好 M 个连续完整物理行，起点在物理行边界，范围落在 parent 内。
起始行不必为 M 的倍数。parent 的物理列数及行跨度保持不变，并等于目标
精确容量导出的物理列数。不能通过减小长度提取每行的一部分列。

接口根据源当前有效区计算 `ValidRow=min(M,max(0,parent.ValidRow-origin_row))`
及 `ValidCol=parent.ValidCol`；两者必须非零。静态目标有效区必须匹配，动态
目标在转换后更新对应元数据。可静态确定的布局、类型、容量和行对齐错误在
编译期拒绝；动态范围越界、未按行对齐、空有效区及目标有效区不匹配在提交
转换前 trap，检查在 `NDEBUG` 下仍有效。源对象的运行时有效区元数据可以被
生产者更新，接口不会把类型声明的初始有效区当作当前发布的有效区。

编码为 Function 2 `BSTART.TMOV DTYPE_NONE`，`B.DATR ND2M32/ND2M16,
DTYPE_NONE, Zero`；`B.DIM LB0/LB1` 显式提供有效列/行，省略 LB2 使用有效值
1 的精确容量推导规则。Shared `B.IOS` 与 Local destination `B.IOT` 使用同一
PE mask，`B.SUBVIEW` 紧跟 Shared binder。`PEMask=0` 不读取源 handle、有效区
或检查范围；非参与 PE 也不访问源或更新目标。

源必须完整发布；硬件按规范等待未就绪的源并处理 generation/lifetime。
转换保留每个有效逻辑元素的原始位与 backing dtype，不转置、不作数值转换，
也不访问 GM。输出有效元素定义明确；padding 写原始零但仍在语义有效区外。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using TileT = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
  TileT src;
  TileT dst;
  TMOV(dst, src);
}
```

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Cube = VecTileM32<uint8_t, 32, 128>;
using ND = SharedTile<Tile<Location::Vec, uint8_t, 32, 128>>;

void roundtrip(Cube &dst, Cube &src) {
  ND shared;
  TMOV<8>(shared, src);
  // 显式 mask=1000 选择 PE0；Shared handle 保持在本函数内。
  TMOV<8>(dst, shared);
}
```

## Bundle 对照

```asm
BSTART.TLSU TMOV, DataType
B.DIM       LB0
B.DIM       (LB1/LB2 for 2D)
B.IOT
BSTOP
```

活动目录编码为 `TLSU`，function `2`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/layout-and-rearrangement/layout/TMOV.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/layout-and-rearrangement/layout/TMOV.asl)

[返回操作索引](../README.md)
