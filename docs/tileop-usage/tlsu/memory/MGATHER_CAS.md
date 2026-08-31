# MGATHER_CAS

`MGATHER_CAS` 是由 TLSU 执行的选择器编码 Tile 操作：它使用 index、expected 和 replacement Tile 对 GM 中的每个元素执行比较交换，并记录观察到的旧值；其当前指令 contract 规定了确切的 bundle 形式和发布边界。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <
    is_tile_data_v DstTile,
    is_tile_data_v IndexTile,
    is_tile_data_v ExpectedTile,
    is_tile_data_v ReplacementTile>
void MGATHER_CAS(
    DstTile &observedOld,
    uint64_t base,
    IndexTile &byteDisplacements,
    ExpectedTile &expected,
    ReplacementTile &replacement,
    uint32_t validCol,
    uint32_t validRow = 1);
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
| `observedOld` | 保存每个位置观察到的旧值的输出 Tile。 |
| `base` | GM 基地址。 |
| `byteDisplacements` | 以字节为单位的地址位移索引 Tile。 |
| `expected` | 比较交换操作的期望值 Tile。 |
| `replacement` | 比较成功时写入 GM 的替换值 Tile。 |
| `validCol` | 有效区域的列数。 |
| `validRow` | 有效区域的行数，省略时使用接口/规范默认值。 |



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

 以下是 C++ 声明中可直接省略的默认实参：

| 参数 | 默认值 |
| --- | --- |
| `validRow` | `1` |

### 编码字段和省略值

- 省略 `B.DATR` 时，padding 值使用 `Null`，布局使用 `NORM`。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`MGATHER_CAS` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.MGATHER.CAS DataType
B.DATR      PadValue, Layout (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       IndexTile, ExpectedTile, mask=PE_MASK
B.IOT       ReplacementTile, mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       BaseGPR, zero, zero, ->zero
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Transfer = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
using ByteOffsets = Tile<Location::Vec, int16_t, 8, 256, BLayout::RowMajor>;

void compare_exchange(Transfer &observed_old, ByteOffsets &byte_offsets,
                      Transfer &expected, Transfer &replacement) {
  // 每个 offset 是相对于 base 的字节位移；返回值是交换前读到的值。
  MGATHER_CAS(observed_old, 0x1000ull, byte_offsets, expected, replacement,
              256, 2);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。