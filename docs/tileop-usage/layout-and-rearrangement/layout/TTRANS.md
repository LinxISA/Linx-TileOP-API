# TTRANS (unreleased)

> **注意**：`TTRANS` 已从当前 PTO ISA catalog（main `961fa81e`）的活跃操作集中移除（deleted_names）。本页保留作历史参考；新代码不应使用该操作。

`TTRANS` 将源 Tile 转置后写入目标 Tile。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TTRANS(tile_shape_out &dst, tile_shape_in &src);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、E2M1X2、E1M2X2、S4X2、U4X2、S64、S32、S16、S8、U64、U32、U16、U8类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |



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
| 有效元素 | 输出 valid region 由布局、偏移或拼接描述符计算；物理 padding 不等于有效数据。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TTRANS` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TTRANS, DataType
B.DATR      (optional)
B.DIM       LB0
B.DIM       (LB1/LB2 for 2D)
B.IOT
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using InputGM = global_tensor<float, RowMajor<32, 16>>;
using OutputGM = global_tensor<float, RowMajor<16, 32>>;
using InputTile = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using OutputTile = Tile<Location::Vec, float, 16, 32, BLayout::RowMajor>;
float src_data[32 * 16] = {}, dst_data[16 * 32] = {};
InputGM src_global(src_data); OutputGM dst_global(dst_data);
InputTile src; OutputTile dst;
TLOAD(src, src_global);
TTRANS(dst, src);
TSTORE(dst_global, dst);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。