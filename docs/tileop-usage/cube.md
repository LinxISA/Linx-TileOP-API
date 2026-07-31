# 矩阵乘接口(CUBE family,BSTART.CUBE)

> 编码 family:BSTART.CUBE
> 非 FIXP 的 TMATMUL 系列产生 implicit ACC；DavinciOO v5 不再提供独立 ACCCVT 导出。
> 需要普通 Tile 输出时必须选择对应的 `TMATMUL*.FIXP` variant。

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
