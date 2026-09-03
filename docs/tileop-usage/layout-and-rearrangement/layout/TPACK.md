# TPACK

`TPACK` 从两个对应的 Local U32 CUBE word 中提取低位字节字段，并将它们拼接为一个
U32 结果；未被字段覆盖的高位补零。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TPACK(D &dst, A &src0, B &src1, uint64_t control);
```

### 支持的数据类型

仅支持 U32。`dst`、`src0` 和 `src1` 都必须是 U32 Tile。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的 U32 输出 Tile。 |
| `src0` | 第一个 U32 输入；其低位字段放在结果低字节。 |
| `src1` | 第二个 U32 输入；其低位字段紧随 `src0` 字段。 |
| `control` | 低两个 control byte 分别给出 `src0`、`src1` 字段宽度。 |

## 约束

- 所有 Tile 必须是 Local U32，使用相同的 `CUBE_M16` 或 `CUBE_M32` layout 和几何。
- 两个字段宽度都必须在 `1..3` bytes，且宽度之和不能超过 4 bytes；control bits `[63:32]` 必须为零。
- `dst` 必须是新目标，不得与任一源 alias；当前 wrapper 要求编译期已知且非零的 destination valid shape。
- 操作按原始字节拼接，不进行数值转换。控制值 `0x00000202` 表示从两个源各取 2 bytes。

输出 padding 为 `Null`；输入保持不变，操作没有内存副作用。

## Bundle 组成

```asm
BSTART.SFU TPACK, U32                ; TEPL selector 119 / 0x077
B.DATR      Layout (optional)
B.DIM       LB0/LB1/LB2 (optional)
B.IOT       source0, source1, ->destination
B.IOR       pack_control
BSTOP
```

## 使用示例

```cpp
#include <jcore/template_asm.hpp>

using namespace pto;
using Words = VecTileM16<uint32_t, 16, 32>;

void pack_16bit_fields(Words &dst, Words &src0, Words &src1) {
  constexpr uint64_t two_bytes_from_each = 0x00000202;
  TPACK(dst, src0, src1, two_bytes_from_each);
}
```

例如 `src0=0x00001234`、`src1=0x00ABCDEF` 时，该 control 产生 `0xCDEF1234`。