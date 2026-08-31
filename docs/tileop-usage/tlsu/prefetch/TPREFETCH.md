# TPREFETCH

`TPREFETCH` 为全部四个 PE 预取一个带类型且带步长的 GM 矩形，不产生 Tile 目标。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_global_data_v gm_shape>
void TPREFETCH(const gm_shape &src, uint32_t valid_col, uint32_t valid_row);
```

### 支持的数据类型

支持的 Tile 数据类型由该操作的 C++ 模板约束和本页约束共同限定。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `src` | 输入 Tile 或源数据。 |
| `valid_col` | 运行时有效列数；不得超过 Tile 的物理列数。 |
| `valid_row` | 运行时有效行数；不得超过 Tile 的物理行数。 |



## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

内存地址、byte displacement、mask 和 PE 参与集合必须符合 TLSU contract；地址单位和 fault 行为见本页的异常和边界行为说明。

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

- 省略 `B.DATR` 时使用 `NORM` 布局。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TPREFETCH` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TPREFETCH DataType
B.DATR      Layout (optional)
B.DIM       LB0/ValidCol, LB1/ValidRow, LB2/Col (optional)
B.IOR       base,row_stride (optional)
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using GM = global_tensor<float, RowMajor<8, 512>>;

void prefetch_rows(const float *input) {
  GM input_gm(input);
  // 请求 4 行、每行 256 个有效元素对应的 GM cache lines。
  TPREFETCH(input_gm, 256, 4);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。