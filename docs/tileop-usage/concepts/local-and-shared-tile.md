# Local 与 Shared Tile

Local Tile 使用 `B.IOT`，Shared Tile 使用 `B.IOS`。Shared handle 必须遵守 inline SSA 和
PE participation 规则；任何具体操作的 Shared 支持以该操作页面为准。

## 使用要求

根据操作接口选择正确的 Tile location；不能把 Local Tile 当作 Shared Tile 传入，也不能在不支持 Shared 的操作中自行替换 binder。

## 约束、默认值、异常和边界行为

Local 与 Shared 对 SizeCode、容量、PE mask、handle 生命周期和参与 PE 集合有不同约束。省略的描述字段使用具体操作默认值；location、handle、容量或参与掩码非法时可能在执行前失败。边界区域、padding 和 fault 行为以具体操作的 PTO-SPEC contract 为准。

## 使用示例

```cpp
// 具体 Tile 类型必须与操作页面声明的 location 约束一致。
TADD(local_dst, local_lhs, local_rhs);
```

## 完整语义

Local/Shared Tile 的完整规则请参阅 [PTO-SPEC tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。
