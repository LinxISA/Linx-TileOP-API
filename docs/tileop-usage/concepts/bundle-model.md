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
// 推荐：直接调用具体操作的 C++ API。
TADD(dst, lhs, rhs);
```

## 完整语义

Bundle 字段和提交边界的完整语义请参阅 [PTO-SPEC tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。
