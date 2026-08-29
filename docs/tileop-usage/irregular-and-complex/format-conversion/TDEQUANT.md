# TDEQUANT

TDEQUANT is a selector-encoded Tile operation executed by SFU. It computes FP32 (q - zero_point) * multiplier for each valid S8 or U8 source element; its current instruction contract owns the exact bundle form and publication boundary.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <
    RoundMode Mode = RoundMode::RNE,
    is_tile_data_v tile_shape_out,
    is_tile_data_v tile_shape_in>
void TDEQUANT(
    tile_shape_out &dst,
    tile_shape_in &src,
    float multiplier = 1.0f,
    int32_t zeroPoint = 0);
template <is_tile_data_v tile_shape_out, is_tile_data_v tile_shape_in>
void TDEQUANT(tile_shape_out &dst, tile_shape_in &src);
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

成功调用后，`TDEQUANT` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TDEQUANT, S8|U8
B.DATR      FP32, RMode
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional, default 1)
B.DIM       rCol, 0, ->LB2  ; (optional, default ValidCol)
B.IOR       MultiplierFP32, ZeroPoint (optional; omission selects 1.0 and 0)
B.IOT       SrcTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using InputTile = Tile<Location::Vec, int8_t, 8, 256, BLayout::RowMajor>;
using OutputTile = Tile<Location::Vec, float, 8, 256, BLayout::RowMajor>;
InputTile src;
OutputTile dst;
TDEQUANT<RoundMode::RTZ>(dst, src, 2.0f, 0);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`TDEQUANT.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/irregular-and-complex/format-conversion/TDEQUANT.md)。
