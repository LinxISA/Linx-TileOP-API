# TLOAD

**GM 搬运与索引访存** · **TLSU** · 0.59 活动目录

## 简介

`TLOAD` 加载一个普通的 Local 或 Shared 矩形，或将 GM 数据转换为持久的 Local CUBE 存储。

## 数学语义

从 GM 搬入 Local 或 Shared，可随重载转换布局。

## C++ 接口

```cpp
// 普通矩形加载；CUBE Tile 会自动选择 ND→CELL 布局转换路径（见下）。
template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
requires(!tile_shape::IsCubeLayout) void TLOAD(tile_shape &dst, gm_shape &src);

template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
PTO_SHARED_INLINE SharedTile<shp> TLOAD(const gm_shape &src);
template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
PTO_SHARED_INLINE void TLOAD(SharedTile<shp> &dst, const gm_shape &src);
template <is_tile_data_v cube_shape, is_global_data_v gm_shape>
requires(cube_shape::IsCubeLayout) void TLOAD(cube_shape &dst, gm_shape &src);

// Convolution weight conversion: GM OHWI/OIHW -> Shared row-major [N][K].
template <WeightLayoutEnum WeightLayout, int PEMask = 1,
          is_tile_data_v shp, is_global_data_v gm_shape>
PTO_SHARED_INLINE void TLOAD(
    SharedTile<shp> &dst, const gm_shape &src, WeightTLOADParams params);
```

### Convolution weight 到 Shared NK

显式指定 `OHWI2NK` 或 `OIHW2NK` 时，三参数 `TLOAD` overload 将 GM 中的卷积权重
转换为已有的 row-major Shared `[N][K]` 视图。该形式复用 `BSTART.TLOAD`，不会改变
普通矩形 `TLOAD`、`TLOAD_ASS` 或 TMATMUL 的语义。

支持的布局码只有：

| C++ 枚举 | B.DATR layout | GM 源布局 | Shared 目标视图 |
| --- | ---: | --- | --- |
| `OHWI2NK` | `10` | `[Cout][KernelH][KernelW][Cin]` | `[N][K]` |
| `OIHW2NK` | `11` | `[Cout][Cin][KernelH][KernelW]` | `[N][K]` |

`OHWI2KN` 和 `OIHW2KN` 尚未分配，因此不属于公开 API。目标必须是非 boxed、非
CUBE 的 row-major `SharedTile`，且 Shared payload 容量为 `128 B..256 KiB`
（`SizeCode=1..12`）。

#### 权重参数

使用 `make_weight_tload_params` 构造 `ShapeGPR` 和 `StartGPR`：

```cpp
constexpr WeightTLOADParams make_weight_tload_params(
    uint16_t cin, uint16_t cout, uint8_t kernel_h, uint8_t kernel_w,
    uint32_t n_start = 0, uint32_t k_start = 0);
```

字段编码如下：

| 字段 | 位段 | 说明 |
| --- | --- | --- |
| `Cin` | `ShapeGPR[15:0]` | 输入通道数，非零 |
| `Cout` | `ShapeGPR[31:16]` | 输出通道数，非零 |
| `KernelH` | `ShapeGPR[39:32]` | kernel 高度，`1..255` |
| `KernelW` | `ShapeGPR[47:40]` | kernel 宽度，`1..255` |
| `NStart` | `StartGPR[31:0]` | 输出通道窗口起始行 |
| `KStart` | `StartGPR[63:32]` | K 窗口起始位置 |

`ShapeGPR[63:48]` 保留并编码为零。`LB0`、`LB1`、`LB2` 在该模式下分别表示
`ValidK`、`ValidN` 和 `TotalK`；当前 C++ overload 从 Shared 目标的物理列数提供
`TotalK`。`ValidK`/`ValidN` 来自目标 Shared view 的运行时有效区域。

OHWI 与 OIHW 都投影到相同的 canonical K 顺序：`[kh][kw][c1][c0]`，其中 `c0`
最快。`Cin` 不足一个 `c0` 时，补齐的通道 lane 定义为 raw zero，且不会访问 GM。
`KStart` 和 `ValidK` 必须按所选 dtype 的 `C0` 对齐；`NStart` 和 `ValidN` 选择
`Cout` 行窗口，不要求 `C0` 对齐。该首版接口不支持 batch 大于一、grouped/depthwise
convolution、Local/CUBE 目标或新的 assembly/publication 协议。

#### PE participation 与 Shared generation

该 overload 默认 `PEMask=1`，表示单 PE 发布完整 Shared generation；不带
`B.ASSEMBLE` carrier 时只能使用单 bit mask。公开的非零 PE mask 为
`1, 2, 4, 8, 12, 14, 15`。多 PE 分片需要由规范定义的 `B.ASSEMBLE` range
carrier 负责显式范围和最终 LAST publication，不能仅将 `PEMask` 改成多 bit
值来替代 assemble。

共享 generation metadata 的字段编码可通过
`encode_weight_tload_generation_metadata` 生成；它是 metadata 编码辅助函数，
不会自动创建或发布 assemble carrier。

示例：

```cpp
using WeightLocal = Tile<Location::Right, float, 16, 128,
                         BLayout::RowMajor, -1, -1>;
using WeightShared = SharedTile<WeightLocal>;
using WeightGM = global_tensor<float, RowMajor<16, 16>>;

void load_weights(WeightShared &dst, const WeightGM &src) {
  auto params = make_weight_tload_params(
      /*Cin=*/16, /*Cout=*/16, /*KernelH=*/1, /*KernelW=*/1);
  TLOAD<OHWI2NK>(dst, src, params);
}
```

该调用生成一个权重模式 bundle：`B.DATR layout=10, DTYPE_NONE, Zero`，
`LB0=ValidK`、`LB1=ValidN`、`LB2=TotalK`，一个包含 `GMBase/ShapeGPR/StartGPR`
的 `B.IOR`，以及一个 Shared `B.IOS` destination。`DTYPE_NONE` 表示继承
`BSTART.TLOAD` 的源数据类型；其它 DATR 字段固定为 `EQ`、默认舍入、禁用饱和和
禁用 canonicalization。

### Associated destination：`TLOAD_ASS`

```cpp
template <is_local_tile_v tile_shape, is_global_data_v gm_shape>
requires(!tile_shape::IsCubeLayout)
void TLOAD_ASS(tile_shape &dst, const gm_shape &src);

template <is_tile_data_v shp, is_global_data_v gm_shape>
PTO_SHARED_INLINE void TLOAD_ASS(SharedTile<shp> &dst, const gm_shape &src);

template <typename Parent, unsigned WriterSizeCode, bool INIT, bool LAST,
          unsigned OffsetUnits, unsigned RegSrc, is_global_data_v gm_shape>
requires(is_local_tile_v<Parent> || is_shared_tile_v<Parent>)
void TLOAD_ASS(
    range::Assemble<Parent, WriterSizeCode, INIT, LAST, OffsetUnits, RegSrc> &dst,
    const gm_shape &src);

// CUBE destination 由统一入口自动选择布局转换路径。
template <is_tile_data_v cube_shape, is_global_data_v gm_shape>
requires(cube_shape::IsCubeLayout)
void TLOAD_ASS(cube_shape &dst, const gm_shape &src);
```

这些重载把 GM 数据加载到已经关联的 Local、Shared 或 Local CUBE destination，
不会分配新的 destination generation。与 TEPL `_ASS` 一样，TLSU load 也采用
destination-first 的 `(dst, src)` 顺序。`range::Assemble` 在这里是现有
parent register/handle 的 C++ view；load bundle 发出 destination-only
`B.ASSEMBLE`，其中第 5 字段是当前 slot 的 `WriterSizeCode`，
INIT/MIDDLE/LAST 都携带相同含义。
普通 Local/Shared load 的容量必须为 `128 B..256 KiB`；CUBE load 还要求 GM 与
CUBE dtype 相同、CELL storage 不超过 Local capacity，并遵循下方 CUBE layout
转换约束。

**Associated destination 的前提是 handle/register 已经由 producer 建立**。
第一次落到一个 assembly range 的 load 必须使用普通 `TLOAD`
分配 destination generation，后续 slot 才能用 `_ASS`
形式追加。不要把默认构造的 `SharedTile` 直接传给 `TLOAD_ASS`：其 handle
尚未初始化，也没有可消费的关联关系（`_ASS` 的 `"Sr"` 输入只接受已建立
Shared 关联的 handle）。正确形态：

```cpp
void shared_tload_ass(Out &out, const GM &src) {
  Shared shared;
  TLOAD(shared, src);       // producer：建立 Shared handle（"=Sr" 输出）
  TLOAD_ASS(shared, src);   // 追加：源侧 "Sr" 消费同一 handle
  TMOV(out, shared);
}
```

Local parent 的等价形态是先对 `range::assemble<...>(parent)` 的第一个 slot
用普通 `TLOAD`，后续 slot 用 `TLOAD_ASS` + `range::assemble_last`/`assemble_middle`。

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、E2M1X2、E1M2X2、HiF4X2、S4X2、U4X2、S64、S32、S16、S8、U64、U32、U16、U8类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |

### 重载选择

这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。

## 约束与默认值

`TLOAD/TSTORE` 的 GM row stride in **bytes**，而不是元素数；普通 Tile 与 CUBE Tile 分别走普通传输和布局转换路径。source/destination 的顺序不可互换。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 传输矩形由 Tile 的有效区域和 GM layout 共同限定；不要把物理 padding 当作需要传输的逻辑元素。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### GM 布局与 stride

| 场景 | 传入 `global_tensor` 的值 | TLSU 使用的值 |
| --- | --- | --- |
| 连续 RowMajor | 使用连续构造形式，无需手写 stride | 相邻行的实际字节间距。 |
| 带 pitch 的子矩阵 | 构造器仍按**元素 stride**接收行跨度 | wrapper 在 `B.IOR.RegSrc1` 中传递换算后的**字节 stride**。 |
| range / subview | base address 与 byte offset 分别传递 | 最终地址为 base 加操作的 range offset。 |

普通 Tile 使用常规 TLSU 传输；CUBE Tile 由统一 `TLOAD/TSTORE` 自动选择布局转换。

### CUBE load contract

对 CUBE Tile 使用 `TLOAD(dst, src)` 时，TileOP 根据目标类型选择 ND→CELL 布局转换，
`dst` 必须是 Local CUBE Tile，`src` 必须是相同 dtype 的 `global_tensor`。
Local CUBE capacity 必须在 `128 B..256 KiB` 范围内，且足以容纳该 CUBE
layout 的逻辑 tile。`LB0/LB1` 传递 runtime valid columns/rows，`LB2` 不参与
CUBE load；valid shape 不能超过 physical shape。`TLOAD` 和 `TLOAD_ASS` 的
Local CUBE 重载对物理 CELL 尾部使用 `Zero` padding。

`CubeM16`、`CubeM32` 和 `CubeN8` 分别使用 `ND2M16`、`ND2M32` 和 `ND2N8`；
对应 store selector 是 `M162ND`、`M322ND` 和 `N82ND`。不能在这些 layout
之间复用错误的转换选择器。

### 显式选择 CUBE 尾部填充

只有算法需要非默认 padding 时，才需使用 0.59 的专用调用形式。`Pad` 可取
`TmaPadValue::Max`、`TmaPadValue::Min` 或 `TmaPadValue::Null`：

```cpp
TLOAD_CUBE<TmaPadValue::Min>(dst, src);
TLOAD_CUBE_ASS<TmaPadValue::Min>(associated_dst, src);
```

### Vector CUBE layout

Vector location 也支持持久化的 CUBE cell layout。推荐使用专用别名表达
layout 意图：

| 别名 | layout | `TLOAD` 转换选择器 | 适用的最大逻辑行数 |
| --- | --- | --- | --- |
| `VecTileM16<T, R, C>` | `BLayout::CubeM16` | `ND2M16` | 16 |
| `VecTileM32<T, R, C>` | `BLayout::CubeM32` | `ND2M32` | 32 |

这两个别名等价于 `Tile<Location::Vec, T, R, C, BLayout::CubeM16>` 或
`BLayout::CubeM32`。它们使用 CUBE 的持久化 cell 存储，而不是普通
`RowMajor` Vector Tile；对应的 `TSTORE` 选择器分别是 `M162ND` 和
`M322ND`。加载和存储必须使用相同的 CUBE layout，不能在 M16 与 M32
之间混用。

```cpp
using VecM16 = VecTileM16<float, 16, 32>;
using VecM32 = VecTileM32<float, 32, 32>;
using GM16 = global_tensor<float, RowMajor<16, 32>>;
using GM32 = global_tensor<float, RowMajor<32, 32>>;

void load_vector_cubes(float *data16, float *data32,
                       VecM16 &m16, VecM32 &m32) {
  GM16 src16(data16);
  GM32 src32(data32);
  TLOAD(m16, src16);  // ND2M16
  TLOAD(m32, src32);  // ND2M32
}
```

### 默认值

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `ValidRow`/`ValidCol` 为运行期（`DYNAMIC`）时，对应 `B.DIM` 使用寄存器源形式（`B.DIM <gpr>, 0, ->lbN`，RegSrc 为绝对 GPR 0..23），不使用立即数/压缩形式；仅静态维度使用 `C.B.DIMI`/立即数形式（ADR-BLOCK-0012 Decision 013/014）。每个 `LB0/LB1/LB2` 只写一次。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using GM = global_tensor<float, RowMajor<32, 32>>;
  using TileT = Tile<Location::Vec, float, 32, 32>;
  float data[32 * 32] = {};
  GM src(data);
  TileT dst;
  TLOAD(dst, src);
}
```

## Bundle 对照

```asm
BSTART.TLSU TLOAD, FP16
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1
B.DIM       rCol, 0, ->LB2
B.IOT       mask=1111, last, ->T<4KB>
B.IOR       a0, a1, 0
```

活动目录编码为 `TLSU`，function `0`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/memory-and-data-movement/regular/TLOAD.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/memory-and-data-movement/regular/TLOAD.asl)

[返回操作索引](../README.md)
