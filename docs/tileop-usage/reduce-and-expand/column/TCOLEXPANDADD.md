# TCOLEXPANDADD

TCOLEXPANDADD is a selector-encoded Tile operation executed by SFU. It adds each full-shape element to the broadcast value from the same column; its current instruction contract owns the exact bundle form and publication boundary.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <
    is_tile_data_v tile_shape_out,
    is_tile_data_v tile_shape_in0,
    is_tile_data_v tile_shape_in1>
void TCOLEXPANDADD(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1);
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

成功调用后，`TCOLEXPANDADD` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TCOLEXPANDADD, DataType
B.DATR      PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcTile, BroadcastTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

## 使用示例

```cpp
#include <jcore/template_asm.hpp>

using namespace pto;
using MatrixGM = global_tensor<float, RowMajor<32, 32>>;
using VectorGM = global_tensor<float, RowMajor<1, 32>>;
using MatrixTile = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 32>;
using ColTile = Tile<Location::Vec, float, 1, 32, BLayout::RowMajor, 1, 32>;
float matrix_data[32 * 32] = {}, col_data[32] = {}, dst_data[32 * 32] = {};
MatrixGM matrix_global(matrix_data), dst_global(dst_data); VectorGM col_global(col_data);
MatrixTile src, dst; ColTile col;
TLOAD(src, matrix_global); TLOAD(col, col_global);
TCOLEXPANDADD(dst, src, col);
TSTORE(dst_global, dst);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`TCOLEXPANDADD.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/reduce-and-expand/column-expansion/TCOLEXPANDADD.md)。
