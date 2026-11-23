# TMATMUL_MX_ACC

**矩阵与矩阵向量计算** · **CUBE** · 0.59 活动目录

## 简介

`TMATMUL_MX_ACC` 将经过缩放的矩阵相乘，并把结果累加到提供的累加器 Tile 中。

## 数学语义

逻辑上，`A[M,K] × B[K,N]` 得到 `C[M,N]`。MX 输入由各自的 scale 操作数解码，再按对应矩阵计算路径累加。以输入 accumulator 为累加初值，生成新的结果。 逻辑维度与物理 CELL 描述的对应关系以本页的操作数约束为准。

## C++ 接口

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
PTO_SHARED_INLINE void TMATMUL_MX_ACC(
    tile_shape_d &d,
    tile_shape_c &c,
    tile_shape_a &a,
    tile_shape_sa &scale_a,
    tile_shape_b &b,
    tile_shape_sb &scale_b,
    size_t groupM);
template <FixpAttr Attr = FixpAttr{}, is_tile_data_v tile_shape_d, is_tile_data_v tile_shape_c,
          is_local_or_shared_left tile_shape_a, typename tile_shape_sa,
          is_local_or_shared_right tile_shape_b, typename tile_shape_sb,
          fixp::is_options_v Options>
PTO_SHARED_INLINE void TMATMUL_MX_ACC(tile_shape_d &d, tile_shape_c &c, tile_shape_a &a,
                    tile_shape_sa &scale_a, tile_shape_b &b, tile_shape_sb &scale_b,
                    const Options &options, size_t groupM);
```

### 支持的数据类型

支持FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E2M1X2、E1M2X2、S4X2、U4X2、S16、S8、U16、U8类型。

| 操作数角色 | 类型要求 |
| --- | --- |
| A / B 主输入 | 支持FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E2M1X2、E1M2X2、S4X2、U4X2、S16、S8、U16、U8类型。；HiF4X2 仅允许 Matrix-MX。 |
| ScaleA / ScaleB | 与对应主输入独立校验；HiF4X2 为 `uint32_t`/group-64，其他 scaled MX 为 E8M0/group-32，storage 必须分别匹配 A/B。 |
| C / D（累加器或结果） | Matrix-MX 默认使用 FP32 累加器。 |
| Bias / 辅助输出 | 必须使用该重载规定的 dtype、shape 与 layout。 |

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `d` | 输出 Tile；成功调用后写入操作结果。 |
| `c` | 累加器或输出 Tile，具体角色由接口名称和重载决定。 |
| `a` | 左操作数或输入 Tile。 |
| `scale_a` | A 操作数的缩放 Tile。 |
| `b` | 右操作数或输入 Tile。 |
| `scale_b` | B 操作数的缩放 Tile。 |
| `options` | `fixp::Options` 选项对象；携带量化、激活、转置、缩放以及可选辅助输出配置。 |
| `groupM` | cooperative `Local-A/Shared-B` 场景下的 core-total `group_M`；必须是 `1..128` 的正值。 |
| `sa` | A 操作数对应的缩放或标量 Tile。 |
| `sb` | B 操作数对应的缩放或标量 Tile。 |

### 重载选择

- **基础重载**：不传 `options`，使用该操作的默认后处理属性。
- **带 `Options` 的重载**：需要量化、激活、转置、scale 或辅助输出时传入 `options`。它不是重复声明，而是在相同核心操作数上增加显式属性；仅可启用本操作支持的属性。参见[矩阵后处理选项](../helpers_zh.md#矩阵后处理选项)。

- **ScaleMask 是什么**：`ScaleMask` 是带 `Options` 的 MX 重载使用的编译期模板参数，用 2 个 bit 记录 A/B 主输入是否携带 MX scale tile。`bit0` 表示 ScaleA 是否存在，`bit1` 表示 ScaleB 是否存在：

  ```cpp
  constexpr bool HasScaleA = (ScaleMask & 1) != 0;  // bit0: A 有 scale
  constexpr bool HasScaleB = (ScaleMask & 2) != 0;  // bit1: B 有 scale
  ```

  | ScaleMask | 含义 | 发射的源流 |
  | --- | --- | --- |
  | 0 | A、B 都无 scale | `B.IOT A` + `B.IOT B` |
  | 1 | 仅 A 有 | `B.IOT A, ScaleA`（合在一条记录） + `B.IOT B` |
  | 2 | 仅 B 有 | `B.IOT A` + `B.IOT B, ScaleB`（合在一条记录） |
  | 3 | 都有 | 完整 `A, ScaleA, B, ScaleB` 源流（含 `B.IOS` 变体） |

  使用 `ScaleMask` 时，不要把它当作独立开关来随意裁剪参数。它必须与 A/B dtype 的 PTO MX contract 一致：普通 MX scaled 类型要求对应 bit 为 1，非 scaled 类型要求对应 bit 为 0；ScaleA/ScaleB 仍必须传入 `scale_a`/`scale_b` 参数位并满足 shape、dtype、layout 和 storage 约束。默认值 `3` 假设 A/B 都是 scaled MX 输入；A、B 只有部分为 scaled MX 类型时必须显式指定对应掩码。

- **带 `groupM` 的重载**（basic 与 `options`+`groupM` 两种形式）：仅用于 cooperative 的 `Local-A/Shared-B` 语义；`groupM` 显式提供 LB0 的 core-total `group_M`，而不是从 Local A shard 推导。`D`（以及 ACC 形式的 `C`）的 valid 行数必须等于 per-PE A shard 大小（`M_per_PE`：CubeM16 对应 `group_M <= 64`，CubeM32 对应 `group_M > 64`）。MX 形式的 scale 操作数与其主操作数存储一致（Local A 配 Local scale，Shared B 配 Shared scale）。

## 约束与默认值

矩阵 MX 的 A/B 主输入分别独立选择 scale contract；普通 MX 类型使用 E8M0/group-32，而 HiF4X2 只能用于 Matrix-MX，并使用 `uint32_t`/group-64。ScaleA/ScaleB 的 storage 必须分别跟随 A/B 主输入，但 physical shape 需区分 Local 与 Shared：Local ScaleA/ScaleB 使用 CUBE_M32，valid shape 分别为 `[M, G_A]` / `[N, G_B]`；Shared ScaleA/ScaleB 使用 RowMajor，physical shape 分别为 `[M, G_A]` / `[N, G_B]`，均不随对应的 `TransA`/`TransB` 改变。转置只作用于对应的 Shared primary matrix。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | M/N/K 的有效维度必须与矩阵乘法关系一致；padding 不应被当作数学输入。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

#### 编码字段和省略值

- 数据类型编码始终使用 `AType`；省略 `B.DATR` 时，`BType` 沿用 `AType`，`PreQuant=None` 与固定舍入（fixed floating/fixed shift 及 QF322*Pre 固定模式）使用 `RNONE`；仅可编程整数预量化模式保留 `RNE` 舍入选择；`Sat` 始终关闭。
- 省略 `LB0`、`LB1`、`LB2` 时，`M`、`N`、`K` 分别默认为 1；显式给出的维度必须为正值。
- 全零 `B.FPATR` 表示不启用转换、激活和归约；只有后处理模式需要时才提供 `B.IOR` 或辅助 `B.IOT` 操作数。
- `TransA=0`、`TransB=0` 表示不转置；非零转置控制仅可用于规范允许的 `Shared` 主操作数。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using A = CubeTileM16<__half, 16, 32>;
using B = CubeTileN8<__fp8_e4m3, 32, 16>; // local B is logical [K, N]
using Acc = CubeAccumulatorM16<float, 16, 16>;
using ScaleB = Tile<Location::Scaling, __fp8_e8m0, 32, 4,
                     BLayout::CubeM32, 16, 1>; // local scale: [N, ceil(K/32)]

void matmul_mx_acc(Acc &d, Acc &c, A &a, B &b, ScaleB &scale_b) {
  // d = c + a * dequant(b, scale_b)。
  TMATMUL_MX_ACC(d, c, a, b, scale_b);
}
```

## Bundle 对照

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

活动目录编码为 `CUBE`，function `6`。执行引擎为 `CUBE`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/matrix-and-matrix-vector/matrix-matrix/TMATMUL_MX_ACC.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/matrix-and-matrix-vector/matrix-matrix/TMATMUL_MX_ACC.asl)

[返回操作索引](../README.md)
