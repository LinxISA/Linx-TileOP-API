# Tile 约束

所有接口都必须满足 dtype、shape、valid region、layout、capacity、location 和 PE mask 约束。
具体操作的约束以对应页面、`template_asm.hpp` 和 PTO-SPEC contract 为准。

## 使用要求

在调用操作前检查输入和输出 Tile 的数据类型、物理 shape、valid region、layout、capacity、location 以及 PE participation mask。

## 默认值

操作页面未列出的可选字段使用对应 PTO-SPEC contract 的默认值；显式编码的零值不一定等同于省略字段。

## 异常和边界行为

超过物理 shape 的 valid region、容量不足、未初始化输入、非法 location 或不支持的数据类型可能在执行前被拒绝。空区域、padding、边界访问、alias、数值异常和 `PE_MASK=0000` 的行为由具体操作规范定义。

## 使用示例

```cpp
// dst、lhs、rhs 的 dtype/shape/layout/location 必须满足 TADD 页面约束。
TADD(dst, lhs, rhs);
```

## 完整语义

完整约束请参阅 [PTO-SPEC tile 文档](https://github.com/PTO-ISA/pto-spec/tree/v0.58.4.1/docs/tile)。
