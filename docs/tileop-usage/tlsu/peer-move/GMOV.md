# GMOV

`GMOV` 是由 TLSU 执行的选择器编码 Tile 操作：它为每个 PE 解析一个由 peer 选择的 read-old Local fragment，并将其按字节复制到选定的新 Local fragment；其当前指令 contract 规定了确切的 bundle 形式和发布边界。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <int PEMask = 15, is_tile_data_v tile_shape_dst, is_tile_data_v tile_shape_src>
void GMOV(tile_shape_dst &dst, uint64_t peer_tid, const tile_shape_src &src);
```

### 支持的数据类型

支持E2M1X2、E1M2X2、HiF4X2、S4X2、U4X2类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `peer_tid` | 目标 peer 的 Tile/线程标识；必须属于当前操作允许的 peer 集合。 |
| `src` | 输入 Tile 或源数据。 |



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