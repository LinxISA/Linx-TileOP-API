# TMATMUL

TMATMUL multiplies matrices into one newly published destination Tile.

## C++ 接口

当前 API 中可用的调用形式：

```cpp
PTO_SHARED_INLINE void TMATMUL(tile_shape_c &c, tile_shape_a &a, tile_shape_b &b);
template <
    is_tile_data_v tile_shape_d,
    is_local_or_shared_left tile_shape_a,
    is_local_or_shared_right tile_shape_b,
    fixp::is_options_v Options>
__attribute__((always_inline)) inline void TMATMUL(
    tile_shape_d &d,
    tile_shape_a &a,
    tile_shape_b &b,
    const Options &options);
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

成功调用后，`TMATMUL` 按操作语义更新输出 Tile。padding、输入持久性、边界行为及数值状态影响请以 PTO-SPEC 为准；未明确声明的副作用不应被假定。

## Bundle composition

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TMATMUL AType
B.DATR      BType, RMode, Sat (optional; BType defaults to AType)
B.FPATR     PreQuantMode, ReluMode, GroupNCode, RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn, TransA, TransB, CScaleEn (exactly one)
B.DIM       LB0 M or cooperative group_M (optional, default 1)
B.DIM       LB1 N (optional, default 1)
B.DIM       LB2 K (optional, default 1)
B.IOS       complete right or both matrix operand groups (optional; cooperative mask 1111)
B.IOT       ordered Local mathematical sources: A CUBE_M16/M32 primary, B CUBE_N8 primary
B.IOT       D matching A's CUBE_M16/M32 layout, optional RowMaxOut, optional GroupMaxOut destinations
B.IOT/B.IOR postprocess operands selected by B.FPATR
BSTOP       or the next BSTART completion boundary
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

constexpr int M = 32;
constexpr int K = 32;
constexpr int N = 32;

using GM = global_tensor<float, RowMajor<32, 32>>;
using A = CubeTileM32<float, M, K>;
using B = CubeTileN8<float, K, N>;
using C = CubeAccumulatorM32<float, M, N>;

void matmul(float *a_data, float *b_data, float *c_data) {
  GM a_global(a_data);
  GM b_global(b_data);
  GM c_global(c_data);
  A a;
  B b;
  C c;

  TLOAD_CUBE(a, a_global);
  TLOAD_CUBE(b, b_global);
  TMATMUL(c, a, b);
  TSTORE_CUBE(c_global, c);
}
```

示例计算 `C[M][N] = A[M][K] * B[K][N]`。`TLOAD_CUBE` 和 `TSTORE_CUBE` 用于 CUBE Tile 与 Global Memory 之间的数据传输。

## 完整语义

完整语义、约束、默认值、异常和边界行为请参阅 [`TMATMUL.md`](https://github.com/PTO-ISA/pto-spec/blob/v0.58.4.1/docs/tile/matrix-and-matrix-vector/matrix-matrix/TMATMUL.md)。
