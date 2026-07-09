# 逐元素运算接口(TEPL family,BSTART.TEPL)

> 编码 family:BSTART.TEPL
> 逐元素 tile-tile / tile-scalar 运算

(待补充)

已有接口(在 `template_asm.hpp`):
- `TXOR(dst, src0, src1)` — 逐元素异或
- `TSLL(dst, src, shamt)` — 标量移位(整 tile 统一左移)
- `TSRL(dst, src, shamt)` — 标量逻辑右移(整 tile 统一)

待补接口(按需添加):
- TADD / TSUB / TMUL / TDIV / TMIN / TMAX / TABS / TEXP / TSQRT / ...
- TADDS / TMINS / TMAXS(tile-scalar 变体)
- TCMP / TCMPS / TSEL / TSELS
- TAND / TOR / TNOT / TSHL / TSHR
