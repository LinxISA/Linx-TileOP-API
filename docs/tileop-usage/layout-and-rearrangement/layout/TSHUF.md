# TSHUF

`TSHUF` 按显式 GPR control，在独立的 2 的幂次 CUBE 行段中重排原始 32-bit word group。
该操作只重排行/word group，不执行逐字节 permutation 或数值转换。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v S, is_tile_data_v C>
void TSHUF(D &dst, S &src, C &controls, uint64_t control);
```

### 支持的数据类型

数据 Tile 支持 FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、
E2M1X2、E1M2X2、S4X2、U4X2、S32、S16、S8、U32、U16、U8 类型。`controls` 固定使用
U32。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的输出数据 Tile。 |
| `src` | 输入数据 Tile。 |
| `controls` | U32 control Tile；每个 word 的低 5 bit 提供对应行操作数，高 27 bit 被忽略。 |
| `control` | 标量 shuffle control，选择 mode、segment width 和越界行为。 |

## 约束

- 三个 Tile 都必须是 Local 且使用相同的 `CUBE_M16` 或 `CUBE_M32` layout。
- `dst` 与 `src` 的 dtype 和几何必须相同；支持 CUBE dtype 中除 64-bit 元素之外的类型。
- `controls` 必须是 U32，并与数据 Tile 具有相同 layout、有效行数和 CUBE CELL 数；其有效列数等于每个数据行中的 32-bit word group 数。
- `control` 的 mode 可选择 `UP`、`DOWN`、`BFLY` 或 `IDX`；segment code 最大为 4，boundary 仅允许 `SELF` 或 `ZERO`，bits `[63:32]` 必须为零。
- segment width 32 只适用于 `CUBE_M32`。
- `dst` 必须是新目标，不得与输入 alias；当前 wrapper 要求编译期已知且非零的 destination valid shape。

输出 padding 为 `Null`；源与 control Tile 保持不变，操作没有内存副作用。

## Bundle 组成

```asm
BSTART.SFU TSHUF, DataType           ; TEPL selector 118 / 0x076
B.DATR      Layout (optional)
B.DIM       LB0/LB1/LB2 (optional)
B.IOT       source, controls, ->destination
B.IOR       shuffle_control
BSTOP
```

## 使用示例

```cpp
#include <jcore/template_asm.hpp>

using namespace pto;
using Words = VecTileM16<uint32_t, 16, 32>;

void shuffle_words(Words &dst, Words &src, Words &controls,
                   uint64_t shuffle_control) {
  TSHUF(dst, src, controls, shuffle_control);
}
```

调用者必须按 PTO ISA v0.58.5 的 shuffle-control 位域构造 `shuffle_control`；保留位或
不支持的 mode/segment/boundary 组合会在目标发布前被拒绝。