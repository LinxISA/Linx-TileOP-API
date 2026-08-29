# TGEMV_MX_BIAS

TGEMV_MX_BIAS multiplies the scaled matrix and vector and adds the bias Tile.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
PTO_SHARED_INLINE void TGEMV_MX_BIAS(
    tile_shape_d &d,
    tile_shape_mtx &mtx,
    tile_shape_smtx &smtx,
    tile_shape_vec &vec,
    tile_shape_svec &svec,
    tile_shape_bias &bias);
template <
    int ScaleMask = 3,
    is_tile_data_v tile_shape_d,
    is_local_tile_v tile_shape_mtx,
    typename tile_shape_smtx,
    is_local_tile_v tile_shape_vec,
    typename tile_shape_svec,
    is_tile_data_v tile_shape_bias,
    fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_MX_BIAS(
    tile_shape_d &d,
    tile_shape_mtx &mtx,
    tile_shape_smtx &smtx,
    tile_shape_vec &vec,
    tile_shape_svec &svec,
    tile_shape_bias &bias,
    const Options &options);
template <
    is_tile_data_v D,
    is_local_tile_v Mtx,
    is_local_tile_v Vec,
    is_tile_data_v Bias,
    fixp::is_options_v Options>
requires(tile_role_v<Mtx> == Location::Right && tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(
    D &d,
    Mtx &mtx,
    Vec &vec,
    Bias &bias,
    const Options &options);
template <
    is_tile_data_v D,
    is_local_tile_v Mtx,
    is_local_tile_v Vec,
    is_local_tile_v ScaleVec,
    is_tile_data_v Bias,
    fixp::is_options_v Options>
requires(tile_role_v<Mtx> == Location::Right && tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(
    D &d,
    Mtx &mtx,
    Vec &vec,
    ScaleVec &scale_vec,
    Bias &bias,
    const Options &options);
template <
    is_tile_data_v D,
    is_local_tile_v Mtx,
    is_local_tile_v ScaleMtx,
    is_local_tile_v Vec,
    is_tile_data_v Bias,
    fixp::is_options_v Options>
requires(tile_role_v<Mtx> == Location::Right && tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX_BIAS(
    D &d,
    Mtx &mtx,
    ScaleMtx &scale_mtx,
    Vec &vec,
    Bias &bias,
    const Options &options);
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, Vec &vec, Bias &bias);
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, Vec &vec, ScaleVec &scale_vec, Bias &bias);
PTO_SHARED_INLINE void TGEMV_MX_BIAS(D &d, Mtx &mtx, ScaleMtx &scale_mtx, Vec &vec, Bias &bias);
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

成功调用后，`TGEMV_MX_BIAS` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TGEMVMX.BIAS AType
B.DATR      BType, RMode, Sat (optional; BType defaults to AType)
B.FPATR     PreQuantMode, ReluMode, GroupNCode, RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn, TransA, TransB, CScaleEn (exactly one)
B.DIM       LB0 M (optional, default 1; TGEMV permits only M=1)
B.DIM       LB1 N (optional, default 1)
B.DIM       LB2 K (optional, default 1)
B.IOT       ordered Local mathematical sources: A CUBE_M16/M32 primary, optional A scale, B CUBE_N8 primary, optional B scale, 1xN Bias
B.IOT       D matching A's CUBE_M16/M32 layout, optional RowMaxOut, optional GroupMaxOut destinations
B.IOT/B.IOR postprocess operands selected by B.FPATR
BSTOP       or the next BSTART completion boundary
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Vec = CubeTileM16<__fp8_e4m3, 1, 32>;
using Matrix = CubeTileN8<__fp8_e4m3, 32, 16>;
using D = CubeAccumulatorM16<float, 1, 16>;
using ScaleVec = Tile<Location::Scaling, __fp8_e8m0, 16, 8,
                       BLayout::RowMajor, 1, 1>;
using ScaleMatrix = Tile<Location::Scaling, __fp8_e8m0, 8, 16,
                           BLayout::RowMajor, 1, 16>;
using Bias = Tile<Location::Bias, float, 8, 16,
                  BLayout::RowMajor, 1, 16>;

void gemv_mx_bias(D &d, Matrix &matrix, ScaleMatrix &scale_matrix, Vec &vec,
                  ScaleVec &scale_vec, Bias &bias) {
  TGEMV_MX_BIAS(d, matrix, scale_matrix, vec, scale_vec, bias);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`TGEMV_MX_BIAS.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/matrix-and-matrix-vector/matrix-vector/TGEMV_MX_BIAS.md)。
