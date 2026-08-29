# MGATHER

MGATHER is a selector-encoded Tile operation executed by TLSU. It uses each integer index as a signed or unsigned GM byte displacement and gathers the addressed elements into a new Local Tile; its current instruction contract owns the exact bundle form and publication boundary.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <
    typename tile_shape_out,
    typename tile_shape_offset,
    typename gm_shape,
    TmaPadValue Pad = TmaPadValue::Null>
inline void MGATHER(tile_shape_out &dst, const gm_shape &src, const tile_shape_offset &offset);
```

## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

除通用 Tile 约束外，必须满足 PTO-SPEC 对本操作规定的操作数角色、数据类型组合、形状、布局、有效区域、容量、存储位置、PE mask 以及 alias 规则。对于需要 Shared Tile、标量、索引、scale、bias 或选项对象的重载，只能使用接口声明的参数形式；不能通过省略参数来伪造另一种操作数组合。

## 默认值

未显式传入的可选参数使用该 C++ 重载和 PTO-SPEC contract 规定的默认值。默认选项、维度、布局、padding、scale mask 和属性字段可能与显式编码的零值不同；调用者不得把“省略”与“传入零值”自动等同。

## 异常和边界行为

类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，可能在编译期或运行前检查阶段被拒绝。有效区域为空、部分有效区域、边界坐标、padding、数值溢出、NaN/无穷值、输入输出 alias、内存 fault 以及 `PE_MASK=0000` 的行为均以该操作的 PTO-SPEC contract 为准；失败时不应假定已经产生部分输出或其他副作用。

## 结果说明

成功调用后，`MGATHER` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.MGATHER DataType
B.DATR      PadValue, Layout (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       IndexTile, mask=PE_MASK, last, ->DstTile<TSize>
B.IOR       BaseGPR, zero, zero, ->zero
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Values = Tile<Location::Vec, float, 8, 32, BLayout::RowMajor>;
using ByteOffsets = Tile<Location::Vec, uint16_t, 8, 32, BLayout::RowMajor>;
using GM = global_tensor<float, RowMajor<8, 1024>>;
using GMOut = global_tensor<float, RowMajor<8, 32>>;

void gather(float *out, const float *base, const uint16_t *offsets) {
  GM base_gm(base);
  GMOut out_gm(out);
  ByteOffsets offset;
  Values dst;
  TLOAD(offset, global_tensor<uint16_t, RowMajor<8, 32>>(offsets));
  // offset 中的每个元素是相对于 GM base 的字节位移。
  MGATHER(dst, base_gm, offset);
  TSTORE(out_gm, dst);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`MGATHER.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/memory-and-data-movement/irregular/MGATHER.md)。
