# Tileop 接口使用说明

> 本目录是 Linx-TileOP-API 接口的使用参考,持续更新。
> 所有接口实现在 `include/jcore/template_asm.hpp`,一层架构(inline-asm / builtin,不走 `__vec__` kernel)。
> 程序员 `#include <tileop-api/jcore/template_asm.hpp>`(或经 `tileop_api.hpp` 间接包含)后,直接用接口名调用。

## 目录

| 类别 | 文件 | 包含的接口 |
| --- | --- | --- |
| 数据搬运(TLSU) | [tlsu.md](tlsu.md) | TLOAD / TSTORE / MGATHER / MSCATTER / MGATHER_MASK / MSCATTER_MASK |
| 矩阵乘(CUBE) | [cube.md](cube.md) | TMATMUL / TMATMUL_ACC / TMATMUL_BIAS / TMATMUL_MX / ACCCVT |
| 逐元素运算(TEPL Mode 0+1) | [elementwise.md](elementwise.md) | TADD / TSUB / TMUL / TDIV / TREM / TFMOD / TAND / TOR / TXOR / TSHL / TSHR / TMAX / TMIN / TCMP / TPRELU / TABS / TNOT / TNEG / TEXP / TLOG / TRECIP / TSQRT / TRSQRT / TRELU / TADDC / TSUBC / TSEL / TCVT / TFMA / TADDS / TSUBS / TMULS / TDIVS / TREMS / TFMODS / TANDS / TORS / TXORS / TSHLS / TSHRS / TMAXS / TMINS / TCMPS / TLRELU / TAXPY / TADDSC / TSUBSC / TSELS / TEXPANDS |
| 归约与广播(TEPL Mode 2) | [reduce-broadcast.md](reduce-broadcast.md) | TROWSUM / TROWMAX / TROWMIN / TROWPROD / TROWEXPAND / TROWARGMAX / TROWARGMIN / TROWEXPANDADD~MIN / TROWEXPANDEXPDIF / TCOLSUM / TCOLMAX / TCOLMIN / TCOLPROD / TCOLEXPAND / TCOLARGMAX / TCOLARGMIN / TCOLEXPANDADD~MIN / TCOLEXPANDEXPDIF |
| 复杂运算(TEPL Mode 3) | [complex.md](complex.md) | TCONCAT / TGATHERB / TEXTRACT / TINSERT / TIMG2COL / TFILLPAD / TCI / TTRI / TRANDOM / TQUANT / TDEQUANT / TSORT32 / TMRGSORT / TTRANS / TGATHER / TSCATTER / TPARTADD / TPARTMUL / TPARTMAX / TPARTMIN / THISTOGRAM |
| 约束与通用约定 | [constraints.md](constraints.md) | Tile size 512B..32KB / Acc tile / 寄存器 / 汇编 family 命名 |
| DavinciOO 对齐状态 | [davincioo-alignment-status.md](davincioo-alignment-status.md) | SharedTReg / TMATMUL FIXP / GMOV / PEID-SSR / encoding 的已实现项与缺失项 |

## 快速开始

```cpp
#include <common/pto_tileop.hpp>
using namespace pto;

void matmul_flow(float* a, float* b, float* c) {
  using t_A = TileLeft<float, 64, 64>;
  using t_B = TileRight<float, 64, 64>;
  using t_C = TileAcc<float, 64, 64>;
  using t_O = TileLeft<float, 64, 64>;
  using gm = global_tensor<float, RowMajor<64, 64>>;
  gm ga(a), gb(b), gc(c);
  t_A da; t_B db; t_C dc; t_O dout;
  TLOAD(da, ga); TLOAD(db, gb);
  TMATMUL(dc, da, db);   // C = A*B → ACC
  // v5: use the matching TMATMUL*.FIXP interface for an ordinary Tile.
  // Independent ACCCVT export is no longer available.
}
```

## 如何添加新的 tileop 说明

1. 找到对应类别的 `.md` 文件(如逐元素运算加到 `elementwise.md`)
2. 按 `### 接口名 — 一句话描述` 标题追加
3. 包含:签名、代码示例、生成汇编说明
4. 如是全新类别,新建 `.md` 并加到上方目录表
