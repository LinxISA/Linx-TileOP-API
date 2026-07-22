# 归约与广播接口（TEPL Mode 2，BSTART.TEPL）

> 编码 family：BSTART.TEPL
> opcode = 64 + Function（Mode 2）
> 全部一层 inline-asm，接口名 = tileop 名

---

## 行归约（1 src，dst 为归约结果）

签名：`void NAME(tile_shape_out &dst, tile_shape_in &src)`

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TROWSUM` | 64 | 无(inline-asm) | `BSTART.TEPL trowsum` | 对列求和归约每一行 |
| `TROWMAX` | 65 | 无(inline-asm) | `BSTART.TEPL trowmax` | 取列间最大值归约每一行 |
| `TROWMIN` | 66 | 无(inline-asm) | `BSTART.TEPL trowmin` | 取列间最小值归约每一行 |
| `TROWPROD` | 67 | 无(inline-asm) | `BSTART.TEPL trowprod` | 跨列乘积归约每一行 |
| `TROWEXPAND` | 68 | 无(inline-asm) | `BSTART.TEPL trowexpand` | 将每行第一个元素广播到目标行 |
| `TROWARGMAX` | 76 | 无(inline-asm) | `BSTART.TEPL trowargmax` | 行 argmax（DavinciOO ext） |
| `TROWARGMIN` | 77 | 无(inline-asm) | `BSTART.TEPL trowargmin` | 行 argmin（DavinciOO ext） |

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

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TROWEXPANDADD` | 69 | 无(inline-asm) | `BSTART.TEPL trowexpandadd` | 行广播加法：dst = s0 + 每行标量 s1 |
| `TROWEXPANDSUB` | 70 | 无(inline-asm) | `BSTART.TEPL trowexpandsub` | 行广播减法 |
| `TROWEXPANDMUL` | 71 | 无(inline-asm) | `BSTART.TEPL trowexpandmul` | 行广播乘法 |
| `TROWEXPANDDIV` | 72 | 无(inline-asm) | `BSTART.TEPL trowexpanddiv` | 行广播除法 |
| `TROWEXPANDMAX` | 73 | 无(inline-asm) | `BSTART.TEPL trowexpandmax` | 行广播最大值 |
| `TROWEXPANDMIN` | 74 | 无(inline-asm) | `BSTART.TEPL trowexpandmin` | 行广播最小值 |
| `TROWEXPANDEXPDIF` | 75 | 无(inline-asm) | `BSTART.TEPL trowexpandexpdif` | 行指数差：dst = exp(s0 - s1) |

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

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TCOLSUM` | 80 | 无(inline-asm) | `BSTART.TEPL tcolsum` | 对行求和归约每一列 |
| `TCOLMAX` | 81 | 无(inline-asm) | `BSTART.TEPL tcolmax` | 取行间最大值归约每一列 |
| `TCOLMIN` | 82 | 无(inline-asm) | `BSTART.TEPL tcolmin` | 取行间最小值归约每一列 |
| `TCOLPROD` | 83 | 无(inline-asm) | `BSTART.TEPL tcolprod` | 跨行乘积归约每一列 |
| `TCOLEXPAND` | 84 | 无(inline-asm) | `BSTART.TEPL tcolexpand` | 将每列第一个元素广播到目标列 |
| `TCOLARGMAX` | 92 | 无(inline-asm) | `BSTART.TEPL tcolargmax` | 列 argmax（DavinciOO ext） |
| `TCOLARGMIN` | 93 | 无(inline-asm) | `BSTART.TEPL tcolargmin` | 列 argmin（DavinciOO ext） |

```cpp
// 示例：列最大值
tile_in src; tile_out dst;
TCOLMAX(dst, src);   // dst[j] = max(src[:, j])
```

---

## 列广播运算（2 src）

签名：`void NAME(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1)`

src0 与 dst 同形状（方阵 `R×C`）；src1 为每列标量向量（`1×C` 每列一个标量，或每列 32B
数据条），**shape 可与 src0 不一致**；src0/src1/dst 三者**dtype 必须一致**（编译期
`static_assert` 守门）。`B.DIM` 的 `ValidCol/ValidRow/Col` 均取 src0 的几何，描述 dst
的 valid region。

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TCOLEXPANDADD` | 85 | 无(inline-asm) | `BSTART.TEPL tcolexpandadd` | 列广播加法 |
| `TCOLEXPANDSUB` | 86 | 无(inline-asm) | `BSTART.TEPL tcolexpandsub` | 列广播减法 |
| `TCOLEXPANDMUL` | 87 | 无(inline-asm) | `BSTART.TEPL tcolexpandmul` | 列广播乘法 |
| `TCOLEXPANDDIV` | 88 | 无(inline-asm) | `BSTART.TEPL tcolexpanddiv` | 列广播除法 |
| `TCOLEXPANDMAX` | 89 | 无(inline-asm) | `BSTART.TEPL tcolexpandmax` | 列广播最大值 |
| `TCOLEXPANDMIN` | 90 | 无(inline-asm) | `BSTART.TEPL tcolexpandmin` | 列广播最小值 |
| `TCOLEXPANDEXPDIF` | 91 | 无(inline-asm) | `BSTART.TEPL tcolexpandexpdif` | 列指数差：dst = exp(s0 - s1) |

```cpp
// 示例：列广播乘法，src1 为每列标量向量（shape 与 src0 不同）
using tile_mat = Tile<Location::Vec, __fp32, 16, 16, BLayout::RowMajor>;
using tile_col = Tile<Location::Vec, __fp32, 1, 16, BLayout::RowMajor>;
tile_mat s0, dst;
tile_col s1(16);            // 每列一个标量
TCOLEXPANDMUL(dst, s0, s1); // dst[i,j] = s0[i,j] * s1[0,j]
```
