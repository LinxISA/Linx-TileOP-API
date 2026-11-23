# GMOV

**GM 搬运与索引访存** · **TLSU** · 0.59 活动目录

## 简介

`GMOV` 是由 TLSU 执行的选择器编码 Tile 操作：它为每个 PE 解析一个由 peer 选择的 read-old Local fragment，并将其按字节复制到选定的新 Local fragment；其当前指令 contract 规定了确切的 bundle 形式和发布边界。

## 数学语义

在操作规定的 PE 间拷贝相同布局的 Local Tile。

## C++ 接口

```cpp
template <int PEMask = 15, is_tile_data_v tile_shape_dst, is_tile_data_v tile_shape_src>
void GMOV(tile_shape_dst &dst, uint64_t peer_tid, const tile_shape_src &src);
```

### 支持的数据类型

GMOV 按 ISA `TypeCode` 判断合法性。支持以下 22 种编码：

- 浮点：FP32、TF32、HF32、FP16、BF16、HIF8、FP8(E4M3/E5M2)、
  FP6(E3M2)、FP5(E2M3)、FP4(E2M1X2/E1M2X2)、E8M0、HiF4X2；
- 有符号整数：S32、S16、S8、S4X2；
- 无符号整数：U32、U16、U8、U4X2。

FP64、S64 和 U64 不合法。不能用 C++ carrier 的 `sizeof` 代替 TypeCode
判断，因为 packed 类型使用 8-bit carrier，但具有独立的合法 ISA 编码。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `peer_tid` | 目标 peer 的 Tile/线程标识；必须属于当前操作允许的 peer 集合。 |
| `src` | 输入 Tile 或源数据。 |

## 约束与默认值

`PEMask` 是 GMOV 专用的原始四位 PE mask。除 `0` 外的全部值
`1..15` 都是 ISA 合法值；`0` 不是此 C++ wrapper 的合法模板参数，
而是汇编层 GMOV 的严格 no-op 编码语义。该规则不同于仍使用旧
`PEMode` 编码的其他 API，不能用公共旧 mask 白名单替代。

内存地址、byte displacement、mask 和 PE 参与集合必须符合 TLSU contract；地址单位和 fault 行为见本页的异常和边界行为说明。任意非零 mask 都只选择目标写入 PE，不会减少 Core4 source readiness rendezvous。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- 省略 `B.DATR` 时使用 `NORM` 布局。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using TileF32 = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;

void receive_from_peer(TileF32 &dst, const TileF32 &src, uint64_t peer_tid) {
  // 所有参与该 collective 的 PE 必须到达同一个 GMOV 实例。
  GMOV(dst, peer_tid, src);
}
```

## Bundle 对照

```asm
BSTART.GMOV DataType
B.DATR      Layout (optional)
B.IOT       source, destination, PE_MASK, TSize, L=1
B.IOR       peer_tid (optional)
BSTOP
```

活动目录编码为 `TLSU`，function `13`。执行引擎为 `TLSU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/memory-and-data-movement/pe-movement/GMOV.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/memory-and-data-movement/pe-movement/GMOV.asl)

[返回操作索引](../README.md)
