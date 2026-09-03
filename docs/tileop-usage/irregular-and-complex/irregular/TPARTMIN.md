# TPARTMIN (unreleased)

> **注意**：`TPARTMIN` 已从当前 PTO ISA catalog（main `961fa81e`）的活跃操作集中移除（deleted_names）。本页保留作历史参考；新代码不应使用该操作。

`TPARTMIN` 构造以原点对齐的并集，并选择重叠位置的最小值。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_tile_data_v tile_shape>
void TPARTMIN(tile_shape &dst, tile_shape &src0, tile_shape &src1);
```

### 支持的数据类型

支持FP32、FP16、BF16、S32、S16、S8、U32、U16、U8类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |



## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

索引/地址 Tile 的 dtype、单位和取值范围必须有效；重复目标、越界、alias 与写入顺序的语义 由该操作的约束和边界行为定义，不能由普通逐元素操作的直觉推断。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。
- 物理 padding 始终使用 `Null`。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TPARTMIN` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TPARTMIN, FP32|FP16|BF16|S32|S16|S8|U32|U16|U8
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional; omission defaults to 1)
B.DIM       rCol, 0, ->LB2  ; (optional; omission defaults to ValidCol)
B.IOT       exactly two persistent Local sources and one new Local destination, common PE_MASK, last
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using TileF32 = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;
using GMF32 = global_tensor<float, RowMajor<8, 32>>;

void min_partial(float *out, const float *a, const float *b) {
  GMF32 out_gm(out), a_gm(a), b_gm(b);
  TileF32 dst, src0, src1;
  TLOAD(src0, a_gm);
  TLOAD(src1, b_gm);
  TPARTMIN(dst, src0, src1);
  TSTORE(out_gm, dst);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。