# 逐元素运算接口（TEPL family，BSTART.TEPL）

> 编码 family：BSTART.TEPL
> opcode = Mode * 32 + Function
> 全部一层 inline-asm，接口名 = tileop 名，程序员直接调用

---

## 二元逐元素（Mode 0，tile-tile）

签名：`void NAME(tile_shape &dst, tile_shape &src0, tile_shape &src1)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TADD(dst, s0, s1)` | 0 | dst = s0 + s1 |
| `TSUB(dst, s0, s1)` | 1 | dst = s0 - s1 |
| `TMUL(dst, s0, s1)` | 2 | dst = s0 * s1 |
| `TDIV(dst, s0, s1)` | 3 | dst = s0 / s1 |
| `TREM(dst, s0, s1)` | 4 | dst = rem(s0, s1)，余数符号与除数相同 |
| `TFMOD(dst, s0, s1)` | 5 | dst = fmod(s0, s1)，余数符号与被除数相同 |
| `TAND(dst, s0, s1)` | 6 | dst = s0 & s1 |
| `TOR(dst, s0, s1)` | 7 | dst = s0 \| s1 |
| `TXOR(dst, s0, s1)` | 8 | dst = s0 ^ s1 |
| `TSHL(dst, s0, s1)` | 9 | dst = s0 << s1 |
| `TSHR(dst, s0, s1)` | 10 | dst = s0 >> s1 |
| `TMAX(dst, s0, s1)` | 11 | dst = max(s0, s1) |
| `TMIN(dst, s0, s1)` | 12 | dst = min(s0, s1) |
| `TCMP(dst, s0, s1)` | 13 | 比较两 tile，写 packed predicate mask |
| `TPRELU(dst, s0, s1)` | 14 | parametric ReLU，s1 为逐元素斜率 |
| `TSEL(dst, s0, s1)` | 26 | 用 mask tile 在两 tile 间逐元素选择 |
| `TPARTADD(dst, s0, s1)` | 28 | partial-valid add（DavinciOO ext） |
| `TPARTMUL(dst, s0, s1)` | 29 | partial-valid multiply |
| `TPARTMAX(dst, s0, s1)` | 30 | partial-valid max |
| `TPARTMIN(dst, s0, s1)` | 31 | partial-valid min |

```cpp
// 示例：两 tile 逐元素加法
using tile_t = Tile<Location::Vec, float, 64, 32>;
tile_t dst, src0, src1;
// TCOPYIN src0, src1 ...
TADD(dst, src0, src1);   // dst = src0 + src1
```

---

## 一元逐元素（Mode 0，单 tile）

签名：`void NAME(tile_shape &dst, tile_shape &src)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TABS(dst, src)` | 15 | dst = \|src\| |
| `TNOT(dst, src)` | 16 | dst = ~src |
| `TNEG(dst, src)` | 17 | dst = -src |
| `TEXP(dst, src)` | 18 | dst = exp(src) |
| `TLOG(dst, src)` | 19 | dst = log(src) |
| `TRECIP(dst, src)` | 20 | dst = 1/src |
| `TSQRT(dst, src)` | 21 | dst = sqrt(src) |
| `TRSQRT(dst, src)` | 22 | dst = 1/sqrt(src) |
| `TRELU(dst, src)` | 23 | dst = max(src, 0) |

```cpp
// 示例：逐元素指数
tile_t dst, src;
TEXP(dst, src);   // dst = exp(src)
```

---

## 三元逐元素（Mode 0）

签名：`void NAME(tile_shape &dst, tile_shape &src0, tile_shape &src1, tile_shape &src2)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TADDC(dst, s0, s1, s2)` | 24 | dst = s0 + s1 + s2 |
| `TSUBC(dst, s0, s1, s2)` | 25 | dst = s0 - s1 + s2 |

---

## 类型转换（Mode 0）

签名：`void TCVT(tile_shape_out &dst, tile_shape_in &src)`（dst 和 src DType 可不同）

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TCVT(dst, src)` | 27 | 逐元素数据格式转换 |

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

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TADDS(dst, src, s)` | 32 | dst = src + s |
| `TSUBS(dst, src, s)` | 33 | dst = src - s |
| `TMULS(dst, src, s)` | 34 | dst = src * s |
| `TDIVS(dst, src, s)` | 35 | dst = src / s |
| `TREMS(dst, src, s)` | 36 | dst = rem(src, s) |
| `TFMODS(dst, src, s)` | 37 | dst = fmod(src, s) |
| `TANDS(dst, src, s)` | 38 | dst = src & s |
| `TORS(dst, src, s)` | 39 | dst = src \| s |
| `TXORS(dst, src, s)` | 40 | dst = src ^ s |
| `TSHLS(dst, src, s)` | 41 | dst = src << s |
| `TSHRS(dst, src, s)` | 42 | dst = src >> s |
| `TMAXS(dst, src, s)` | 43 | dst = max(src, s) |
| `TMINS(dst, src, s)` | 44 | dst = min(src, s) |
| `TCMPS(dst, src, s)` | 45 | 比较 tile 与标量 |
| `TLRELU(dst, src, s)` | 46 | LeakyReLU，s 为标量斜率 |
| `TAXPY(dst, src, s)` | 47 | APYX-style fused update（DavinciOO ext） |

```cpp
// 示例：tile 与标量乘法
tile_t dst, src; float s = 2.0f;
TMULS(dst, src, s);   // dst = src * 2.0
```

### 融合 tile-scalar-tile（dst, src0, scalar, src1）

签名：`void NAME(tile_shape &dst, tile_shape &src0, typename tile_shape::DType s, tile_shape &src1)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TADDSC(dst, s0, s, s1)` | 56 | dst = s0 + s + s1 |
| `TSUBSC(dst, s0, s, s1)` | 57 | dst = s0 - s + s1 |

### 标量选择（dst, src, mask, scalar）

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TSELS(dst, src0, src1, s)` | 58 | 用 mask 在 src tile 和标量间选择 |

### 标量广播（dst, scalar）

签名：`void TEXPANDS(tile_shape &dst, typename tile_shape::DType s)`

| 接口 | opcode | 语义 |
| --- | --- | --- |
| `TEXPANDS(dst, s)` | 59 | 将标量广播到整个 dst tile |

```cpp
// 示例：标量广播
tile_t dst; float s = 1.0f;
TEXPANDS(dst, s);   // dst 的每个元素 = 1.0
```
