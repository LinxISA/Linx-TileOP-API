# MSCATTER_MASK

`MSCATTER_MASK` 使用索引 Tile 中的 S32/U32/S64/U64 字节位移访问 GM：每个访问起点为 `Base + displacement`，不进行元素宽度缩放，不使用 GM 行 stride。可选 `IndexedByteRange` 传递相对 Base 的有符号 64 位字节边界；详见 [indexed TLSU 范围参数](../../indexed-tlsu-range.md)。

## C++ 接口

### 实验性 PredicateCell 消费者

开发分支提供 `predicate::scatter(destination, byte_offsets, values, active)`，
可直接消费 `predicate::compare_scalar` 返回的强类型 CUBE PredicateCell，
不插入 TCVT。它不是把普通 U8 Tile 强制转换成 PredicateCell，也不改变
下列既有重载。当前 executable ASL helper 接受这一 carrier，但指令页文字
仍有待 owner 对齐的差异；支持边界与说明见
[PredicateCell adapters](../../predicate-adapters.md)。该接口尚未发布。

当前 API 中可用的调用形式：

```cpp
template <
    typename tile_shape_in,
    typename tile_shape_offset,
    typename tile_shape_mask,
    typename gm_shape>
inline void MSCATTER_MASK(
    gm_shape &dst,
    const tile_shape_in &src,
    const tile_shape_offset &offset,
    const tile_shape_mask &mask);
template <
    typename tile_shape_in,
    typename tile_shape_offset,
    typename tile_shape_mask,
    typename gm_shape>
inline void MSCATTER_MASK(
    gm_shape &dst,
    const tile_shape_in &src,
    const tile_shape_offset &offset,
    const tile_shape_mask &mask,
    IndexedByteRange range);
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
| `offset` | 相对 GM Base 的显式字节位移 Tile；不按元素宽度缩放。 |
| `mask` | Local U8 字节掩码；每个索引事务一个字节，值只能为 0 或 1。 |



范围重载在最后追加 `IndexedByteRange range` 参数。

## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

`mask` 必须是普通 Local `uint8_t` Tile，每个索引事务对应一个字节，字节值只能为 `0x00` 或 `0x01`；`2`、`0xff` 等非 canonical 值在执行时拒绝，不能按非零即 true 解释。mask 的 dtype 与传输数据的 dtype 独立。

mask、index、data 必须使用相同布局类（RowMajor、CUBE_M16 或 CUBE_M32）；mask 与 index 的有效行列必须一致，包括动态有效区域的实际值。每个 Tile 的物理行列和容量可以不同，但必须覆盖访问区域，mask 容量须有合法 SizeCode（128 B..256 KiB）。动态有效区域的一致性和边界由调用方保证，执行侧继续校验。CUBE 布局的发射支持单独跟踪于 [#201](https://github.com/LinxISA/Linx-TileOP-API/issues/201)，本页 RowMajor 示例不作为 CUBE 路径的验证证据。

RowMajor `TCMP/TCMPS` 产生 packed bits，不能直接作为本接口的字节掩码，即使其 Tile dtype 是 U8。reinterpret 或仅修改模板 dtype 不会解包。通过 `TLOAD` 加载预先生成的 0/1 字节是合法生产方式。类型检查只能排除不兼容载体，无法证明数据来源或字节值合法。

mask=0 时不进行对应 lane 的地址生成或内存访问；gather 使用所选 `PadValue`，scatter 保留对应 GM 原值。offset 为相对 Base 的字节位移，不按元素宽度缩放。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

mask 与 index 的有效区域必须一致，每个索引事务读取一个 U8 字节。仅 mask=1 的位置写入 GM；mask=0 的地址保留原值。本操作不分配输出 Tile。


## 默认值

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- 省略 `B.DATR` 时使用该操作规定的默认编码；若显式提供该描述符，未使用的字段必须保持为零。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- `B.IOR` 的首条无标志记录为 `[Base, Min, Max]`；省略范围时 Min/Max 选择器均为 zero。显式范围必须至少有一个非 zero 边界选择器，即使其寄存器值为 0。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；范围 Min > Max 时仅当前参与 PE 在副作用前 fault，其他 PE 已有的副作用保留。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

只更新活动 lane 对应的 GM 地址，非活动地址保留原值；输入 Tile 保持不变。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.MSCATTER.MASK DataType
B.DATR      Layout (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       DataTile, IndexTile, mask=PE_MASK
B.IOT       MaskTile, mask=PE_MASK, last
B.IOR       [BaseGPR, MinIndexGPR, MaxIndexGPR], []
BSTOP
```

## 使用示例

主机侧准备 32 个字节：`enabled[j] = memory[j] >= 16 ? 1 : 0`。
`offsets[j] = (int32_t(j) - 16) * int32_t(sizeof(float))`，调用时 Base 指向 `memory + 16`，
确保负位移也落在已分配内存内。scatter 的目标区应预先初始化，以核对非活动地址保持原值。

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Values = Tile<Location::Vec, float, 1, 32, BLayout::RowMajor>;
using ByteOffsets = Tile<Location::Vec, int32_t, 1, 32, BLayout::RowMajor>;
using Mask = Tile<Location::Vec, uint8_t, 1, 128, BLayout::RowMajor, 1, 32>;
using GM = global_tensor<float, RowMajor<1, 32>>;

void scatter_masked(float *base, const float *input, const int32_t *offsets,
                    const uint8_t *enabled) {
  GM base_gm(base);
  Values src;
  ByteOffsets offset;
  Mask mask;
  global_tensor<float, RowMajor<1, 32>> input_gm(input);
  global_tensor<int32_t, RowMajor<1, 32>> offsets_gm(offsets);
  global_tensor<uint8_t, RowMajor<1, 32>> enabled_gm(enabled);
  TLOAD(src, input_gm);
  TLOAD(offset, offsets_gm);
  // enabled[j] must be exactly 0 or 1; each byte controls one transaction.
  TLOAD(mask, enabled_gm);
  MSCATTER_MASK(base_gm, src, offset, mask);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。
