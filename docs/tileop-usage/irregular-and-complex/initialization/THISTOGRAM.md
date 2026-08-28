# THISTOGRAM

`THISTOGRAM` is a TileOP C++ interface for the PTO ISA v0.58.3 `THISTOGRAM` operation.

## 接口身份

| 项目 | 内容 |
| --- | --- |
| API 名称 | `THISTOGRAM` |
| ISA 操作 | `THISTOGRAM` |
| 执行引擎 | `SFU` |
| 指令分类 | `irregular-and-complex` |
| 编码 carrier | `TEPL` |
| Logical selector | `104` (`0x68`) |
| Function | `8` |
| Mode | `3` |
| C++ 定义位置 | `include/jcore/template_asm.hpp:246` |

## C++ 接口

```cpp
void THISTOGRAM(...);
```

上面的签名是该操作族的接口摘要。完整的模板重载、默认参数和辅助类型以
`template_asm.hpp` 为准；本页面不把宏展开或底层 inline-asm 实现伪装成新的公共接口。

## 功能语义

`THISTOGRAM` 在所有合法操作数和 Tile 描述符完成检查后，对有效区域执行 `THISTOGRAM`
对应的 PTO 操作。有效区域之外的物理位置不应被当作输入数据；具体 padding、数值
状态和 alias 行为由 PTO-SPEC 的当前 contract 定义。

## 操作数与结果

- 所有 Tile 参数必须满足其 C++ 类型、dtype、shape、layout、capacity 和 location 约束。
- 输出 Tile 是由接口签名指定的新目标或目标参数；输入 Tile 的角色和顺序不得交换。
- 除接口显式声明的 scalar、descriptor 或 Shared Tile 外，不得推断额外操作数。
- `PE_MASK`、valid region 和 Tile SizeCode 必须符合 [通用约束](../../concepts/tile-constraints.md)。

## 分类与执行引擎

```text
Instruction class: irregular-and-complex
Execution engine: SFU
```

执行引擎与分类是两个独立属性；不得仅根据分类推断 engine。

## 汇编与编码

```asm
THISTOGRAM <bundle operands>
```

| Operation | Carrier | Selector | Function | Mode |
| --- | --- | ---: | ---: | ---: |
| `THISTOGRAM` | `TEPL` | `104` (`0x68`) | `8` | `3` |

`THISTOGRAM` 是 selector-encoded block operation，不是独立 standalone opcode。

## Bundle composition

```asm
BSTART.SFU THISTOGRAM, DataType
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

完整调用形式和类型约束请参阅 `template_asm.hpp` 中的定义以及对应的主题指南（若存在）。
本页的接口摘要只用于导航，不将宏展开或底层 inline-asm 实现伪装成公共 overload。

## 验证映射

相关实现和测试位于：

```text
/Users/lulu/Developer/LinxISA/latest-tileop-api/include/jcore/template_asm.hpp
/Users/lulu/Developer/LinxISA/latest-tileop-api/test/tileop_api/
```
