# TGPR2T

**布局与重排** · **SFU** · 0.59 活动目录

## 简介

`TGPR2T` 将四个 64-bit GPR predicate plane carrier 重编码到普通数值 U8 CUBE Tile。

## 数学语义

将四个 GPR predicate plane 的位模式写成数值 U8 Tile。

## C++ 接口

```cpp
template <is_tile_data_v D>
void TGPR2T(D &dst, uint64_t gpr0, uint64_t gpr1, uint64_t gpr2,
            uint64_t gpr3);
```

### 支持的数据类型

仅支持 U8。`dst` 必须是 U8 Tile；`gpr0` 至 `gpr3` 是 `uint64_t` 标量，不是 Tile 数据类型。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的普通数值 U8 CUBE 输出 Tile。 |
| `gpr0..gpr3` | 四个有序、只读的 64-bit predicate plane carrier。 |

## 约束与默认值

- `dst` 必须是 Local U8，且 valid shape/layout 只能是 `CUBE_M32 32x4` 或 `CUBE_M16 16x8`。
- 四个 GPR 的顺序有语义，不得交换或省略。wrapper 将其编码为两个立即连续的 source-only `B.IOR`，arity 必须为 `3+1`。
- `CUBE_M32` 将每 32 行的 8 个 predicate bit 打包到一个选定 U8 byte；`CUBE_M16` 将每 16 行打包到两个选定 U8 byte。
- 当前 C++ wrapper 使用全 PE mask，并采用省略 `B.DATR` 的默认行为：Zero padding、ByteOffset0。
- 操作不读取旧 destination payload，不修改 GPR 或数值状态，也没有内存副作用。

### 默认值

省略 `B.DATR` 时选择 Zero padding 和 ByteOffset0。ISA 只允许 Zero 或 Max whole-tile
padding；当前接口没有暴露非默认 padding/byte-offset 参数。

## 示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using PredicateBytes = VecTileM16<uint8_t, 16, 8>;

void predicate_planes_to_tile(PredicateBytes &dst, uint64_t plane0,
                              uint64_t plane1, uint64_t plane2,
                              uint64_t plane3) {
  TGPR2T(dst, plane0, plane1, plane2, plane3);
}
```

`CUBE_M32` 形式应改用 `VecTileM32<uint8_t, 32, 4>`；其他 shape 会被编译期拒绝。

## Bundle 对照

```asm
BSTART.SFU TGPR2T, U8                ; TEPL selector 126 / 0x07E
B.DATR      PadValueOrByteId, RMode (optional)
B.DIM       LB0=ValidCol
B.DIM       LB1=ValidRow
B.IOR       GPR0, GPR1, GPR2
B.IOR       GPR3
B.IOT       mask=PE_MASK, last, ->destination<TSize>
BSTOP
```

活动目录编码为 `TEPL`，mode `3`，function `30`，selector `0x07E`。执行引擎为 `SFU`。

## 参考

[PTO ISA 操作规范](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/docs/tile/layout-and-rearrangement/layout/TGPR2T.md) · [规范 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/tile/layout-and-rearrangement/layout/TGPR2T.asl)

[返回操作索引](../README.md)
