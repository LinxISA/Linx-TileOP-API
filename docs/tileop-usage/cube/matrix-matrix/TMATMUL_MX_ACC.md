# TMATMUL_MX_ACC

TMATMUL_MX_ACC multiplies scaled matrices and accumulates into the supplied accumulator Tile.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
PTO_SHARED_INLINE void TMATMUL_MX_ACC(
    tile_shape_d &d,
    tile_shape_c &c,
    tile_shape_a &a,
    tile_shape_sa &scale_a,
    tile_shape_b &b,
    tile_shape_sb &scale_b);
template <
    int ScaleMask = 3,
    is_tile_data_v tile_shape_d,
    is_tile_data_v tile_shape_c,
    is_local_or_shared_left tile_shape_a,
    typename tile_shape_sa,
    is_local_or_shared_right tile_shape_b,
    typename tile_shape_sb,
    fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(
    tile_shape_d &d,
    tile_shape_c &c,
    tile_shape_a &a,
    tile_shape_sa &scale_a,
    tile_shape_b &b,
    tile_shape_sb &scale_b,
    const Options &options);
template <
    is_tile_data_v D,
    is_tile_data_v C,
    is_local_or_shared_left A,
    is_local_or_shared_right B,
    fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b, const Options &options);
template <
    is_tile_data_v D,
    is_tile_data_v C,
    is_local_or_shared_left A,
    is_any_tile_data_v SA,
    is_local_or_shared_right B,
    fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, SA &sa, B &b, const Options &options);
template <
    is_tile_data_v D,
    is_tile_data_v C,
    is_local_or_shared_left A,
    is_local_or_shared_right B,
    is_any_tile_data_v SB,
    fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b, SB &sb, const Options &options);
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b);
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, SA &sa, B &b);
PTO_SHARED_INLINE void TMATMUL_MX_ACC(D &d, C &c, A &a, B &b, SB &sb);
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

成功调用后，`TMATMUL_MX_ACC` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TMATMULMX.ACC AType
B.DATR      BType, RMode, Sat (optional; BType defaults to AType)
B.FPATR     PreQuantMode, ReluMode, GroupNCode, RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn, TransA, TransB, CScaleEn (exactly one)
B.DIM       LB0 M or cooperative group_M (optional, default 1)
B.DIM       LB1 N (optional, default 1)
B.DIM       LB2 K (optional, default 1)
B.IOS       complete right or both matrix operand groups (optional; cooperative mask 1111)
B.IOT       ordered Local mathematical sources: C CUBE_M16/M32 accumulator matching A with encoded selector distinct from DstTile, A CUBE_M16/M32 primary, optional A scale, B CUBE_N8 primary, optional B scale, optional U8 CScale CUBE_M32 [M,1]
B.IOT       D matching A's CUBE_M16/M32 layout with a distinct encoded destination index, optional RowMaxOut, optional GroupMaxOut destinations
B.IOT/B.IOR postprocess operands selected by B.FPATR
BSTOP       or the next BSTART completion boundary
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using A = CubeTileM16<__fp8_e4m3, 16, 32>;
using B = CubeTileN8<__half, 32, 16>;
using Acc = CubeAccumulatorM16<float, 16, 16>;
using ScaleA = Tile<Location::Scaling, __fp8_e8m0, 16, 8,
                     BLayout::RowMajor, 16, 1>;
using ScaleB = Tile<Location::Scaling, __fp8_e8m0, 8, 16,
                     BLayout::RowMajor, 1, 16>;

void matmul_mx_acc(Acc &d, Acc &c, A &a, ScaleA &scale_a, B &b,
                   ScaleB &scale_b) {
  // d = c + dequant(a, scale_a) * dequant(b, scale_b)。
  TMATMUL_MX_ACC(d, c, a, scale_a, b, scale_b);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`TMATMUL_MX_ACC.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/matrix-and-matrix-vector/matrix-matrix/TMATMUL_MX_ACC.md)。
