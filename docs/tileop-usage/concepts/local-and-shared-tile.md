# Local 与 Shared Tile

Local Tile 使用 `B.IOT`，Shared Tile 使用 `B.IOS`。Shared handle 必须遵守 inline SSA 和
PE participation 规则；任何具体操作的 Shared 支持以该操作页面为准。

## 使用要求

根据操作接口选择正确的 Tile location；不能把 Local Tile 当作 Shared Tile 传入，也不能在不支持 Shared 的操作中自行替换 binder。

## 约束、默认值、异常和边界行为

Local 与 Shared 对 SizeCode、容量、PE mask、handle 生命周期和参与 PE 集合有不同约束。省略的描述字段使用具体操作默认值；location、handle、容量或参与掩码非法时在执行前失败。边界区域、padding 和 fault 行为以对应操作页明确列出的说明为准。

## 使用示例

```cpp
// tileop-doc: fragment -- illustrative call; operand declarations are omitted.
// 具体 Tile 类型必须与操作页面声明的 location 约束一致。
TADD(local_dst, local_lhs, local_rhs);
```

## Local/Shared 的具体约束

- Local Tile 的 allocation mask 必须描述已分配的 Local 寄存器；Shared Tile
  是 core-private 的持久 descriptor-plus-payload 状态。
- Shared Tile 的 `initialized_mask` 记录已写入的固定 offset quarter；只有
  `initialized_mask == allocation_mask` 且 payload 已定义时，Shared Tile 才完全初始化。
- Cooperative matrix Shared Tile 必须使用 allocation mask `1111`、initialized
  mask `1111`、已发布且内容已定义的 descriptor。
- Shared Tile 的 descriptor 必须已分配，容量在 `128 B..256 KiB` 范围内，shape
  不超过容量，valid rows/columns 不超过物理 rows/columns，且禁止通用索引访问
  CUBE layout。非 CUBE 的兼容 descriptor 还必须匹配容量、shape、dtype 和 layout。
- 非零 PE mask 的部分更新先检查 descriptor 兼容性，再复制选定 quarter；完整记录
  赋值是架构提交点。`PE_MASK=0000` 是不改变状态的严格空操作。
