# TUNPACK

`TUNPACK` 从每个 Local U32 CUBE word 中提取连续的原始字节字段，并将字段右对齐、
零扩展为 U32 结果。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v S>
void TUNPACK(D &dst, S &src, uint64_t control);
```

### 支持的数据类型

仅支持 U32。`dst` 和 `src` 都必须是 U32 Tile。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的 U32 输出 Tile。 |
| `src` | U32 输入 Tile。 |
| `control` | 低 control byte 是源 byte offset，下一 byte 是提取 byte count。 |

## 约束

- `src` 与 `dst` 必须是 Local U32，使用相同的 `CUBE_M16` 或 `CUBE_M32` layout 和几何。
- offset 必须在 `0..3`，count 必须在 `1..4`，且 `offset + count <= 4`；control bits `[63:32]` 必须为零。
- `dst` 必须是新目标，不得与 `src` alias；当前 wrapper 要求编译期已知且非零的 destination valid shape。
- 操作按原始字节提取，不进行符号扩展或数值转换。

输出 padding 为 `Null`；输入保持不变，操作没有内存副作用。

## Bundle 组成

```asm
BSTART.SFU TUNPACK, U32              ; TEPL selector 120 / 0x078
B.DATR      Layout (optional)
B.DIM       LB0/LB1/LB2 (optional)
B.IOT       source, ->destination
B.IOR       unpack_control
BSTOP
```

## 使用示例

```cpp
#include <jcore/template_asm.hpp>

using namespace pto;
using Words = VecTileM16<uint32_t, 16, 32>;

void unpack_middle_16_bits(Words &dst, Words &src) {
  constexpr uint64_t offset_one_count_two = 0x00000201;
  TUNPACK(dst, src, offset_one_count_two);
}
```

例如源 word 为 `0x44332211` 时，该 control 从 byte offset 1 提取 2 bytes，结果为
`0x00003322`。