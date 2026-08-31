# TGEMV_MX

`TGEMV_MX` 将经过缩放的矩阵与向量相乘并发布新的目标。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
PTO_SHARED_INLINE void TGEMV_MX(
    tile_shape_d &d,
    tile_shape_mtx &mtx,
    tile_shape_smtx &smtx,
    tile_shape_vec &vec,
    tile_shape_svec &svec);
template <
    int ScaleMask = 3,
    is_tile_data_v tile_shape_d,
    is_local_tile_v tile_shape_mtx,
    typename tile_shape_smtx,
    is_local_tile_v tile_shape_vec,
    typename tile_shape_svec,
    fixp::is_options_v Options>
PTO_SHARED_INLINE void TGEMV_MX(
    tile_shape_d &d,
    tile_shape_mtx &mtx,
    tile_shape_smtx &smtx,
    tile_shape_vec &vec,
    tile_shape_svec &svec,
    const Options &options);
template <is_tile_data_v D, is_local_tile_v Mtx, is_local_tile_v Vec, fixp::is_options_v Options>
requires(tile_role_v<Mtx> == Location::Right && tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(
    D &d,
    Mtx &mtx,
    Vec &vec,
    const Options &options);
template <
    is_tile_data_v D,
    is_local_tile_v Mtx,
    is_local_tile_v Vec,
    is_local_tile_v ScaleVec,
    fixp::is_options_v Options>
requires(tile_role_v<Mtx> == Location::Right && tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(
    D &d,
    Mtx &mtx,
    Vec &vec,
    ScaleVec &scale_vec,
    const Options &options);
template <
    is_tile_data_v D,
    is_local_tile_v Mtx,
    is_local_tile_v ScaleMtx,
    is_local_tile_v Vec,
    fixp::is_options_v Options>
requires(tile_role_v<Mtx> == Location::Right && tile_role_v<Vec> == Location::Left)
PTO_SHARED_INLINE void TGEMV_MX(
    D &d,
    Mtx &mtx,
    ScaleMtx &scale_mtx,
    Vec &vec,
    const Options &options);
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, Vec &vec);
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, Vec &vec, ScaleVec &scale_vec);
PTO_SHARED_INLINE void TGEMV_MX(D &d, Mtx &mtx, ScaleMtx &scale_mtx, Vec &vec);
```

### 支持的数据类型

支持FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E2M1X2、E1M2X2、S4X2、U4X2、S16、S8、U16、U8类型。

| 操作数角色 | 类型要求 |
| --- | --- |
| A / B 主输入 | 支持FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E2M1X2、E1M2X2、S4X2、U4X2、S16、S8、U16、U8类型。 A/B 必须属于该矩阵重载允许的数值类。 |
| C / D（累加器或结果） | 类型由该数值类和重载确定；不能仅因列在支持列表中就任意组合。 |
| Bias / Scale / 辅助输出 | 必须使用该重载规定的 dtype、shape 与 layout。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `d` | 输出 Tile；成功调用后写入操作结果。 |
| `mtx` | 矩阵 Tile 操作数。 |
| `smtx` | 矩阵操作数对应的缩放 Tile。 |
| `vec` | 向量 Tile 操作数。 |
| `svec` | 向量操作数对应的缩放 Tile。 |
| `options` | `fixp::Options` 选项对象；携带量化、激活、转置、缩放以及可选辅助输出配置。 |
| `tile_role_v<Mtx>` | 由矩阵 Tile 角色约束推导的操作数；必须匹配该重载要求的矩阵布局和位置。 |
| `scale_vec` | 向量缩放 Tile。 |
| `scale_mtx` | 矩阵缩放 Tile。 |

### 重载选择

- **基础重载**：不传 `options`，使用该操作的默认后处理属性。
- **带 `Options` 的重载**：需要量化、激活、转置、scale 或辅助输出时传入 `options`。它不是重复声明，而是在相同核心操作数上增加显式属性；仅可启用本操作支持的属性。详见 [fixp::Options 指南](../../options.md)。


## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

矩阵维度必须满足乘法关系（`M×K` 与 `K×N`，或对应 GEMV 形式）；A/B/D 的 CUBE layout、累加器类型和任何 scale/bias/options 必须构成该重载允许的组合。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | M/N/K 的有效维度必须与矩阵乘法关系一致；padding 不应被当作数学输入。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 以下是 C++ 声明中可直接省略的默认实参：

| 参数 | 默认值 |
| --- | --- |
| `tile_role_v<Mtx>` | `= Location::Right && tile_role_v<Vec> == Location::Left) PTO_SHARED_INLINE void TGEMV_MX(D &d` |

### 编码字段和省略值

- 数据类型编码始终使用 `AType`；省略 `B.DATR` 时，`BType` 沿用 `AType`，舍入模式使用 `RNE`，并关闭饱和处理。
- 省略 `LB0`、`LB1`、`LB2` 时，`M`、`N`、`K` 分别默认为 1，且 `TGEMV` 固定要求 `M=1`；显式给出的维度必须为正值。
- 全零 `B.FPATR` 表示不启用转换、激活和归约；只有后处理模式需要时才提供 `B.IOR` 或辅助 `B.IOT` 操作数。
- `TransA=0`、`TransB=0` 表示不转置；非零转置控制仅可用于规范允许的 `Shared` 主操作数。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TGEMV_MX` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TGEMVMX AType
B.DATR      BType, RMode, Sat (optional; BType defaults to AType)
B.FPATR     PreQuantMode, ReluMode, GroupNCode, RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn, TransA, TransB, CScaleEn (exactly one)
B.DIM       LB0 M (optional, default 1; TGEMV permits only M=1)
B.DIM       LB1 N (optional, default 1)
B.DIM       LB2 K (optional, default 1)
B.IOT       ordered Local mathematical sources: A CUBE_M16/M32 primary, optional A scale, B CUBE_N8 primary, optional B scale
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

void gemv_mx(D &d, Matrix &matrix, ScaleMatrix &scale_matrix, Vec &vec,
             ScaleVec &scale_vec) {
  TGEMV_MX(d, matrix, scale_matrix, vec, scale_vec);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。