# TSTORE

`TSTORE` 将一个有效的 Local 或 Shared 矩形存储到 GM，且不修改源 Tile。
对 Local CUBE Tile，`TSTORE` 等价于显式的 `TSTORE_CUBE` layout conversion；
GM 与 CUBE dtype 必须相同，CUBE capacity 必须在 `128 B..256 KiB` 范围内，
并且只写入 source 的 valid rows/columns。store padding 固定为 `Null`。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_global_data_v gm_shape, is_tile_data_v tile_shape>
requires(!tile_shape::IsCubeLayout) void TSTORE(gm_shape &dst, tile_shape &src);
template <is_global_data_v gm_shape, is_tile_data_v cube_shape>
requires(cube_shape::IsCubeLayout) void TSTORE(gm_shape &dst, const cube_shape &src);
template <is_global_data_v gm_shape, is_shared_tile_v SharedTileT>
PTO_SHARED_INLINE void TSTORE(gm_shape &dst, const SharedTileT &src);

// Explicit CUBE layout-conversion spelling.
template <is_global_data_v gm_shape, is_local_tile_v cube_shape>
requires(cube_shape::IsCubeLayout)
void TSTORE_CUBE(gm_shape &dst, const cube_shape &src);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、E2M1X2、E1M2X2、HiF4X2、S4X2、U4X2、S64、S32、S16、S8、U64、U32、U16、U8类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |

### 重载选择

这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。


## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

`TLOAD/TSTORE` 的 GM 行 stride 以**字节**计，而不是元素数；普通 Tile 与 CUBE Tile 分别走普通传输和布局转换路径。source/destination 的顺序不可互换。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

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

普通 Tile 使用常规 TLSU 传输；CUBE Tile 由统一 `TLOAD/TSTORE` 自动选择布局转换。需要在源码中显式表达该边界时，可使用 `TLOAD_CUBE/TSTORE_CUBE`。

### Shared Tile 与部分 PE 存储

`TSTORE(gm, shared)` 只接受完整的 `PE_MASK=1111` Shared store。需要写入非零
PE 子集时使用：

```cpp
template <int PEMask = 15, is_global_data_v gm_shape,
          is_shared_tile_v SharedTileT>
PTO_SHARED_INLINE void TSTORE_PART(gm_shape &dst, const SharedTileT &src);
```

`PEMask` 只能是 `1, 2, 4, 8, 12, 14, 15`。Shared Tile 的 Local payload
必须与 GM dtype 相同，必须是非 boxed RowMajor，且容量在 `128 B..256 KiB`
（`SizeCode=1..12`）内。Shared handle 必须在当前函数或 inline SSA 生命周期
内保持有效。`TSTORE`/`TSTORE_PART` 只存储 runtime valid rectangle；物理
capacity 不会扩大 GM 的逻辑输出区域。

`TSTORE_CUBE` 的两个 C++ 参数顺序是 `(global_tensor, cube_tile)`，并要求两者
dtype 相同；它只适用于 Local CUBE Tile，不是 Shared partial-store 接口。

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

## 默认值

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- 省略 `B.DATR` 时使用该操作规定的默认编码；若显式提供该描述符，未使用的字段必须保持为零。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TSTORE` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
The Local form uses TLSU Function 1, exactly one terminating source B.IOT, at most one B.IOR, and no B.IOS. The Shared full form uses TLSU Function 1, exactly one source B.IOS, at most one B.IOR, no B.IOT, and PE_MASK=1111 for every nonzero access. The Shared partial form uses TLSU Function 14 (TSTORE.SPART), exactly one source B.IOS, at most one B.IOR, no B.IOT, and any nonzero PE subset. The Local CUBE form uses Function 1, explicit B.DATR M322ND, M162ND, or N82ND with DTYPE_NONE, explicit LB0/LB1, absent LB2, and one persistent source B.IOT.
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using GM = global_tensor<float, RowMajor<32, 32>>;
using TileT = Tile<Location::Vec, float, 32, 32>;
float data[32 * 32] = {};
GM dst(data);
TileT src;
TSTORE(dst, src);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。