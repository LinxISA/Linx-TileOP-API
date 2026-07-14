# 归约与广播接口（TEPL Mode 2，BSTART.TEPL）

> 编码 family：BSTART.TEPL
> opcode = 64 + Function（Mode 2）
> 全部一层 inline-asm，接口名 = tileop 名

---

## 行归约（1 src，dst 为归约结果）

签名：`void NAME(tile_shape_out &dst, tile_shape_in &src)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TROWSUM(dst, src)` | 64 | 对列求和归约每一行 |
| `TROWMAX(dst, src)` | 65 | 取列间最大值归约每一行 |
| `TROWMIN(dst, src)` | 66 | 取列间最小值归约每一行 |
| `TROWPROD(dst, src)` | 67 | 跨列乘积归约每一行 |
| `TROWEXPAND(dst, src)` | 68 | 将每行第一个元素广播到目标行 |
| `TROWARGMAX(dst, src)` | 76 | 行 argmax（DavinciOO ext） |
| `TROWARGMIN(dst, src)` | 77 | 行 argmin（DavinciOO ext） |

```cpp
// 示例：行求和
using tile_in = Tile<Location::Vec, float, 64, 32>;
using tile_out = Tile<Location::Vec, float, 64, 1>;
tile_in src; tile_out dst;
TROWSUM(dst, src);   // dst[i] = sum(src[i, :])
```

---

## 行广播运算（2 src，dst = 广播运算结果）

签名：`void NAME(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1)`

src0 与 dst 同形状（方阵 `R×C`）；src1 为每行标量向量（PTO Mode 1：`R×1` 每行一个标量；
PTO Mode 2：每行 32B 数据条），**shape 可与 src0 不一致**；src0/src1/dst 三者
**dtype 必须一致**（编译期 `static_assert` 守门）。`B.DIM` 的 `ValidCol/ValidRow/Col`
均取 src0 的几何，描述 dst 的 valid region。

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TROWEXPANDADD(dst, s0, s1)` | 69 | 行广播加法：dst = s0 + 每行标量 s1 |
| `TROWEXPANDSUB(dst, s0, s1)` | 70 | 行广播减法 |
| `TROWEXPANDMUL(dst, s0, s1)` | 71 | 行广播乘法 |
| `TROWEXPANDDIV(dst, s0, s1)` | 72 | 行广播除法 |
| `TROWEXPANDMAX(dst, s0, s1)` | 73 | 行广播最大值 |
| `TROWEXPANDMIN(dst, s0, s1)` | 74 | 行广播最小值 |
| `TROWEXPANDEXPDIF(dst, s0, s1)` | 75 | 行指数差：dst = exp(s0 - s1) |

```cpp
// 示例：行广播乘法，src1 为每行标量向量（shape 与 src0 不同）
using tile_mat = Tile<Location::Vec, __fp32, 16, 16, BLayout::RowMajor>;
using tile_row = Tile<Location::Vec, __fp32, 16, 1,  BLayout::RowMajor>;
tile_mat s0, dst;
tile_row s1(16);            // 每行一个标量
TROWEXPANDMUL(dst, s0, s1); // dst[i,j] = s0[i,j] * s1[i,0]
```

---

## 列归约（1 src，dst 为归约结果）

签名：`void NAME(tile_shape_out &dst, tile_shape_in &src)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TCOLSUM(dst, src)` | 80 | 对行求和归约每一列 |
| `TCOLMAX(dst, src)` | 81 | 取行间最大值归约每一列 |
| `TCOLMIN(dst, src)` | 82 | 取行间最小值归约每一列 |
| `TCOLPROD(dst, src)` | 83 | 跨行乘积归约每一列 |
| `TCOLEXPAND(dst, src)` | 84 | 将每列第一个元素广播到目标列 |
| `TCOLARGMAX(dst, src)` | 92 | 列 argmax（DavinciOO ext） |
| `TCOLARGMIN(dst, src)` | 93 | 列 argmin（DavinciOO ext） |

```cpp
// 示例：列最大值
tile_in src; tile_out dst;
TCOLMAX(dst, src);   // dst[j] = max(src[:, j])
```

---

## 列广播运算（2 src）

签名：`void NAME(tile_shape_out &dst, tile_shape_in &src0, tile_shape_in &src1)`

src1 为每列标量向量，广播到 src0 的各列。

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TCOLEXPANDADD(dst, s0, s1)` | 85 | 列广播加法 |
| `TCOLEXPANDSUB(dst, s0, s1)` | 86 | 列广播减法 |
| `TCOLEXPANDMUL(dst, s0, s1)` | 87 | 列广播乘法 |
| `TCOLEXPANDDIV(dst, s0, s1)` | 88 | 列广播除法 |
| `TCOLEXPANDMAX(dst, s0, s1)` | 89 | 列广播最大值 |
| `TCOLEXPANDMIN(dst, s0, s1)` | 90 | 列广播最小值 |
| `TCOLEXPANDEXPDIF(dst, s0, s1)` | 91 | 列指数差：dst = exp(s0 - s1) |
