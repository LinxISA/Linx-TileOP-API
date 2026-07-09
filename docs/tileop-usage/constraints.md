# 约束与通用约定

## Tile size:128 B..8 KB

DavinciOO active PE-local profile(见 `isa/intrinsic/header/B.IOT.md`)只允许 `imm4=3..9`,对应 Tile allocation size 128 B..8 KB。

| imm4 | size | 合法? |
| --- | --- | --- |
| 0 | 0 B | reserved/illegal |
| 1 | 32 B | reserved/illegal |
| 2 | 64 B | reserved/illegal |
| 3 | 128 B | ✓ |
| 4 | 256 B | ✓ |
| 5 | 512 B | ✓ |
| 6 | 1 KB | ✓ |
| 7 | 2 KB | ✓ |
| 8 | 4 KB | ✓ |
| 9 | 8 KB | ✓ |
| 10 | 16 KB | reserved/illegal |
| 11 | 32 KB | reserved/illegal |
| 12 | 64 KB | reserved/illegal |
| 13 | 128 KB | reserved/illegal |
| 14 | 256 KB | reserved/illegal |
| 15 | 512 KB | reserved/illegal |

超出范围的 tile 在**编译期**被 `static_assert` 拒绝,报错信息:
```
Tile allocation size must be 128 B..8 KB (imm4=3..9) per DavinciOO
active PE-local profile (header/B.IOT.md)
```

实现:`tile_type_traits::IsValidActiveSize`(`include/jcore/type.hpp`),各 tileop 模板在 `template_asm.hpp` 里 `static_assert` 检查 dst/src tile。

## TMATMUL 系列:Acc tile 约束

- 输出 C 必须是 `Location::Acc` tile(用 `TileAcc<T, M, N>` 别名)
- 输入 A/B/bias/scale 不能是 Acc
- C 的数据先落在 ACC 累加器,需 `ACCCVT` 导出到普通 tile

## MGATHER/MSCATTER:offset 必须 tile

- offset(索引)必须是 tile 形式,先 `TCOPYIN` 从 GM 搬进 tile
- 不能直接传普通指针/数组(会变 global load,见 Block-C 痛点文档问题 7)

## 汇编 family 命名

- 数据搬运:TMA 已改名为 **TLSU**(编码不变,`BSTART.TLSU`)
- 矩阵乘:**CUBE**(`BSTART.CUBE`)
- 逐元素运算:**TEPL**(`BSTART.TEPL`)
