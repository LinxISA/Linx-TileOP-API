# THISTOGRAM

`THISTOGRAM` 为每个源行构造包含 256 个 U32 bin 的前缀直方图。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void THISTOGRAM(tile_shape_out &dst, tile_shape_in &src, tile_shape_in &Idx, int ByteId);
```

### 支持的数据类型

支持U32、U16类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `Idx` | 索引 Tile 或索引描述符；其 dtype 和范围由对应重载约束。 |
| `ByteId` | 按字节寻址的标识或偏移；不得与元素索引混用。 |



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

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。
- 物理 padding 始终使用 `Null`。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`THISTOGRAM` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU THISTOGRAM, U16|U32
B.DATR      DstDataType=U32, ByteId=0..3 (mandatory)
B.IOT       SourceTile, FilterTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Samples = Tile<Location::Vec, uint32_t, 8, 32, BLayout::RowMajor>;
using Histogram = Tile<Location::Vec, uint32_t, 8, 256, BLayout::RowMajor>;
using GMSamples = global_tensor<uint32_t, RowMajor<8, 32>>;
using GMHistogram = global_tensor<uint32_t, RowMajor<8, 256>>;

void histogram_rows(uint32_t *out, const uint32_t *input) {
  GMSamples input_gm(input);
  GMHistogram out_gm(out);
  Samples src, filter;
  Histogram dst;
  TLOAD(src, input_gm);
  TLOAD(filter, input_gm);
  // 当前 C++ API 的 Idx 参数与 src 共用 tile_shape_in；U32 ByteId=3 不筛选。
  THISTOGRAM(dst, src, filter, 3);
  TSTORE(out_gm, dst);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。