# 矩阵乘接口(CUBE family,BSTART.CUBE)

> 编码 family:BSTART.CUBE
> 非 FIXP 的 TMATMUL 系列产生 implicit ACC；DavinciOO v5 不再提供独立 ACCCVT 导出。
> 需要普通 Tile 输出时必须选择对应的 `TMATMUL*.FIXP` variant。
> `TMATMUL_FIXP` 的完整 options API、量化 descriptor、RowMax/GroupMax 和 Shared Right 用法见 [tmatmul-fixp.md](tmatmul-fixp.md)。

---

## TMATMUL — C = A × B

```cpp
void matmul_example(float* a, float* b, float* c) {
  using t_A = TileLeft<float, 64, 64>;
  using t_B = TileRight<float, 64, 64>;
  using t_C = TileAcc<float, 64, 64>;       // C 必须是 Acc
  t_A da; t_B db; t_C dc;
  // 先 TLOAD da, db ...
  TMATMUL(dc, da, db);      // C = A*B → implicit ACC
  // 如需普通 Tile/GM 输出，改用 TMATMUL.FIXP 对应接口。
}
```

- **签名**:`TMATMUL(c, a, b)`,c 是 `TileAcc`
- **builtin**：`blk_matmul`
- **约束**:c 必须是 `Location::Acc`;a/b 不能是 Acc
- **生成**:`BSTART.CUBE TMATMUL, <dtypeA>` + `B.DIM(M/N/K)` + `B.IOT [a, b], last, ->acc<size>`

### Shared A/B 矩阵

所有公开 `TMATMUL*` 接口使用相同的参数顺序，矩阵 `A`、`B` 可以通过
`TMOV_L2S_PUBLISH` 或 `TMOV_L2S_INSERT` 转换成 `SharedTile`。当前支持三种存储组合：

```cpp
using A = TileLeft<float, 16, 16>;
using B = TileRight<float, 16, 16>;
using C = Tile<Location::Vec, float, 16, 16, BLayout::RowMajor>;

void example(C &dst, A &a, B &b) {
  TMATMUL(dst, a, b);  // Local A, Local B

  auto shared_b = TMOV_L2S_PUBLISH(b);
  TMATMUL(dst, a, shared_b);  // Local A, Shared B

  auto shared_a = TMOV_L2S_PUBLISH(a);
  TMATMUL(dst, shared_a, shared_b);  // Shared A, Shared B
}
```

双 Shared 形式会为两个矩阵分别分配 Shared 寄存器，并按矩阵 source 顺序生成：

```asm
BSTART.CUBE TMATMUL, FP32
...
C.B.IOS S#0
C.B.IOS S#1
B.IOT mask=1111, last, ->dst<size>
```

矩阵 `A`、`B` 不再出现在 `B.IOT` source stream 中。`TMATMULMX*` 的
`scale_a`、`scale_b` 以及 BIAS/ACC/FIXP 的附加 Tile 仍是普通 Local Tile，继续编码为
`B.IOT`。

目前不支持只有 `A` 是 Shared、`B` 是 Local 的组合。单条 `C.B.IOS` 已用于既有的
Shared-Right ABI，无法区分 Shared-A/Local-B；该写法会在编译期给出明确错误。若 `A`
使用 Shared，请同时将 `B` 转换为 Shared。

以下接口均支持 Local/Local、Local/Shared-B 和 Shared-A/Shared-B：

- `TMATMUL`、`TMATMUL_ACC`、`TMATMUL_BIAS`
- `TMATMUL_FIXP`、`TMATMUL_ACC_FIXP`、`TMATMUL_BIAS_FIXP`
- `TMATMUL_MX`、`TMATMUL_MX_ACC`、`TMATMUL_MX_BIAS`
- `TMATMUL_MX_FIXP`、`TMATMUL_MX_ACC_FIXP`、`TMATMUL_MX_BIAS_FIXP`

---

## TMATMUL_ACC — C += A × B(累加)

```cpp
TMATMUL_ACC(dc, da, db);    // dc 既作 ACC 输入又作输出
```

- **签名**:`TMATMUL_ACC(c, a, b)`,c 既是输入 ACC 也是输出
- **builtin**：`blk_matmul_ac`
- **生成**:`BSTART.CUBE TMATMUL.ACC`

---

## TMATMUL_BIAS — C = A × B + bias

```cpp
using t_bias = TileLeft<float, 64, 64>;
t_bias dbi;
// TLOAD dbi ...
TMATMUL_BIAS(dc, da, db, dbi);   // bias 是第 4 个 tile(ExtraTile)
```

- **签名**:`TMATMUL_BIAS(c, a, b, bias)`
- **builtin**：`blk_matmul_ac`
- **bias**:普通 tile(不能是 Acc),必须先 TLOAD 初始化
- **生成**:`BSTART.CUBE TMATMUL.BIAS`

---

## TMATMUL_MX — C = (A × aScale) × (B × bScale)

```cpp
using t_S = TileLeft<float, 64, 64>;
t_S das, dbs;
// TLOAD das, dbs ...
TMATMUL_MX(dc, da, das, db, dbs);  // 带 scale 的矩阵乘
```

- **签名**:`TMATMUL_MX(c, a, aScale, b, bScale)`
- **builtin**：`blk_matmulmx`
- **aScale/bScale**:普通 tile,必须先 TLOAD 初始化
- **生成**:`BSTART.CUBE TMATMULMX`

---

## ACCCVT — ACC → 普通 Tile(CUBE helper)

- **签名**:`ACCCVT(tile_shape_out &dst, tile_shape_in &src)`,src 是 Acc tile
- **状态**：DavinciOO v5 已移除独立 `ACCCVT` opcode；当前 API 对该调用给出编译期迁移错误。
- **可随路做类型转换**:`dst` 和 `src` DType 不同时,自动做 convert
- **迁移**：使用对应的 `TMATMUL*.FIXP` variant 直接产生普通 Tile；不能再从 live implicit ACC 单独导出。

---

## FIXP 迁移说明

当前头文件提供以下基础 Local form：

```cpp
TMATMUL_FIXP(d, a, b);           // d = FIXP(a*b)
TMATMUL_ACC_FIXP(d, acc, a, b);  // d = FIXP(acc+a*b)
```

- `d` 是普通 Local Tile，逻辑大小为 512 B..32 KB；`a`/`b` 分别是
  `Location::Left`/`Location::Right`。
- `TMATMUL_ACC_FIXP` 的 `acc` 必须是 `Location::Acc`，由编译器作为专用 ACC
  dependency 保留，不会编码成普通 `B.IOT` operand。
- 基础 form 固定生成 `B.FPATR 0, 0, 0, 0, 0, 0, 0`，即不启用 quant、ReLU、
  RowMax 或 GroupMax。带参数和附加输出的高级 FIXP mode 尚未开放。
- 不能用旧 `ACCCVT` wrapper 静默替代 FIXP 写回。
