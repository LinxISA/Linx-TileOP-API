# 逐元素运算接口（TEPL family，BSTART.TEPL）

> 编码 family：BSTART.TEPL
> opcode = Mode * 32 + Function
> 全部一层 inline-asm，接口名 = tileop 名，程序员直接调用

---

## 二元逐元素（Mode 0，tile-tile）

签名：`void NAME(tile_shape &dst, tile_shape &src0, tile_shape &src1)`

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TADD` | 0 | 无(inline-asm) | `BSTART.TEPL tadd` | dst = s0 + s1 |
| `TSUB` | 1 | 无(inline-asm) | `BSTART.TEPL tsub` | dst = s0 - s1 |
| `TMUL` | 2 | 无(inline-asm) | `BSTART.TEPL tmul` | dst = s0 * s1 |
| `TDIV` | 3 | 无(inline-asm) | `BSTART.TEPL tdiv` | dst = s0 / s1 |
| `TREM` | 4 | 无(inline-asm) | `BSTART.TEPL trem` | dst = rem(s0, s1)，余数符号与除数相同 |
| `TFMOD` | 5 | 无(inline-asm) | `BSTART.TEPL tfmod` | dst = fmod(s0, s1)，余数符号与被除数相同 |
| `TAND` | 6 | 无(inline-asm) | `BSTART.TEPL tand` | dst = s0 & s1 |
| `TOR` | 7 | 无(inline-asm) | `BSTART.TEPL tor` | dst = s0 \| s1 |
| `TXOR` | 8 | 无(inline-asm) | `BSTART.TEPL txor` | dst = s0 ^ s1 |
| `TSHL` | 9 | 无(inline-asm) | `BSTART.TEPL tshl` | dst = s0 << s1 |
| `TSHR` | 10 | 无(inline-asm) | `BSTART.TEPL tshr` | dst = s0 >> s1 |
| `TMAX` | 11 | 无(inline-asm) | `BSTART.TEPL tmax` | dst = max(s0, s1) |
| `TMIN` | 12 | 无(inline-asm) | `BSTART.TEPL tmin` | dst = min(s0, s1) |
| `TCMP` | 13 | 无(inline-asm) | `BSTART.TEPL tcmp` | 比较两 tile，写 packed predicate mask |
| `TPRELU` | 14 | 无(inline-asm) | `BSTART.TEPL tprelu` | parametric ReLU，s1 为逐元素斜率 |
| `TSEL` | 26 | 无(inline-asm) | `BSTART.TEPL tsel` | 用 mask tile 在两 tile 间逐元素选择 |
| `TFMA` | 28 | 无(inline-asm) | `BSTART.TEPL tfma` | dst = s0 * s1 + s2（fused multiply-add, DavinciOO ext） |

> 注意：TPARTADD/TPARTMUL/TPARTMAX/TPARTMIN 已从 Mode 0 移到 Mode 3（见 [complex.md](complex.md)）。opcode 29-31 在 Mode 0 为 reserved。

```cpp
// 示例：两 tile 逐元素加法
using tile_t = Tile<Location::Vec, float, 64, 32>;
tile_t dst, src0, src1;
// TLOAD src0, src1 ...
TADD(dst, src0, src1);   // dst = src0 + src1
```

---

## 一元逐元素（Mode 0，单 tile）

签名：`void NAME(tile_shape &dst, tile_shape &src)`

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TABS` | 15 | 无(inline-asm) | `BSTART.TEPL tabs` | dst = \|src\| |
| `TNOT` | 16 | 无(inline-asm) | `BSTART.TEPL tnot` | dst = ~src |
| `TNEG` | 17 | 无(inline-asm) | `BSTART.TEPL tneg` | dst = -src |
| `TEXP` | 18 | 无(inline-asm) | `BSTART.TEPL texp` | dst = exp(src) |
| `TLOG` | 19 | 无(inline-asm) | `BSTART.TEPL tlog` | dst = log(src) |
| `TRECIP` | 20 | 无(inline-asm) | `BSTART.TEPL trecip` | dst = 1/src |
| `TSQRT` | 21 | 无(inline-asm) | `BSTART.TEPL tsqrt` | dst = sqrt(src) |
| `TRSQRT` | 22 | 无(inline-asm) | `BSTART.TEPL trsqrt` | dst = 1/sqrt(src) |
| `TRELU` | 23 | 无(inline-asm) | `BSTART.TEPL trelu` | dst = max(src, 0) |

```cpp
// 示例：逐元素指数
tile_t dst, src;
TEXP(dst, src);   // dst = exp(src)
```

---

## 三元逐元素（Mode 0）

签名：`void NAME(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2)`

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TADDC` | 24 | 无(inline-asm) | `BSTART.TEPL taddc` | dst = s0 + s1 + s2 |
| `TSUBC` | 25 | 无(inline-asm) | `BSTART.TEPL tsubc` | dst = s0 - s1 + s2 |

---

## 类型转换（Mode 0）

签名：`void TCVT(tile_shape_out &dst, tile_shape_in &src)`（dst 和 src DType 可不同）

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TCVT` | 27 | 无(inline-asm) | `BSTART.TEPL tcvt` | 逐元素数据格式转换 |

```cpp
// 示例：bf16 -> fp32 转换
using tile_bf = Tile<Location::Vec, __bf16, 64, 32>;
using tile_f32 = Tile<Location::Vec, float, 64, 32>;
tile_bf src; tile_f32 dst;
TCVT(dst, src);   // bf16 -> fp32
```

---

## Tile-标量逐元素（Mode 1）

签名：`void NAME(tile_shape &dst, tile_shape &src, typename tile_shape::DType s)`

scalar 通过 `B.IOR` 传入。

### 标准 tile-scalar（dst, src, scalar）

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TADDS` | 32 | 无(inline-asm) | `BSTART.TEPL tadds` | dst = src + s |
| `TSUBS` | 33 | 无(inline-asm) | `BSTART.TEPL tsubs` | dst = src - s |
| `TMULS` | 34 | 无(inline-asm) | `BSTART.TEPL tmuls` | dst = src * s |
| `TDIVS` | 35 | 无(inline-asm) | `BSTART.TEPL tdivs` | dst = src / s |
| `TREMS` | 36 | 无(inline-asm) | `BSTART.TEPL trems` | dst = rem(src, s) |
| `TFMODS` | 37 | 无(inline-asm) | `BSTART.TEPL tfmods` | dst = fmod(src, s) |
| `TANDS` | 38 | 无(inline-asm) | `BSTART.TEPL tands` | dst = src & s |
| `TORS` | 39 | 无(inline-asm) | `BSTART.TEPL tors` | dst = src \| s |
| `TXORS` | 40 | 无(inline-asm) | `BSTART.TEPL txors` | dst = src ^ s |
| `TSHLS` | 41 | 无(inline-asm) | `BSTART.TEPL tshls` | dst = src << s |
| `TSHRS` | 42 | 无(inline-asm) | `BSTART.TEPL tshrs` | dst = src >> s |
| `TMAXS` | 43 | 无(inline-asm) | `BSTART.TEPL tmaxs` | dst = max(src, s) |
| `TMINS` | 44 | 无(inline-asm) | `BSTART.TEPL tmins` | dst = min(src, s) |
| `TCMPS` | 45 | 无(inline-asm) | `BSTART.TEPL tcmps` | 比较 tile 与标量 |
| `TLRELU` | 46 | 无(inline-asm) | `BSTART.TEPL tlrelu` | LeakyReLU，s 为标量斜率 |
| `TAXPY` | 47 | 无(inline-asm) | `BSTART.TEPL taxpy` | APYX-style fused update（DavinciOO ext） |

```cpp
// 示例：tile 与标量乘法
tile_t dst, src; float s = 2.0f;
TMULS(dst, src, s);   // dst = src * 2.0
```

### 融合 tile-scalar-tile（dst, src0, scalar, src1）

签名：`void NAME(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1)`

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TADDSC` | 56 | 无(inline-asm) | `BSTART.TEPL taddsc` | dst = s0 + s + s1 |
| `TSUBSC` | 57 | 无(inline-asm) | `BSTART.TEPL tsubsc` | dst = s0 - s + s1 |

### 标量选择（dst, src, mask, scalar）

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TSELS` | 58 | 无(inline-asm) | `BSTART.TEPL tsels` | 用 mask 在 src tile 和标量间选择 |

### 标量广播（dst, scalar）

签名：`void TEXPANDS(tile_shape &dst, typename tile_shape::DType s)`

| 接口 | opcode | builtin | 汇编助记符 | 语义 |
| --- | --- | --- | --- | --- |
| `TEXPANDS` | 59 | 无(inline-asm) | `BSTART.TEPL texpands` | 将标量广播到整个 dst tile |

```cpp
// 示例：标量广播
tile_t dst; float s = 1.0f;
TEXPANDS(dst, s);   // dst 的每个元素 = 1.0
```
