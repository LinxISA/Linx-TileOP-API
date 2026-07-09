# 布局转换接口

> Tile 之间的 layout 转换、类型转换、转置等

(待补充)

已有接口(在 `template_asm.hpp`):
- `TCVT_T(dst, src)` — 类型转换(inline-asm)
- `TMOV_ND2NZ` / `TMOV_NZ2ND` / `TMOV_NORM` 等 — layout 转换(宏模板)

待补接口(按需添加):
- TTRANS / TRESHAPE / TFILLPAD / ...
