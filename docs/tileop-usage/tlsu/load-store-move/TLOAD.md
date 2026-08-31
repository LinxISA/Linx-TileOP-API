# TLOAD

`TLOAD` 加载一个普通的 Local 或 Shared 矩形，或将 GM 数据转换为持久的 Local CUBE 存储。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_tile_data_v tile_shape, is_global_data_v gm_shape>
requires(!tile_shape::IsCubeLayout) void TLOAD(tile_shape &dst, gm_shape &src);
template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
PTO_SHARED_INLINE SharedTile<shp> TLOAD(const gm_shape &src);
template <is_tile_data_v shp, int PEMask = 15, is_global_data_v gm_shape>
PTO_SHARED_INLINE void TLOAD(SharedTile<shp> &dst, const gm_shape &src);
template <is_tile_data_v cube_shape, is_global_data_v gm_shape>
requires(cube_shape::IsCubeLayout) void TLOAD(cube_shape &dst, gm_shape &src);
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

    成功调用后，`TLOAD` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TLSU TLOAD, FP16
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1
B.DIM       rCol, 0, ->LB2
B.IOT       mask=1111, last, ->T<4KB>
B.IOR       a0, a1, 0
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using GM = global_tensor<float, RowMajor<32, 32>>;
using TileT = Tile<Location::Vec, float, 32, 32>;
float data[32 * 32] = {};
GM src(data);
TileT dst;
TLOAD(dst, src);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。