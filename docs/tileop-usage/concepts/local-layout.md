# 统一 Local layout 模型（PTO-ISA #291）

本节说明 PTO-ISA #291（`spec: unify Local layouts for issues 264 and 267`）
之后 Local Tile 的布局契约，以及它如何改变 TileOP 的 C++ 接口。

## 背景

在 #291 之前，Local Tile 的状态里除 `layout` 之外还有一个正交的 `location`
（Vector / Matrix / Memory / Any）。`CUBE_M16`/`CUBE_M32` 这些矩阵表示被建模成
"Matrix location 上的私有表示"，于是凡是想在 CUBE 布局上做逐元素计算的调用都会
先被 location 断言挡掉。

#291 删除了 `TileLocation`，把表示能力全部收进 `TileLayout`：一个 Tile 是
RowMajor、CUBE_M16、CUBE_M32 还是 CUBE_N8，由 layout 单独决定；`B.DATR.Layout`
直接选择操作数的物理 Local 布局。

受影响的四组接口如下。

## 1. CUBE 布局上的逐元素操作

elementwise TEPL 操作（TADD/TMUL/TABS/TMULS/TEXPANDS 等）现在接受
`CUBE_M16`/`CUBE_M32` 的 Local 操作数，并通过 `B.DATR.Layout` 把物理布局带下去。
RowMajor 仍然走 `NORM` 默认值，**不额外发出 `B.DATR`**，所以普通 Tile 的编码不变。

编码上 `B.DATR.Layout` 使用 direct-Local 选择器：

| 布局 | `B.DATR.Layout` |
| --- | --- |
| RowMajor | `NORM`（0，省略 `B.DATR`） |
| CUBE_M32 | 29 |
| CUBE_M16 | 31 |

```cpp
using M32 = VecTileM32<float, 32, 32>;

void scale(M32 &dst, M32 &src) {
  TMULS(dst, src, 2.0f);   // BSTART.TEPL TMULS, FP32 + B.DATR CUBE_M32, Null
}
```

反汇编会把 29/31 打印成 `CUBE_M32.normal`/`CUBE_M16.normal`。

源与目标必须使用同一 layout，形状与 valid region 逐项匹配。

### 1a. 归约/广播（SFU）与 CELL 重排同样需要该选择器

`B.DATR.Layout` 不是逐元素操作专属。ISA 的 `datr_contract` 里
`allowed_nonzero_fields` 含 `Layout` 的操作都会读这个字段，TileOP 里另外两组需要注意：

**归约与广播**（TROWSUM/TROWMAX/TROWMIN/TROWPROD、TROWARGMAX/TROWARGMIN、
TROWEXPAND* 及其 7 个二元形式，以及对应的 TCOL* 族，共 28 个）。ISA 允许
RowMajor / CUBE_M16 / CUBE_M32，并且要求目的端 layout 与源一致：

```cpp
using Src = VecTileM32<float, 32, 32>;
using Dst = VecTileM32<float, 32, 1>;   // N x 1

void reduce(Dst &dst, Src &src) {
  TROWMAX(dst, src);   // B.DATR CUBE_M32, Null
}
```

裸的 `TROWMAX(RedRM&, RM&)` 走 NORM 默认；但**如果源是 CUBE 布局而 B.DATR 被省略**，
模型会按 NORM 分配目的端，随即因 `destination.layout != source.layout` 触发
`Fault_TileLegality`。所以 TileOP 现在按**源**的 layout 发选择器，并在编译期要求
源与目的端同 layout（广播形式则要求两个源都等于目的端）。CUBE_M16 的 valid rows
上限为 16、CUBE_M32 为 32，超限由 `TileReductionAndExpansionRowLimitLegal` 在模型侧拒绝。

**CELL 重排**（TPERMUTE/TSHUF/TPACK/TUNPACK）只接受 CUBE_M16/CUBE_M32，没有 RowMajor
形态，所以这 4 个操作永远发出显式的布局选择器：

```cpp
using M32 = CubeTileM32<float, 32, 32>;

void permute(M32 &dst, M32 &a, M32 &b, M32 &idx) {
  TPERMUTE(dst, a, b, idx);   // B.DATR CUBE_M32, Zero
}
```

它们的 `datr_contract` 是 must-zero，因此显式形式写 `Zero`（与 GMOV 同一约定）；
归约族是 pad-value，显式形式写 `Null`——两者都等价于各自省略 `B.DATR` 时的默认 padding。

## 2. Vec 位置的 CUBE 布局 TCVT（issue #267）

`TCVT` 的 CUBE 分支不再要求 Matrix location：`VecTileM16`/`VecTileM32` 现在可以直接
转换，CUBE 布局与 valid region 保持不变，而物理 Rows/Cols、CELL 数量与最小
TSize 按目标 dtype 独立推导。

```cpp
using Src = VecTileM32<float, 32, 32>;
using Dst = VecTileM32<__fp8_e8m0, 32, 32>;

void quantize(Dst &dst, Src &src) {
  // MX E8M0 scale 需要纯 floor 舍入（OCP MX 6.3）。
  TCVT<LINX_RDN>(dst, src);
}
```

`CubeTileM16/M32`（Matrix 表示）与 `VecTileM16/M32` 走的是同一条实现路径，差别只在
`Location` 传参，不再影响合法性判定。原来的负例断言（`SHOULD_FAIL_TCVT_*`）仍然覆盖
layout/valid-shape 不匹配和 `CUBE_N8`。

## 3. GMOV 的同布局 peer 拷贝

GMOV 允许 Local RowMajor、CUBE_M16、CUBE_M32 三种同布局 peer 拷贝，并保留 payload
字节与 definedness；`CUBE_N8` 仍只作为 GM↔Local 传输表示，在 GMOV 上被拒绝。

```cpp
using M32 = VecTileM32<float, 32, 32>;

void copy(M32 &dst, M32 &src, uint64_t peer_tid) {
  GMOV<15>(dst, peer_tid, src);   // BSTART.GMOV FP32 + B.DATR CUBE_M32, Zero
}
```

## 4. Matrix Bias 使用解析出的 M 布局

Bias 不再固定为普通 RowMajor 矩形，而是携带 resolver 选出的 M 布局 ML：

```
Bias.layout == ML == D.layout
```

ML 由 `BundleMatrixCooperativeMLayout` 决定：存在 Local A/C 时取其 layout，否则由
per-PE M 推导（M ≤ 16 → CUBE_M16，M ≤ 32 → CUBE_M32）。CUBE_M16 只覆盖至多 16 个
逻辑行，CUBE_M32 至多 32 行。

TileOP 用 `CubeBias` 表达这个契约：

```cpp
using Bias = CubeBias<float, 16>;       // 声明 CUBE_M16 的 1xN Bias
using Bias32 = CubeBias<float, 32, 32>; // 32 行 CELL，声明 CUBE_M32

using D = CubeAccumulatorM16<float, 16, 16>;
using A = CubeTileM16<float, 16, 16>;
using B = CubeTileN8<float, 16, 16>;

void bias_add(D &d, A &a, B &b, Bias &bias) {
  TMATMUL_BIAS(d, a, b, bias);
}
```

`CubeBias<Element, Cols, Rows = 16, ColValid = Cols>` 的第一个模板参数是元素类型，
第二个是逻辑 N（`ValidCol`），第三个是物理 CELL 行数并据此选择 M16/M32。
Bias 的 `ValidRow` 恒为 1。

## 尚未覆盖

以下操作在 ISA 里同样读 `B.DATR.Layout`，但 TileOP 目前还没有按操作数布局发出该字段
（RowMajor 默认值之外的形态会静默退化）：

- `MGATHER` / `MGATHER_MASK` / `MGATHER_CAS` / `MSCATTER` / `MSCATTER_MASK`：
  ISA 对这几个操作要求目的端/源与索引 Tile 使用**非 CUBE** 布局
  （`TileDescriptorLegal` → `TileGenericIndexingPermitted` 要求 `!TileLayoutIsCube`），
  所以有意义的选择器只有 RowMajor/ColumnMajor 两种。当前包装器发出的是固定的
  `B.DATR Null`（等价 `Layout=NORM`），既没有覆盖 ColumnMajor，也顺带忽略了
  `MGATHER`/`MGATHER_MASK` 的 `Pad` 模板参数——`Pad=Zero/Max` 与默认的 `Null`
  编码相同，参数实际上不生效。
- 18 个 GM atomic/reduction 操作（`MGATHER_EXCH/MAX/MIN/ADD/INC/DEC/AND/OR/XOR`、
  `MSCATTER_MAX/MIN/ADD/INC/DEC/AND/OR/XOR/POPC`）在 TileOP 里还没有包装器，
  补齐时需要一并携带布局选择器。

## 迁移提示

- 原来写 `Tile<Location::Bias, T, Rows, Cols, BLayout::RowMajor, 1, N>` 的 Bias，
  改为 `CubeBias<T, N>`（或 `CubeBias<T, N, 32>`）。
- 原来因为 location 断言而绕开 CUBE 布局、改走 GM 往返或 Matrix 转换的 kernel，
  现在可以直接对 `VecTileM16/M32` 调用 TCVT、逐元素算子与归约/广播算子；
  CELL 重排（TPERMUTE/TSHUF/TPACK/TUNPACK）本来就只接受 CUBE 布局。
- 具体的 `B.DATR` 布局编码以上表为准；`CUBE_N8` 仍然不可用于逐元素操作、归约与 GMOV。
