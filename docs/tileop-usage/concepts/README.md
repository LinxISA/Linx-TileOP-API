# 通用概念

- [Tile 约束](tile-constraints.md)
- [Bundle 模型](bundle-model.md)
- [Local 与 Shared Tile](local-and-shared-tile.md)
- [Shape、valid region 与 SizeCode](tile-shape-and-valid-region.md)

## 使用要求

阅读具体 TileOP 页面前，应先理解 Tile 的位置、布局、容量、有效区域和 bundle 生命周期。概念文档提供共用规则，具体操作页面提供操作专用规则。

## 约束、默认值、异常和边界行为

所有 TileOP 都受 dtype、shape、valid region、layout、capacity、location 和 PE mask 约束。省略字段时采用对应操作 contract 的默认值；非法描述符、容量不足、未定义输入或不支持的组合可能在执行前被拒绝。空有效区域、padding、alias 和 fault 的精确定义由具体操作规范决定。

## 使用示例

建议先阅读 [Tile 约束](tile-constraints.md) 和 [Tile shape 与 valid region](tile-shape-and-valid-region.md)，再调用任一操作页面中的 C++ API。

## 完整语义

具体操作的完整语义请参阅对应页面末尾链接，以及 [PTO-SPEC tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。
