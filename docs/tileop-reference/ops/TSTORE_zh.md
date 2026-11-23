# TSTORE

**GM 搬运与索引访存** · **TLSU** · 0.59 活动目录

## 简介

`TSTORE` 将一个有效的 Local、Shared 或 CUBE 矩形存储到 GM，且不修改源 Tile。
对 Local CUBE Tile，`TSTORE` 自动选择 CELL→GM 布局转换；GM 与 CUBE dtype
必须相同，CUBE capacity 必须在 `128 B..256 KiB` 范围内，并且只写入 source
的 valid rows/columns。store padding 固定为 `Null`。

## 数学语义

将 Tile 写回 GM，可随重载转换布局或执行 reduction。

## C++ 接口

```cpp
template <is_global_data_v gm_shape, is_tile_data_v tile_shape>
requires(!tile_shape::IsCubeLayout) void TSTORE(gm_shape &dst, tile_shape &src);
// CUBE Tile 自动选择 CELL→GM 布局转换路径。
template <is_global_data_v gm_shape, is_tile_data_v cube_shape>
requires(cube_shape::IsCubeLayout) void TSTORE(gm_shape &dst, const cube_shape &src);
template <is_global_data_v gm_shape, is_shared_tile_v SharedTileT>
PTO_SHARED_INLINE void TSTORE(gm_shape &dst, const SharedTileT &src);
```

### 规则地址 Atomic TSTORE

规则地址原子归约使用下面八个无返回值接口。C++ 名称中的下划线对应规范的
`TSTORE.ATOMIC_*` 虚拟别名；它们仍编码为 TLSU Function 1 `TSTORE`，通过
`B.DATR Canonicalize=1` 和 `CMode=0..7` 选择操作，不增加新的 TileOp 或
TLSU Function。

```cpp
TSTORE_ATOMIC_ADD(dst, src);
TSTORE_ATOMIC_MIN(dst, src);
TSTORE_ATOMIC_MAX(dst, src);
TSTORE_ATOMIC_AND(dst, src);
TSTORE_ATOMIC_OR(dst, src);
TSTORE_ATOMIC_XOR(dst, src);
TSTORE_ATOMIC_INC(dst, src);
TSTORE_ATOMIC_DEC(dst, src);
```

所有八个接口都可用模板参数选择参与 PE：

```cpp
TSTORE_ATOMIC_ADD<12>(dst, src); // PE mask 1100
TSTORE_ATOMIC_ADD<0>(dst, src);  // 架构严格空操作
```

`PEMask` 可为 `0, 1, 2, 4, 8, 12, 14, 15`。Local `CUBE_M16/M32`
还可以使用 Predicate Tile execution mask；第二个模板参数控制反选：

```cpp
TSTORE_ATOMIC_ADD<15, false>(dst, local_cube, predicate);
TSTORE_ATOMIC_ADD<15, true>(dst, local_cube, predicate); // PredInv=1
```

Predicate 的 CUBE layout 和 valid shape 必须与 Local 源一致。该重载固定
`Zero=0`，不提供 zero-fill 形式。RowMajor Local 和 Shared 源不接受 Predicate
参数；Shared Atomic TSTORE 仅支持 PE participation mask。

`src` 可为 Local RowMajor、Shared RowMajor，或 Local `CUBE_M16/M32`；不接受
`CUBE_N8`。GM 与源 dtype 必须相同。支持的 dtype 如下：

| 操作 | dtype |
| --- | --- |
| `ADD` | FP16、BF16、FP32、FP64、S32、U32、U64 |
| `MIN` / `MAX` | S32、S64、U32、U64 |
| `AND` / `OR` / `XOR` | U32、U64 |
| `INC` / `DEC` | U32 |

地址仍由 `global_tensor` base 和字节行 stride 决定。Local CUBE 使用
`M322ND/M162ND`、显式 LB0/LB1 且省略 LB2；普通 Local/Shared 使用 NORM 和
LB2 物理列数。每个有效元素对对应 GM 地址执行一次原子读改写。

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

`TLOAD/TSTORE` 的 GM 行 stride 以**字节**计，而不是元素数；普通 Tile 与 CUBE Tile 分别走普通传输和布局转换路径。source/destination 的顺序不可互换。

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

### Shared Tile 与部分 PE 存储

`TSTORE(gm, shared)` 将完整 Shared 有效区写回 GM，默认掩码为 `15`。
`TSTORE<PEMask>(gm, shared)` 指定参与 PE，每个参与 PE 仍写回完整有效区。
若只写回某个范围，传入 `range::shared_nd_subview`：

```cpp
TSTORE<8>(gm, shared);                                // 一个 PE 写回完整有效区
TSTORE<8>(gm_fragment, range::shared_nd_subview<8, 8>(shared));
```

`PEMask` 可取 `0, 1, 2, 4, 8, 12, 14, 15`；`0` 不写回。Shared 源须已发布，
为非 boxed RowMajor，容量在 `128 B..256 KiB`，且 dtype 与 GM 一致。上例子视图从
第 `8 × 128 B` 处开始、长度为 `8 × 128 B`；写回视图与父 Tile 有效区的交集。
多 PE 分段写回时，用子视图的运行时偏移指定各 PE 的源范围，并为它们准备
相应的 GM 地址。PE 掩码不会自动切分源或调整 GM 地址。

### Vector CUBE layout

Vector location 也支持持久化的 CUBE cell layout。推荐使用专用别名表达
layout 意图：

| 别名 | layout | `TSTORE` 转换选择器 | 适用的最大逻辑行数 |
| --- | --- | --- | --- |
| `VecTileM16<T, R, C>` | `BLayout::CubeM16` | `M162ND` | 16 |
| `VecTileM32<T, R, C>` | `BLayout::CubeM32` | `M322ND` | 32 |

这两个别名等价于 `Tile<Location::Vec, T, R, C, BLayout::CubeM16>` 或
`BLayout::CubeM32`。它们使用 CUBE 的持久化 cell 存储，而不是普通
`RowMajor` Vector Tile；对应的 `TLOAD` 选择器分别是 `ND2M16` 和
`ND2M32`。加载和存储必须使用相同的 CUBE layout，不能在 M16 与 M32
之间混用。

```cpp
using VecM16 = VecTileM16<float, 16, 32>;
using VecM32 = VecTileM32<float, 32, 32>;
using GM16 = global_tensor<float, RowMajor<16, 32>>;
using GM32 = global_tensor<float, RowMajor<32, 32>>;

void store_vector_cubes(float *data16, float *data32,
                        const VecM16 &m16, const VecM32 &m32) {
  GM16 dst16(data16);
  GM32 dst32(data32);
  TSTORE(dst16, m16);  // M162ND
  TSTORE(dst32, m32);  // M322ND
}
```

### 默认值

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `ValidRow`/`ValidCol` 为运行期（`DYNAMIC`）时，对应 `B.DIM` 使用寄存器源形式（`B.DIM <gpr>, 0, ->lbN`，RegSrc 为绝对 GPR 0..23），不使用立即数/压缩形式；仅静态维度使用 `C.B.DIMI`/立即数形式（ADR-BLOCK-0012 Decision 013/014）。每个 `LB0/LB1/LB2` 只写一次。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void store_roundtrip(float *output, const float *input) {
  using GM = pto::global_tensor<float, pto::RowMajor<32, 32>>;
  using Data = pto::Tile<pto::Location::Vec, float, 32, 32>;
  GM source(input), destination(output);
  Data tile;
  TLOAD(tile, source);
  TSTORE(destination, tile);
}
```

## Bundle 对照

```asm
Local 和 Shared 均使用 TLSU `TSTORE`。Local 源由 `B.IOT` 绑定，Shared 源由
`B.IOS` 绑定。Shared 子视图在源绑定后附加 `B.SUBVIEW`；`B.IOR` 提供 GM
基地址与行跨度。Local CUBE 源使用 `M322ND`、`M162ND` 或 `N82ND` 布局转换。
```

活动目录编码为 `TLSU`，function `1`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/memory-and-data-movement/regular/TSTORE.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/memory-and-data-movement/regular/TSTORE.asl)

[返回操作索引](../README.md)
