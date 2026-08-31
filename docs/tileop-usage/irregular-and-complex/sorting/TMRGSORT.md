# TMRGSORT

`TMRGSORT` 稳定合并两个已排序的单行 Local 数据流。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_tile_data_v DstTile, is_tile_data_v LeftTile, is_tile_data_v RightTile>
void TMRGSORT(DstTile &dst, LeftTile &left, RightTile &right, bool descending = false);
```

### 支持的数据类型

支持FP32、FP16类型。

| 操作数角色 | 类型要求 |
| --- | --- |
| 值输入 / 值输出 | 支持FP32、FP16类型。 |
| 索引输出 | `U32`，保存组内原始位置。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `left` | 左输入 Tile。 |
| `right` | 右输入 Tile。 |
| `descending` | 排序方向；`false`（默认）为升序，`true` 为降序；稳定性不因方向改变。 |



## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

排序输出值与输入的有效区域对应；索引输出使用 U32 的组内原始位置。排序宽度、行组形状和 输入的已排序前提（仅 merge sort）必须满足该操作定义。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 仅在各独立行组的有效范围内排序或合并；索引以该有效行组为基准。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 以下是 C++ 声明中可直接省略的默认实参：

| 参数 | 默认值 |
| --- | --- |
| `descending` | `false` |

### 编码字段和省略值

- 省略 `B.DATR` 时使用该操作规定的默认编码；若显式提供该描述符，未使用的字段必须保持为零。
- 省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。
- 物理 padding 始终使用 `Null`。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TMRGSORT` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TMRGSORT, FP32|FP16
B.DATR      all-zero (optional)
B.IOR       Descending (optional; omission defaults to ascending)
B.IOT       two Local sources and one new Local destination, common PE_MASK, last
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using SortedHalf = Tile<Location::Vec, __half, 1, 128, BLayout::RowMajor>;
using SortedFull = Tile<Location::Vec, __half, 1, 256, BLayout::RowMajor>;
using GMHalf128 = global_tensor<__half, RowMajor<1, 128>>;
using GMHalf256 = global_tensor<__half, RowMajor<1, 256>>;

void merge_sorted(__half *out, const __half *left_ptr, const __half *right_ptr) {
  GMHalf128 left_gm(left_ptr), right_gm(right_ptr);
  GMHalf256 out_gm(out);
  SortedHalf left, right;
  SortedFull dst;
  TLOAD(left, left_gm);
  TLOAD(right, right_gm);
  TMRGSORT(dst, left, right, false);
  TSTORE(out_gm, dst);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。