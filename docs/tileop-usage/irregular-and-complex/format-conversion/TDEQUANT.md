# TDEQUANT

`TDEQUANT` 对每个有效的 S8 或 U8 源元素计算 FP32 结果 `(q - zero_point) * multiplier`。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <
    RoundMode Mode = RoundMode::RNE,
    is_tile_data_v tile_shape_out,
    is_tile_data_v tile_shape_in>
void TDEQUANT(
    tile_shape_out &dst,
    tile_shape_in &src,
    float multiplier = 1.0f,
    int32_t zeroPoint = 0);
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TDEQUANT(tile_shape_out &dst, tile_shape_in &src);
```

### 支持的数据类型

支持源 Tile 类型 S8、U8；支持目标 Tile 类型 FP32。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `multiplier` | 量化或反量化使用的乘数。 |
| `zeroPoint` | 量化使用的零点。 |

### 重载选择

这些重载覆盖不同的 Tile location、返回方式或可选操作数。优先选择参数最少且能表达当前数据流的形式；不要通过传入无意义的零值来模拟另一个重载。


## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 以下是 C++ 声明中可直接省略的默认实参：

| 参数 | 默认值 |
| --- | --- |
| `multiplier` | `1.0f` |
| `zeroPoint` | `0` |

### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。
- 物理 padding 始终使用 `Null`。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TDEQUANT` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TDEQUANT, S8|U8
B.DATR      FP32, RMode
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional, default 1)
B.DIM       rCol, 0, ->LB2  ; (optional, default ValidCol)
B.IOR       MultiplierFP32, ZeroPoint (optional; omission selects 1.0 and 0)
B.IOT       SrcTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using InputTile = Tile<Location::Vec, int8_t, 8, 256, BLayout::RowMajor>;
using OutputTile = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
InputTile src;
OutputTile dst;
TDEQUANT<RoundMode::RTZ>(dst, src, 2.0f, 0);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。