# TPERMUTE

`TPERMUTE` 使用一个 Local U8 索引 Tile，在两个 Local CUBE 源 Tile 之间按原始字节选择数据。
查表范围在每个 128-byte CUBE CELL 重新开始，不进行数值类型转换。

## C++ 接口

```cpp
template <is_tile_data_v D, is_tile_data_v A, is_tile_data_v B, is_tile_data_v I>
void TPERMUTE(D &dst, A &src0, B &src1, I &indices);
```

### 支持的数据类型

数据 Tile 支持 FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、
E2M1X2、E1M2X2、S4X2、U4X2、S32、S16、S8、U32、U16、U8 类型。`indices` 固定使用
U8。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 新的输出数据 Tile。 |
| `src0` | 第一个数据源。 |
| `src1` | 第二个数据源；可以与 `src0` alias。 |
| `indices` | U8 字节索引 Tile；为每个有效目标字节提供一个索引。 |

## 约束

- 四个 Tile 都必须是 Local `CUBE_M16` 或 `CUBE_M32`，且 layout 相同。
- `dst`、`src0` 和 `src1` 的 dtype 与几何必须相同；支持 CUBE dtype 中除 64-bit 元素之外的类型。
- `indices` 的 dtype 必须是 U8。它按**字节**描述目标，因此逻辑 shape 通常不同于数据 Tile：每个有效目标元素需要 `sizeof(D::DType)` 个索引元素。
- 例如 U32 `CUBE_M16` 数据 Tile 为 `16x32` 时，对应的 U8 索引 Tile 为 `16x128`。
- `CUBE_M32` 的每源 CELL 表宽为 4 bytes，合法索引为 `0..7`；`CUBE_M16` 的每源 CELL 表宽为 8 bytes，合法索引为 `0..15`。前半范围选择 `src0`，后半范围选择 `src1`。
- `dst` 必须是新目标；`indices` 不得与任一数据源 alias。非法索引必须在发布目标前被拒绝。
- 当前 C++ wrapper 要求编译期已知且非零的 destination valid shape。

输出 padding 为 `Null`；输入保持不变，操作没有内存副作用。

## 默认值

接口没有默认实参。`B.DATR` 只选择 `CUBE_M16`/`CUBE_M32` layout，其余字段为零。

## Bundle 组成

```asm
BSTART.SFU TPERMUTE, DataType        ; TEPL selector 117 / 0x075
B.DATR      Layout (optional)
B.DIM       LB0/LB1/LB2 (optional)
B.IOT       source0, source1
B.IOT       indices, ->destination
BSTOP
```

## 使用示例

```cpp
#include <jcore/template_asm.hpp>

using namespace pto;
using Data = VecTileM16<uint32_t, 16, 32>;
using Indices = VecTileM16<uint8_t, 16, 128>;

void permute_bytes(Data &dst, Data &src0, Data &src1, Indices &indices) {
  TPERMUTE(dst, src0, src1, indices);
}
```

示例中的 `B.DIM` 描述目标数据 Tile 的 `16x32` 几何；索引 Tile 的 `16x128` shape
来自“每个目标字节一个索引”，不能把它误写成数据 Tile shape。