# 复杂运算接口（TEPL Mode 3，BSTART.TEPL）

> 编码 family：BSTART.TEPL
> opcode = 96 + Function（Mode 3）
> 全部一层 inline-asm，接口名 = tileop 名

---

## 子 tile 提取/插入

### TEXTRACT — 提取子 tile

```cpp
// 从 src 的 (indexRow, indexCol) 位置提取子 tile 到 dst
TEXTRACT(dst, src, indexRow, indexCol);
```
- **签名**：`TEXTRACT(tile_shape_out &dst, tile_shape_in &src, int32_t indexRow, int32_t indexCol)`
- **opcode**：98（Func 2）
- indexRow/indexCol 通过 B.IOR 传入

### TINSERT — 插入子 tile

```cpp
// 将 src 插入 dst 的 (indexRow, indexCol) 位置
TINSERT(dst, src, indexRow, indexCol);
```
- **签名**：`TINSERT(tile_shape_out &dst, tile_shape_in &src, int32_t indexRow, int32_t indexCol)`
- **opcode**：99（Func 3）

---

## 数据变换

### TIMG2COL — feature-map 转 im2col

```cpp
TIMG2COL(dst, src);   // feature-map -> im2col 矩阵
```
- **签名**：`TIMG2COL(tile_shape_out &dst, tile_shape_in &src)`
- **opcode**：100（Func 4）

### TFILLPAD — 拷贝 valid region + 填充 padding

```cpp
TFILLPAD(dst, src);   // dst = copy(src valid region) + pad(PadValue)
```
- **签名**：`TFILLPAD(tile_shape_out &dst, tile_shape_in &src)`
- **opcode**：101（Func 5）

### TTRANS — tile 转置

```cpp
TTRANS(dst, src);   // dst = src^T
```
- **签名**：`TTRANS(tile_shape_out &dst, tile_shape_in &src)`
- **opcode**：110（Func 14）

---

## 生成类

### TCI — 连续整数序列

```cpp
// dst[i] = s + i（连续整数序列）
TCI(dst, s);
```
- **签名**：`TCI(tile_shape &dst, typename tile_shape::DType s)`
- **opcode**：102（Func 6）
- s 通过 B.IOR 传入

### TTRI — 三角 mask 生成

```cpp
TTRI(dst);   // 生成三角 mask tile
```
- **签名**：`TTRI(tile_shape &dst)`
- **opcode**：103（Func 7）

### TRANDOM — 随机数 tile 生成

```cpp
TRANDOM(dst, s);   // 基于 counter 的随机数生成
```
- **签名**：`TRANDOM(tile_shape &dst, typename tile_shape::DType s)`
- **opcode**：105（Func 9）
- s 为 counter/seed，通过 B.IOR 传入

---

## 量化

### TQUANT — 量化

```cpp
TQUANT(dst, src);   // profile-defined quantization
```
- **签名**：`TQUANT(tile_shape_out &dst, tile_shape_in &src)`
- **opcode**：106（Func 10）

### TDEQUANT — 反量化

```cpp
TDEQUANT(dst, src);   // profile-defined dequantization
```
- **签名**：`TDEQUANT(tile_shape_out &dst, tile_shape_in &src)`
- **opcode**：107（Func 11）

---

## 排序

### TSORT32 — 32 元素块排序

```cpp
TSORT32(dst, src);   // 对每个 32-element block 排序（带索引）
```
- **签名**：`TSORT32(tile_shape_out &dst, tile_shape_in &src)`
- **opcode**：108（Func 12）

### TMRGSORT — 合并已排序 list tile

```cpp
TMRGSORT(dst, src0, src1);   // 合并两个已排序 tile
```
- **签名**：`TMRGSORT(tile_shape &dst, tile_shape &src0, tile_shape &src1)`
- **opcode**：109（Func 13）

---

## Gather/Scatter（tile-tile 索引）

### TGATHER — 索引/mask tile gather

```cpp
// dst[i] = src[off[i]]
TGATHER(dst, src, off);
```
- **签名**：`TGATHER(tile_shape_out &dst, tile_shape_in &src, tile_shape_off &off)`
- **opcode**：111（Func 15）
- 注意：这是 tile-tile 的 gather（不同于 TLSU family 的 MGATHER，后者是 GM->tile）

### TSCATTER — 索引 tile scatter

```cpp
// dst[off[i]] = src[i]
TSCATTER(dst, src, off);
```
- **签名**：`TSCATTER(tile_shape_out &dst, tile_shape_in &src, tile_shape_off &off)`
- **opcode**：112（Func 16）

---

## Partial-valid 运算（从 Mode 0 迁移）

签名：`void NAME(tile_shape &dst, tile_shape &src0, tile_shape &src1)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TPARTADD(dst, s0, s1)` | 113 | partial-valid add |
| `TPARTMUL(dst, s0, s1)` | 114 | partial-valid multiply |
| `TPARTMAX(dst, s0, s1)` | 115 | partial-valid max |
| `TPARTMIN(dst, s0, s1)` | 116 | partial-valid min |

> 注意：这 4 个在旧规格中位于 Mode 0（opcode 28-31），在最新 DavinciOO 更新中迁移到 Mode 3（opcode 113-116）。

---

## 其他

### TCONCAT — 列拼接

```cpp
TCONCAT(c, a, b);   // c = [a | b] 列拼接
```
- **签名**：`TCONCAT(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1)`
- **opcode**：96（Func 0）
- **shape**：`dst = [src0 | src1]` 沿列维拼接。三者 shape **全部不同**：`src0` 为 `R×C0`、`src1` 为 `R×C1`、`dst` 为 `R×(C0+C1)`。`B.DIM` 的 `ValidCol/ValidRow/Col` 取 **dst** 的几何（描述 dst 的 valid region），不是 src0。
- **dtype**：`src0/src1/dst` 三者必须一致（编译期 `static_assert` 守门）。
- **行数**：`src0/src1/dst` 三者 `Rows` 必须相同（编译期 `static_assert` 守门）。

### TGATHERB — 字节偏移 tile gather

```cpp
TGATHERB(dst, src, offset);   // dst[i] = src[base + offset[i] (byte)]
```
- **签名**：`TGATHERB(tile_shape_out &dst, gm_shape &src, tile_shape_offset &offset)`
- **opcode**：97（Func 1）

### THISTOGRAM — 累积直方图

```cpp
THISTOGRAM(dst, src, idx, 0);   // ByteId=0
```
- **签名**：`THISTOGRAM(tile_shape_out &dst, tile_shape_in &src, tile_shape_in &Idx, int ByteId)`
- **opcode**：104（Func 8）
