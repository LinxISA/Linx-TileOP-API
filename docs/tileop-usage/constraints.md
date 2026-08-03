# 约束与通用约定

## Tile size: 512 B..32 KB

DavinciOO v5 `B.IOT.TSize` is a 3-bit logical-Tile size code. `0` is implicit
(no ordinary destination allocation); explicit Tile operands use `1..7`:

| TSize | Logical Tile size |
| ---: | ---: |
| 0 | implicit |
| 1 | 512 B |
| 2 | 1 KB |
| 3 | 2 KB |
| 4 | 4 KB |
| 5 | 8 KB |
| 6 | 16 KB |
| 7 | 32 KB |

Each PE owns a fixed quarter fragment of the logical Tile. The API maps
`sizeof(TileDType)` through `tile_type_traits::TilesizeCode` and rejects sizes
outside 512 B..32 KB with `IsValidActiveSize`.

## TMATMUL 系列：普通 Tile 输出与累加输入

- v5 不再使用 Acc register class；TMATMUL 系列输出均为普通 Tile。
- `TMATMUL_ACC(dst, acc, a, b)` 的 `dst` 与 `acc` 都是普通 Tile，`acc` 作为普通 B.IOT source。
- `ACCCVT` 已删除；FIXP、Bias、RowMax 等结果由对应 TMATMUL 接口直接写入普通 Tile。

## MGATHER/MSCATTER:offset 必须 tile

- offset(索引)必须是 tile 形式,先 `TLOAD` 从 GM 搬进 tile
- 不能直接传普通指针/数组(会变 global load,见 Block-C 痛点文档问题 7)

## 两 src 运算的 shape/dtype 契约

适用于 TEPL 行/列广播(`TROWEXPAND*`/`TCOLEXPAND*`,共 14 条)与列拼接(`TCONCAT`)。

- **shape 可不同**:src0 与 src1 用独立 template 参数 `tile_shape_in0`/`tile_shape_in1`,不强制同 shape。
  - 广播:`src0`/`dst` 同形(`R×C`),`src1` 是每行/每列标量向量(`R×1` 或 `1×C`)或 32B 数据条。
  - 拼接(`TCONCAT`):三者 shape **全部不同**——`src0`=`R×C0`,`src1`=`R×C1`,`dst`=`R×(C0+C1)`,`src0`/`src1` 行数必须相同且等于 `dst` 行数。
- **dtype 须一致**:`src0`/`src1`/`dst` 三者 dtype 必须相同,编译期 `static_assert` 守门(`src0/src1` 不匹配、`src0/dst` 不匹配各一条)。`%c1` 主 DataType 取 `src0`。
- **B.DIM 取值**:
  - 广播:`ValidCol/ValidRow/Col` 取 `src0`(dst 跟 src0 同形)。
  - `TCONCAT`:取 **dst**(`tile_shape_out`),因为 dst 的 `R×(C0+C1)` 才是完整几何,取 `src0` 会丢 `src1` 那半段。
- 函数实现在 `include/jcore/template_asm.hpp`,签名固定为 `NAME(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1)`。

## 汇编 family 命名

- 数据搬运:TMA 已改名为 **TLSU**(编码不变,`BSTART.TLSU`)
- 矩阵乘:**CUBE**(`BSTART.CUBE`)
- 逐元素运算:**TEPL**(`BSTART.TEPL`)


## Dynamic Valid Shape

Linx one-level TileOP supports runtime `ValidRow`/`ValidCol` across the public
inline-assembly interfaces. The physical Tile allocation remains compile-time
fixed; only the active region is dynamic.

```cpp
using tile_t = Tile<Location::Vec, float,
                    16, 512, BLayout::RowMajor,
                    -1, -1>;

tile_t tile(runtime_rows, runtime_cols);
```

The first `Rows`/`Cols` pair determines register capacity, layout, and B.IOT
TileSize encoding and therefore cannot be `-1`. The final `RowValid`/`ColValid`
pair may be `-1`; constructors then store the runtime values and every Linx
one-level TileOP emits register-form dimensions such as:

```asm
B.DIM x3, 0, ->lb0
B.DIM x4, 0, ->lb1
```

Operations with several same-shaped operands require callers to construct them
with matching runtime valid dimensions. Matrix operations derive `M`, `N`, and
`K` from their Left/Right operands. `SharedTile` preserves these runtime values
when produced by `TMOV_L2S_INSERT` or `TMOV_L2S_PUBLISH`, so Shared Matmul uses
the same dynamic `B.DIM` path as Local Matmul.
