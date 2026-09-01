# Bundle 模型

TileOP 通过 `BSTART`、`B.DIM`、`B.DATR`、`B.IOR`、`B.IOT`、`B.IOS` 和 `BSTOP` 组成操作 bundle。
`TEPL` 是编译 carrier；engine alias 和 wrapper 名称不能互相混淆。

## 使用要求

开发者通常调用 TileOP C++ wrapper，由 wrapper 负责生成 bundle。只有在编写 assembler、调试编码或核对 binder 顺序时，才需要直接阅读 `BSTART` 到 `BSTOP` 的组成。

## 约束、默认值、异常和边界行为

一个 bundle 必须具有合法的开始和结束边界，binder 顺序、`last` 标记、Tile 位置、PE mask 和属性字段必须符合具体操作 contract。省略的可选字段采用操作规范默认值；显式零值可能具有不同含义。非法 binder、未定义输入、容量不足或不匹配的 engine/Tile 位置会在执行前失败，且不应假定产生部分结果。

`B.SUBVIEW` 和 `B.ASSEMBLE` 必须紧跟它们修饰的 `B.IOT` 或 `B.IOS` binder。
在 C++ 中应通过 `range::subview` 或 `range::assemble` 传递 carrier，让 wrapper
维护这个顺序。两类接口的 `base_units` 和 `OffsetUnits` 都以 128B 为单位；
高层接口使用 `zero` 或编译器自动分配的 GPR，不要求开发者暴露寄存器编号。

## 使用示例

```cpp
// tileop-doc: fragment -- illustrative call; operand declarations are omitted.
// 推荐：直接调用具体操作的 C++ API。
TADD(dst, lhs, rhs);
```

## Bundle 字段与提交边界

- Bundle 必须以一个合法的 `BSTART` 开始，并以 `BSTOP` 或下一个
  `BSTART` 形成完成边界；不能跨 bundle 使用 binder 或 modifier。
- `B.DIM`、`B.DATR`、`B.FPATR` 和 binder 的出现次数、顺序、字段适用性以及
  默认值由具体操作页列出的 bundle 结构决定；未列出的字段不能自行添加。
- 在操作数、descriptor、类型、shape、capacity、definedness、alias 和资源
  检查全部通过前，不分配目标，也不产生 payload、status 或 memory effect。
- 合法操作在其规定的提交点一次性发布目标及相关辅助输出；被拒绝的 bundle
  不发布部分结果。`PE_MASK=0000` 若被该操作支持，则在上述检查和副作用前作为严格空操作处理。
