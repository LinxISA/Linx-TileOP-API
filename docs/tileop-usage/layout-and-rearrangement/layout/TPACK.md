# TPACK

`TPACK` 从两个 Local CUBE 源的对应原始 32-bit word 中提取低位原始字节字段，并将它们
拼接到目标 word；未被字段覆盖的高位补零。它不进行数值转换、符号扩展或饱和处理。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B>
void TPACK(D &dst, A &src0, B &src1, uint64_t control);
```

### 支持的数据类型

`dst` 的 operation type 可以是 U8、U16 或 U32。`src0` 和 `src1` 可以分别使用已有的
Local numeric U8、U16 或 U32 backing type，且两个源的 backing type 可以不同。源和目标
都必须使用 `CUBE_M16` 或 `CUBE_M32`；不会新增 U24 或其它 public carrier DataType。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的 U8、U16 或 U32 输出 Tile；其类型是 BSTART operation type。 |
| `src0` | 第一个 Local CUBE 输入；低 `W0` 个原始字节放在结果低位。 |
| `src1` | 第二个 Local CUBE 输入；低 `W1` 个原始字节紧随 `src0` 字段。 |
| `control` | `control[7:0] = W0`、`control[15:8] = W1`。运行时会拒绝高 32 位非零、超过源 backing 宽度或超过 32-bit raw word 宽度的 control。 |

## 约束

- `src0` 和 `src1` 必须是 Local U8/U16/U32 numeric Tile，使用相同的 `CUBE_M16` 或 `CUBE_M32` layout、`valid_rows` 和有效 raw-word 数。源的 logical `valid_columns` 可以不同。
- 每个源的有效行字节数必须是 4 的整数倍；两个源的每行 raw-word 数必须相同。`dst` 必须是 Local U8/U16/U32，并使用共同的 CUBE layout、`valid_rows`，其 `valid_columns` 为 `raw_words * (4 / bytes(dst.type))`。
- `control[7:0]` 是 `src0` 字段宽度，`control[15:8]` 是 `src1` 字段宽度；两者都必须至少为 1，且分别不超过对应源的 backing 宽度，宽度之和不能超过一个 32-bit raw word。目标 U8/U16/U32 只决定目标元素解释和几何，不改变 raw word 控制宽度。
- `control[63:32]` 必须为零；非法 control 会在进入 Tile bundle 前通过 `__builtin_trap()` fail closed，不发布 destination，也不产生部分 Tile 副作用。
- `dst` 必须是新目标，不得与任一源 alias；当前 wrapper 要求编译期已知且非零的 destination valid shape。
- 操作按每个有效 raw word 独立进行，并按对应的 raw CELL 位置处理（CUBE_M32 每行一个 word，CUBE_M16 每行两个 word）；不读取 padding。控制值 `0x00000202` 表示从两个源各取 2 bytes。

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
using Words = CubeTileM16<uint32_t, 16, 32>;

void pack_16bit_fields(Words &dst, Words &src0, Words &src1) {
  constexpr uint64_t two_bytes_from_each = 0x00000202;
  TPACK(dst, src0, src1, two_bytes_from_each);
}
```

例如 `src0=0x00001234`、`src1=0x00ABCDEF` 时，该 control 产生 `0xCDEF1234`。

`U8 + U16 -> U32` 也是合法形式。例如 `control = 0x00000201` 时，目标低 24 位由
`src0` 的 1 个低字节和 `src1` 的 2 个低字节组成。