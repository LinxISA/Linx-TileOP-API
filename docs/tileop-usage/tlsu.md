# 数据搬运接口(TLSU family,BSTART.TLSU)

> 编码 family:BSTART.TLSU(原 BSTART.TMA,已改名对齐 DavinciOO,编码不变)
> 涉及 GM ↔ Tile 的数据搬运、离散地址 gather/scatter

---

## TLOAD — GM → Tile

```cpp
#include <common/pto_tileop.hpp>
using namespace pto;

void load_example(float* gm_data) {
  using gm = global_tensor<float, RowMajor<64, 32>>;
  using tile_t = Tile<Location::Vec, float, 64, 32>;
  gm src(gm_data);
  tile_t d;
  TLOAD(d, src);            // 把 GM 数据搬进 tile 缓冲区
}
```

- **签名**:`TLOAD(tile_shape &dst, gm_shape &src)`
- **dst**:已声明的 tile;**src**:`global_tensor` 视图
- **生成**:`BSTART.TLSU TLOAD, <dtype>` + `B.IOT [], last, ->dst<size>` + `B.IOR [base, stride]`

---

## TSTORE — Tile → GM

```cpp
void store_example(float* gm_out, tile_t& d) {
  using gm = global_tensor<float, RowMajor<64, 32>>;
  gm dst(gm_out);
  TSTORE(dst, d);           // 把 tile 数据写回 GM
}
```

- **签名**:`TSTORE(gm_shape &dst, tile_shape &src)`
- **生成**:`BSTART.TLSU TSTORE` + `B.IOT [src], last` + `B.IOR [base, stride]`

---

## MGATHER — 离散地址 gather

```cpp
void gather_example(float* gm_src, uint32_t* gm_idx, tile_t& d) {
  using gm = global_tensor<float, RowMajor<64, 32>>;
  using gm_idx = global_tensor<uint32_t, RowMajor<64, 32>>;
  using tile_idx = Tile<Location::Vec, uint32_t, 64, 32>;
  gm src(gm_src);
  gm_idx idx(gm_idx);
  tile_idx off;
  TCOPYIN(off, idx);        // 先把索引搬进 tile
  MGATHER(d, src, off);     // dst[i] = src[base + off[i]]
}
```

- **签名**:`MGATHER(dst, src, offset)`
- **offset**:tile 形式的索引(先 TCOPYIN),不能直接传普通指针
- **生成**:`BSTART.TLSU MGATHER` + `B.DATR Null` + `B.DIM(ValidCol/ValidRow/Col)` + `B.IOT [off], last, ->dst<size>` + `B.IOR [base, stride]`

---

## MSCATTER — 离散地址 scatter

```cpp
void scatter_example(float* gm_dst, uint32_t* gm_idx, tile_t& d) {
  using gm = global_tensor<float, RowMajor<64, 32>>;
  using gm_idx = global_tensor<uint32_t, RowMajor<64, 32>>;
  using tile_idx = Tile<Location::Vec, uint32_t, 64, 32>;
  gm dst(gm_dst);
  gm_idx idx(gm_idx);
  tile_idx off;
  TCOPYIN(off, idx);
  MSCATTER(dst, d, off);    // dst[base + off[i]] = src[i]
}
```

- **签名**:`MSCATTER(dst, src, offset)`
- **生成**:`BSTART.TLSU MSCATTER` + `B.DIM(ValidCol/ValidRow/Col)` + `B.IOT [src, off], last` + `B.IOR [base, stride]`

---

## MGATHER_MASK — 带 mask 的 gather

```cpp
void masked_gather_example(float* gm_src, uint32_t* gm_idx, uint8_t* gm_msk,
                           tile_t& d) {
  using gm = global_tensor<float, RowMajor<64, 32>>;
  using gm_idx = global_tensor<uint32_t, RowMajor<64, 32>>;
  using gm_m = global_tensor<uint8_t, RowMajor<64, 32>>;
  using tile_idx = Tile<Location::Vec, uint32_t, 64, 32>;
  using tile_m = Tile<Location::Vec, uint8_t, 64, 32>;
  gm src(gm_src);
  gm_idx idx(gm_idx); gm_m msk(gm_msk);
  tile_idx off; tile_m mask;
  TCOPYIN(off, idx); TCOPYIN(mask, msk);
  MGATHER_MASK(d, src, off, mask);    // mask[i]=0 的 lane 填 PadValue
}
```

- **签名**:`MGATHER_MASK(dst, src, offset, mask)`
- **生成**:`BSTART.TLSU MGATHER.MASK` + `B.DATR Null` + `B.DIM(ValidCol/ValidRow/Col)` + `B.IOT [off], ->dst; [mask], last` + `B.IOR [base, stride]`

---

## MSCATTER_MASK — 带 mask 的 scatter

```cpp
void masked_scatter_example(float* gm_dst, uint32_t* gm_idx, uint8_t* gm_msk,
                            tile_t& d) {
  using gm = global_tensor<float, RowMajor<64, 32>>;
  using gm_idx = global_tensor<uint32_t, RowMajor<64, 32>>;
  using gm_m = global_tensor<uint8_t, RowMajor<64, 32>>;
  using tile_idx = Tile<Location::Vec, uint32_t, 64, 32>;
  using tile_m = Tile<Location::Vec, uint8_t, 64, 32>;
  gm dst(gm_dst);
  gm_idx idx(gm_idx); gm_m msk(gm_msk);
  tile_idx off; tile_m mask;
  TCOPYIN(off, idx); TCOPYIN(mask, msk);
  MSCATTER_MASK(dst, d, off, mask);   // mask[i]=0 的 lane 不写
}
```

- **签名**:`MSCATTER_MASK(dst, src, offset, mask)`
- **生成**:`BSTART.TLSU MSCATTER.MASK` + `B.DIM(ValidCol/ValidRow/Col)` + `B.IOT [src, off]; [mask], last` + `B.IOR [base, stride]`
