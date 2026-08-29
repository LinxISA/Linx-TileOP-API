# TSTORE

TSTORE stores one valid Local or Shared rectangle to GM without modifying the source Tile.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_global_data_v gm_shape, is_tile_data_v tile_shape>
void TSTORE(gm_shape &dst, tile_shape &src);
template <is_global_data_v gm_shape, is_shared_tile_v SharedTileT>
PTO_SHARED_INLINE void TSTORE(gm_shape &dst, const SharedTileT &src);
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

成功调用后，`TSTORE` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
The Local form uses TLSU Function 1, exactly one terminating source B.IOT, at most one B.IOR, and no
B.IOS       . The Shared full form uses TLSU Function 1, exactly one source B.IOS, at most one B.IOR, no B.IOT, and PE_MASK=1111 for every nonzero access. The Shared partial form uses TLSU Function 14 (TSTORE.SPART), exactly one source B.IOS, at most one
B.IOR       , no B.IOT, and any nonzero PE subset. The Local CUBE form uses Function 1, explicit B.DATR M322ND, M162ND, or N82ND with DTYPE_NONE, explicit LB0/LB1, absent LB2, and one persistent source B.IOT.
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using GM = global_tensor<float, RowMajor<32, 32>>;
using TileT = Tile<Location::Vec, float, 32, 32>;
float data[32 * 32] = {};
GM dst(data);
TileT src;
TSTORE(dst, src);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`TSTORE.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/memory-and-data-movement/regular/TSTORE.md)。
