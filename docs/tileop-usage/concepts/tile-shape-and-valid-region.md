# Shape、valid region 与 SizeCode

物理 Tile shape、valid region 和 SizeCode 是不同概念。valid rows/columns 不得超过物理
rows/columns，SizeCode 表示容量而不是重新定义逻辑矩阵维度。

## 使用要求

创建或传递 Tile 时，valid rows/columns 必须位于物理 shape 内；输出容量和 SizeCode 必须覆盖操作所需的存储范围。

## 约束、默认值、异常和边界行为

valid region 超出物理 shape、SizeCode 不在 location 支持范围内或输入有效元素未初始化时，操作可能在执行前失败。省略的 valid dimension 使用具体操作默认值；valid region 外的物理空间只按操作 contract 规定作为 padding 处理，不能当作有效输入。

## 使用示例

```cpp
// 具体 Tile 类型和 valid region 必须与操作页面要求一致。
TADD(dst, lhs, rhs);
```

## 完整语义

Shape、valid region 和 SizeCode 的完整语义请参阅 [PTO-SPEC tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。
