# 矩阵乘接口(CUBE family,BSTART.CUBE)

> 编码 family:BSTART.CUBE
> TMATMUL 系列的输出 C 必须是 `Location::Acc` tile(用 `TileAcc<T, M, N>` 别名)。
> 计算结果先落在 ACC 累加器,需用 `ACCCVT` 导出到普通 tile。

---

## TMATMUL — C = A × B

```cpp
void matmul_example(float* a, float* b, float* c) {
  using t_A = TileLeft<float, 64, 64>;
  using t_B = TileRight<float, 64, 64>;
  using t_C = TileAcc<float, 64, 64>;       // C 必须是 Acc
  using t_O = TileLeft<float, 64, 64>;      // 输出普通 tile
  t_A da; t_B db; t_C dc; t_O dout;
  // 先 TCOPYIN da, db ...
  TMATMUL(dc, da, db);      // C = A*B → ACC
  ACCCVT(dout, dc);         // ACC → 普通 tile
  // TCOPYOUT(c, dout) ...
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
ACCCVT(dout, dc);
```

- **签名**:`TMATMUL_ACC(c, a, b)`,c 既是输入 ACC 也是输出
- **builtin**：`blk_matmul_ac`
- **生成**:`BSTART.CUBE TMATMUL.ACC`

---

## TMATMUL_BIAS — C = A × B + bias

```cpp
using t_bias = TileLeft<float, 64, 64>;
t_bias dbi;
// TCOPYIN dbi ...
TMATMUL_BIAS(dc, da, db, dbi);   // bias 是第 4 个 tile(ExtraTile)
ACCCVT(dout, dc);
```

- **签名**:`TMATMUL_BIAS(c, a, b, bias)`
- **builtin**：`blk_matmul_ac`
- **bias**:普通 tile(不能是 Acc),必须先 TCOPYIN 初始化
- **生成**:`BSTART.CUBE TMATMUL.BIAS`

---

## TMATMUL_MX — C = (A × aScale) × (B × bScale)

```cpp
using t_S = TileLeft<float, 64, 64>;
t_S das, dbs;
// TCOPYIN das, dbs ...
TMATMUL_MX(dc, da, das, db, dbs);  // 带 scale 的矩阵乘
ACCCVT(dout, dc);
```

- **签名**:`TMATMUL_MX(c, a, aScale, b, bScale)`
- **builtin**：`blk_matmulmx`
- **aScale/bScale**:普通 tile,必须先 TCOPYIN 初始化
- **生成**:`BSTART.CUBE TMATMULMX`

---

## ACCCVT — ACC → 普通 Tile(CUBE helper)

```cpp
ACCCVT(dout, dc);   // 把 ACC 累加器导出为普通 tile
```

- **签名**:`ACCCVT(tile_shape_out &dst, tile_shape_in &src)`,src 是 Acc tile
- **builtin**：`blk_acccvt`
- **可随路做类型转换**:`dst` 和 `src` DType 不同时,自动做 convert
- **生成**:`BSTART.CUBE ACCCVT, <srcType>` + `B.DATR NORM.normal, <dstType>, Null` + `B.IOT [], last, ->dst<size>` + `C.B.DIMI(ValidCol/ValidRow)`

---

## 完整示例:matmul + bias 流程

```cpp
#include <common/pto_tileop.hpp>
using namespace pto;

void matmul_bias_flow(float* a, float* b, float* bias, float* c) {
  using t_A = TileLeft<float, 64, 64>;
  using t_B = TileRight<float, 64, 64>;
  using t_C = TileAcc<float, 64, 64>;
  using t_O = TileLeft<float, 64, 64>;
  using t_BI = TileLeft<float, 64, 64>;
  using gm = global_tensor<float, RowMajor<64, 64>>;

  gm ga(a), gb(b), gbi(bias), gc(c);
  t_A da; t_B db; t_C dc; t_O dout; t_BI dbi;

  TCOPYIN(da, ga); TCOPYIN(db, gb); TCOPYIN(dbi, gbi);
  TMATMUL_BIAS(dc, da, db, dbi);   // C = A*B + bias → ACC
  ACCCVT(dout, dc);                // ACC → tile
  TSTORE(gc, dout);                // tile → GM
}
```
