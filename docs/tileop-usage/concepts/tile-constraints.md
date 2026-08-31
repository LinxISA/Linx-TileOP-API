# Tile 约束

所有接口都必须满足 dtype、shape、valid region、layout、capacity、location 和 PE mask 约束。
具体操作的约束以对应页面和 `template_asm.hpp` 为准。

## 使用要求

在调用操作前检查输入和输出 Tile 的数据类型、物理 shape、valid region、layout、capacity、location 以及 PE participation mask。

## 默认值

操作页面未列出的可选字段使用该操作明确列出的默认值；显式编码的零值不一定等同于省略字段。

## 异常和边界行为

超过物理 shape 的 valid region、容量不足、未初始化输入、非法 location 或不支持的数据类型可能在执行前被拒绝。空区域、padding、边界访问、alias、数值异常和 `PE_MASK=0000` 的行为由具体操作规范定义。

## 使用示例

```cpp
// dst、lhs、rhs 的 dtype/shape/layout/location 必须满足 TADD 页面约束。
TADD(dst, lhs, rhs);
```

## 通用约束补充

- valid rows/columns 不得超过物理 rows/columns；物理容量由 SizeCode 表示，
  不会重新定义逻辑 shape。
- 输入的每个被读取元素必须已定义；输出容量、dtype、layout、shape、location
  和 PE participation mask 必须满足具体操作的声明。
- 非法 descriptor、未定义输入、容量不足、类型或 shape/layout 不匹配会在分配
  或 payload 写入前触发 `Fault_TileLegality`；不应假定产生部分输出。
- 物理 valid region 外的 padding、空区域、alias、NaN/无穷值和数值状态，只能依赖
  具体操作页明确写出的处理方式；未写出的状态不可依赖。
