# 复杂运算接口（TEPL Mode 3，BSTART.TEPL）

> 编码 family：BSTART.TEPL
> opcode = 96 + Function（Mode 3）
> 全部一层 inline-asm，接口名 = tileop 名

---

## TCONCAT — 列拼接

```cpp
using tile_a = Tile<Location::Vec, float, 64, 32>;
using tile_b = Tile<Location::Vec, float, 64, 32>;
using tile_c = Tile<Location::Vec, float, 64, 64>;
tile_a a; tile_b b; tile_c c;
TCONCAT(c, a, b);   // c = [a | b] 列拼接
```

- **签名**：`TCONCAT(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1)`
- **opcode**：96
- **语义**：将 src0、src1 按列拼接为 dst

---

## TGATHERB — 字节偏移 tile gather

```cpp
using gm = global_tensor<float, RowMajor<64, 32>>;
using tile_t = Tile<Location::Vec, float, 64, 32>;
using tile_off = Tile<Location::Vec, uint32_t, 64, 32>;
gm src(data_ptr);
tile_t dst; tile_off offset;
TCOPYIN(offset, *(new global_tensor<uint32_t, RowMajor<64,32>>(idx_ptr)));
TGATHERB(dst, src, offset);   // dst[i] = src[base + offset[i] (byte)]
```

- **签名**：`TGATHERB(tile_shape_out &dst, gm_shape &src, tile_shape_offset &offset)`
- **opcode**：97
- **语义**：以字节偏移方式从 GM 离散 gather 到 tile（DavinciOO ext）

---

## THISTOGRAM — 累积直方图统计

```cpp
using tile_t = Tile<Location::Vec, uint32_t, 64, 32>;
tile_t dst, src, idx;
// TCOPYIN src, idx ...
THISTOGRAM(dst, src, idx, 0);   // ByteId=0
```

- **签名**：`THISTOGRAM(tile_shape_out &dst, tile_shape_in &src, tile_shape_in &Idx, int ByteId)`
- **opcode**：104（Mode 3, Function 8）
- **语义**：对 src 按 Idx 的字节（ByteId 0~3）做累积直方图统计
- **注意**：该接口在 `template_asm.hpp` 中已存在（非 TEPL 模板编号格式，用 `BSTART.TEPL 0b1101000`）
