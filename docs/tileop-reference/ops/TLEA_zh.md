# TLEA

**Tile 标量与立即数** · **VEC** · 0.59 活动目录

## 简介

`TLEA` 将 Local 源 Tile 中的逻辑元素索引扩展为 64 位,并按每 PE 的元素位宽显式缩放为字节偏移(`dst[i] = extend64(src[i]) * (element_bits / 8)`,保留低 64 位)。它只生成字节偏移,不做 BaseGPR 相加、访存或数值状态更新。

## 数学语义

将每个索引按所选元素字节宽度缩放，得到 `offset[r,c] = index[r,c] × element_bytes`。

## C++ 接口

当前 API 中可用的调用形式:

```cpp
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TLEA(tile_shape_out &dst, tile_shape_in &src, unsigned element_bits);
```

参数顺序为 destination、source、element_bits。源有符号索引按符号扩展到 S64,无符号索引按零扩展到 U64,再乘以 `element_bits / 8`;结果保留低 64 位,不做饱和,也不产生溢出 fault。

### 支持的数据类型

- `src`(元素索引):S32、U32、S64、U64。
- `dst`(字节偏移):S64 或 U64。
- `element_bits`(每 PE 元素位宽):8、16、32 或 64;四位 packed 宽度拒绝。
- 源与目的共享同一 layout,只能是 RowMajor 或 CUBE_M32;Shared、CUBE_N8、CUBE_M16 拒绝。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile;成功调用后写入新的 S64/U64 字节偏移。物理 geometry 与容量独立于源。 |
| `src` | 输入 Tile,持有 Local 元素索引(S32/U32/S64/U64)。 |
| `element_bits` | 每 PE 元素位宽(bit),经 `B.IOR` 的私有 GPR 传入;无隐含宽度,零寄存器为非法宽度。 |

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的逐项规则;除非本页明确允许,不应假定可原地执行或允许 alias。直接 S64/U64 aliasing 读取的是旧的源负载。

操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束;只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 仅对源和目的共同的有效区域定义结果;未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量,不重新定义逻辑 shape;源与目的容量各自匹配自身 DataType。 |
| 输出 padding | 由 `CurrentBundlePadValue` 数值 padding 决定;除非明确规定,否则不可依赖。 |

### 默认值

此页面列出的 C++ 形参没有默认实参;不要把省略某个操作数与传入零值视为等价。

#### 编码字段和省略值

- 省略 `B.DATR` 时,PadValue 使用 `Null`,layout 使用 RowMajor;Layout 字段 29/31 分别选择 CUBE_M32/CUBE_M16(CUBE_M16 b64 输出未分配)。
- `LB0` 给出 `ValidCol`,必须存在且非零;省略 `LB1` 时 `ValidRow=1`,省略 `LB2` 时目的物理列数等于 `ValidCol`。
- `B.IOR` 必须存在,`RegSrc0` 提供元素位宽;没有隐含宽度或基址。其余字段编码为零。
- `PE_MASK=0000` 在任何读取、分配和 fault 之前都是严格 no-op。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  // 逻辑元素索引(S32)缩放为 S64 字节偏移。
  using IndexTile = Tile<Location::Vec, int32_t, 8, 32>;
  using OffsetTile = Tile<Location::Vec, int64_t, 8, 32>;
  using IndexGM = global_tensor<int32_t, RowMajor<8, 32>>;
  using OffsetGM = global_tensor<int64_t, RowMajor<8, 32>>;

  int32_t index_data[8 * 32] = {};
  int64_t offset_data[8 * 32] = {};
  IndexGM index_global(index_data);
  OffsetGM offset_global(offset_data);
  IndexTile indices;
  OffsetTile offsets;

  TLOAD(indices, index_global);
  // 针对 16 位(2 字节)元素,将元素索引缩放为字节偏移。
  TLEA(offsets, indices, 16u);
  TSTORE(offset_global, offsets);
}
```

当前提供 JCORE inline-asm wrapper；CPU simulator/AArch64 尚无对应 wrapper。
生产与消费保持在可内联区域。动态位宽是否合法、动态有效形状是否匹配由
目标端合同检查；API 不增加主机侧内存访问或数值状态操作。

## Bundle 对照

开发者通常直接调用 C++ 接口,无需手工编写 bundle。下面保留对应汇编结构供核对:

```asm
BSTART.VEC  TLEA, SrcDataType
B.DATR      PadValue, Layout (optional)
B.DIM       ValidCol, 0, ->LB0
B.DIM       ValidRow, 0, ->LB1  ; (optional)
B.DIM       DstCol, 0, ->LB2    ; (optional)
B.IOT       IndexTile, mask=PE_MASK, last, ->ByteOffsetTile<TSize>
B.IOR       ElementBitsGPR, zero, zero, ->zero
BSTOP
```

对应编码为 TEPL Mode 1 Function 14(selector `0x02E`)。

活动目录编码为 `TEPL`，mode `1`，function `14`，selector `0x02E`。执行引擎为 `VEC`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/tile-scalar-and-immediate/arithmetic/TLEA.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/tile-scalar-and-immediate/arithmetic/TLEA.asl)

[返回操作索引](../README.md)
