# MSCATTER

`MSCATTER` 是由 TLSU 执行的选择器编码 Tile 操作：它将每个整数索引作为 GM 字节位移，并存储对应的有效源元素；其当前指令 contract 规定了确切的 bundle 形式和发布边界。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <typename tile_shape_in, typename tile_shape_offset, typename gm_shape>
inline void MSCATTER(gm_shape &dst, const tile_shape_in &src, const tile_shape_offset &offset);
```

### 支持的数据类型

支持索引 Tile 类型 S32、U32、S64、U64；支持传输数据 Tile 类型 FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、S64、S32、S16、S8、U64、U32、U16、U8。

| 操作数角色 | 类型要求 |
| --- | --- |
| 数据 Tile | 支持索引 Tile 类型 S32、U32、S64、U64；支持传输数据 Tile 类型 FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、S64、S32、S16、S8、U64、U32、U16、U8。 |
| 索引 / 地址位移 Tile | 必须使用该操作 contract 允许的整数 dtype 与单位。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |
| `offset` | 以元素或字节计的偏移；具体单位由该重载和本页约束定义。 |



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

- 省略 `B.DATR` 时使用该操作规定的默认编码；若显式提供该描述符，未使用的字段必须保持为零。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`MSCATTER` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.MSCATTER DataType
B.DATR      Layout (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       DataTile, IndexTile, mask=PE_MASK, last
B.IOR       BaseGPR, zero, zero, ->zero
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Values = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;
using ByteOffsets = Tile<Location::Vec, uint16_t, 8, 32, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<8, 1024>>;

void scatter(float *base, const float *input, const uint16_t *offsets) {
  GM base_gm(base);
  Values src;
  ByteOffsets offset;
  TLOAD(src, global_tensor<float, RowMajor<8, 32>>(input));
  TLOAD(offset, global_tensor<uint16_t, RowMajor<8, 32>>(offsets));
  // offset 中的每个元素是相对于 GM base 的字节位移。
  MSCATTER(base_gm, src, offset);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。