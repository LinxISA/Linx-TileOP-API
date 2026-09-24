# TUNPACK

`TUNPACK` 从 Local CUBE 输入的有效 raw 32-bit word 中按 B32 group 提取连续的原始字节字段，
将字段右对齐并零扩展到目标 carrier。它不进行数值转换或符号扩展。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v S>
void TUNPACK(D &dst, S &src, uint64_t control);
```

### 支持的数据类型

`dst` 的 operation type 可以是 U8、U16 或 U32。`src` 可以使用已有的 Local numeric
U8、U16 或 U32 backing type。源和目标都必须使用 `CUBE_M16` 或 `CUBE_M32`。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的 U8、U16 或 U32 输出 Tile；其类型是 BSTART operation type。 |
| `src` | Local U8/U16/U32 numeric CUBE 输入 Tile。 |
| `control` | `control[7:0]` 是每个 raw word 的源 byte offset，`control[15:8]` 是提取 byte count；运行时会拒绝高 32 位非零或 `offset + count > 4` 的 control。 |

## 约束

- `src` 必须是 Local U8/U16/U32 numeric CUBE Tile；`dst` 必须是 Local U8/U16/U32 CUBE Tile，并与源使用相同 layout 和 `valid_rows`。
- 每个有效源行的字节数 `src.valid_columns * bytes(src.type)` 必须是 4 的整数倍。每 4 字节形成一个 raw word，因此 `dst.valid_columns = raw_words * (4 / bytes(dst.type))`，而不一定等于源列数。
- `control[7:0]` 是源 byte offset，必须在 `0..3`；`control[15:8]` 是 byte count，必须在 `1..4`；并且 `offset + count <= 4`。字段写入目标 raw word 的低字节，目标 U8/U16/U32 只决定目标元素解释和几何。
- `control[63:32]` 必须为零；非法 control 会在进入 Tile bundle 前通过 `__builtin_trap()` fail closed，不发布 destination，也不产生部分 Tile 副作用。
- `dst` 必须是新目标，不得与 `src` alias；当前 wrapper 要求编译期已知且非零的 destination valid shape。
字段选择在每个 raw word 中重新开始，不从 padding 读取数据。源和目标的 physical CELL 数量可以不同。

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
using Words = CubeTileM16<uint32_t, 16, 32>;

void unpack_middle_16_bits(Words &dst, Words &src) {
  constexpr uint64_t offset_one_count_two = 0x00000201;
  TUNPACK(dst, src, offset_one_count_two);
}
```

例如源 word 为 `0x44332211` 时，该 control 从 byte offset 1 提取 2 bytes，结果为
`0x00003322`。历史 U32-to-U32 形式保持 bit-exact；对于 U16 source，每两个有效
U16 元素组成一个 B32 group，字段选择在每个 group 中重新开始。