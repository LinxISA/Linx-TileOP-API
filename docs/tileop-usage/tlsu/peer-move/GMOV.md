# GMOV

`GMOV` 是由 TLSU 执行的选择器编码 Tile 操作：它为每个 PE 解析一个由 peer 选择的 read-old Local fragment，并将其按字节复制到选定的新 Local fragment；其当前指令 contract 规定了确切的 bundle 形式和发布边界。

## C++ 接口

当前 API 中可用的调用形式：

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



## 使用要求

- Tile 类型必须满足接口模板约束；
- source/destination 必须都是 Local Vec Tile（不能是 Matrix 或 Shared），且 dtype、
  physical shape、valid shape、BLayout、SLayout、SFractalSize 和逻辑容量完全相同；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

`PEMask` 是 GMOV 专用的原始四位 PE mask。除 `0` 外的全部值
`1..15` 都是 ISA 合法值；`0` 不是此 C++ wrapper 的合法模板参数，
而是汇编层 GMOV 的严格 no-op 编码语义。该规则不同于仍使用旧
`PEMode` 编码的其他 API，不能用公共旧 mask 白名单替代。

内存地址、byte displacement、mask 和 PE 参与集合必须符合 TLSU contract；地址单位和 fault 行为见本页的异常和边界行为说明。任意非零 mask 都只选择目标写入 PE，不会减少 Core4 source readiness rendezvous。

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
- 省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`GMOV` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.GMOV DataType
B.DATR      Layout (optional)
B.IOT       source, destination, PE_MASK, TSize, L=1
B.IOR       peer_tid (optional)
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using TileF32 = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;

void receive_from_peer(TileF32 &dst, const TileF32 &src, uint64_t peer_tid) {
  // 所有参与该 collective 的 PE 必须到达同一个 GMOV 实例。
  GMOV(dst, peer_tid, src);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。