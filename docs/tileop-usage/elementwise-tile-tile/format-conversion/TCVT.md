# TCVT

The partitioned examples below use `std::move`; include `<utility>` when using
`TASSEMBLY` in a standalone translation unit.

`TCVT` is a TileOP C++ interface for the PTO ISA v0.58.3 `TCVT` operation.

## 接口身份

| 项目 | 内容 |
| --- | --- |
| API 名称 | `TCVT` |
| ISA 操作 | `TCVT` |
| 执行引擎 | `VEC` |
| 指令分类 | `elementwise-tile-tile` |
| 编码 carrier | `TEPL` |
| Logical selector | `27` (`0x1b`) |
| Function | `27` |
| Mode | `0` |
| C++ 定义位置 | `include/jcore/template_asm.hpp:6966` |

## C++ 接口

```cpp
template <is_tile_data_v Dst, is_tile_data_v Src0, is_tile_data_v Src1>
void TCVT(Dst &dst, Src0 &src0, Src1 &src1);
```

上面的签名是该操作族的接口摘要。完整的模板重载、默认参数和辅助类型以
`template_asm.hpp` 为准；本页面不把宏展开或底层 inline-asm 实现伪装成新的公共接口。

## 功能语义

`TCVT` 在所有合法操作数和 Tile 描述符完成检查后，对有效区域执行 `TCVT`
对应的 PTO 操作。有效区域之外的物理位置不应被当作输入数据；具体 padding、数值
状态和 alias 行为由 PTO-SPEC 的当前 contract 定义。

## 操作数与结果

- 所有 Tile 参数必须满足其 C++ 类型、dtype、shape、layout、capacity 和 location 约束。
- 输出 Tile 是由接口签名指定的新目标或目标参数；输入 Tile 的角色和顺序不得交换。
- 除接口显式声明的 scalar、descriptor 或 Shared Tile 外，不得推断额外操作数。
- `PE_MASK`、valid region 和 Tile SizeCode 必须符合 [通用约束](../../concepts/tile-constraints.md)。

## 分类与执行引擎

```text
Instruction class: elementwise-tile-tile
Execution engine: VEC
```

执行引擎与分类是两个独立属性；不得仅根据分类推断 engine。

## 汇编与编码

```asm
TCVT <bundle operands>
```

| Operation | Carrier | Selector | Function | Mode |
| --- | --- | ---: | ---: | ---: |
| `TCVT` | `TEPL` | `27` (`0x1b`) | `27` | `0` |

`TCVT` 是 selector-encoded block operation，不是独立 standalone opcode。

## Bundle composition

```asm
BSTART.VEC TCVT, DataType
; operation-defined descriptor fields, dimensions, scalar inputs, and Tile bindings
; are emitted according to this operation's C++ overload and PTO contract
BSTOP
```

实际 binder 数量、source/destination 角色、`last` 位置以及属性字段必须以
`template_asm.hpp` 和 PTO-SPEC contract 为准；本页不添加接口未声明的 binder。

## 默认值与零值编码

操作未显式提供的字段使用 PTO-SPEC 规定的默认值。显式编码的零值与省略字段可能
具有不同的 bundle 描述语义；调用者不得将二者自动等同。

## 合法性约束

必须满足以下边界：

1. 操作数类型和数量与 C++ overload 及 PTO contract 一致；
2. dtype、layout、shape、valid region 和物理容量合法；
3. Local/Shared location、PE mask 和 Tile SizeCode 合法；
4. 不存在未声明的 `B.IOT`、`B.IOS`、`B.IOR` 或 `B.DIM`；
5. 合法性检查失败时不得产生部分架构状态变化。

## 状态效果与内存效果

成功执行后，只发布 PTO contract 声明的目标 Tile、descriptor、definedness、padding
和 numeric-status 变化。没有明确声明的 GM、Shared 或同步副作用不得由 API 使用者假定。

## 异常与错误边界

不支持的 dtype、layout、shape、容量、location、PE mask、属性组合或 bundle 结构应在
C++ 模板实例化阶段或 Tile legality/allocation preflight 阶段被拒绝。拒绝必须发生在
部分目标写入和状态发布之前。

## 使用示例

普通 Tile 转换使用标准的 Tile destination：

```cpp
using Src = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;
using Dst = TileLeft<__bf16, 32, 16>;

Src src;
Dst dst;
TCVT(dst, src);
```

当转换结果属于一个分区组装结果时，destination 可以使用
`TileArrayOutputRef`。该写法把 `TCVT` 的结果直接写入对应 slot：

```cpp
using Parent = TileLeft<__bf16, 32, 64>;
using Fragment = TileLeft<__bf16, 32, 16>;
using Input = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

TileArray<Fragment, 1, 4> fragments;
Input input;

for (int j = 0; j < 4; ++j) {
  auto slot = fragments[0][j];
  TCVT(slot, input);
}

Parent result = TASSEMBLY<Parent>(std::move(fragments));
```

`TPARTVIEW` 可用于取得 source parent 的 fragment view，再将该 view 传给
支持 region 快速路径的 TileOP：

```cpp
using Parent = Tile<Location::Vec, float, 32, 64, BLayout::RowMajor>;
using Fragment = Tile<Location::Vec, float, 32, 16, BLayout::RowMajor>;

Parent parent;
auto parts = TPARTVIEW<Fragment, 1, 4>(parent);
auto part = parts[0][j];
Tile<Location::Vec, float, 32, 16, BLayout::RowMajor> scaled;
TMULS(scaled, part, 0.125f);
```

在当前 Linx inline-asm 实现中，上述 region overload 会在对应 binder 后
附加 `B.SUBVIEW`；`TCVT(TileArrayOutputRef, Tile)` 会在 destination binder
后附加 `B.ASSEMBLE`。`INIT/LAST` 和 parent size code 由 slot ordinal 与
TileArray 覆盖关系推导，调用者不需要手动编码这些字段。

完整的 partition/assembly 约束和生命周期说明见
[Tile partition and assembly views](../../tile-arrays.md)。
