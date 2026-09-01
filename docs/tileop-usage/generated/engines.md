# LinxISA / PTO ISA v0.58.3 执行引擎

架构定义的引擎类别只有 **VEC**, **TLSU**, **CUBE**, and **SFU**。
VEC 只包含逐元素操作；SFU 包含归约、广播、变换、排序以及其他需要更复杂硬件的操作。
TEPL 仍是唯一的编译 carrier 标识。`BSTART.VEC` 和 `BSTART.SFU` 是特定引擎的汇编别名；
inline wrapper 保留 `BSTART.TEPL`，以兼容之前的工具链源码。

下表根据固定版本的 LinxISA 权威数据生成，数据记录在
[`contracts/linxisa-v0.58-engine-ops.json`](../../contracts/linxisa-v0.58-engine-ops.json) 中。

## VEC

| API / 操作 | 规范汇编 | 逻辑 selector | 分类 |
| --- | --- | ---: | --- |
| `TADD` | `BSTART.VEC TADD` | 0 | elementwise-tile-tile |
| `TSUB` | `BSTART.VEC TSUB` | 1 | elementwise-tile-tile |
| `TMUL` | `BSTART.VEC TMUL` | 2 | elementwise-tile-tile |
| `TAND` | `BSTART.VEC TAND` | 6 | elementwise-tile-tile |
| `TOR` | `BSTART.VEC TOR` | 7 | elementwise-tile-tile |
| `TXOR` | `BSTART.VEC TXOR` | 8 | elementwise-tile-tile |
| `TSHL` | `BSTART.VEC TSHL` | 9 | elementwise-tile-tile |
| `TSHR` | `BSTART.VEC TSHR` | 10 | elementwise-tile-tile |
| `TMAX` | `BSTART.VEC TMAX` | 11 | elementwise-tile-tile |
| `TMIN` | `BSTART.VEC TMIN` | 12 | elementwise-tile-tile |
| `TCMP` | `BSTART.VEC TCMP` | 13 | elementwise-tile-tile |
| `TABS` | `BSTART.VEC TABS` | 15 | elementwise-tile-tile |
| `TNOT` | `BSTART.VEC TNOT` | 16 | elementwise-tile-tile |
| `TNEG` | `BSTART.VEC TNEG` | 17 | elementwise-tile-tile |
| `TRELU` | `BSTART.VEC TRELU` | 23 | elementwise-tile-tile |
| `TSEL` | `BSTART.VEC TSEL` | 26 | elementwise-tile-tile |
| `TCVT` | `BSTART.VEC TCVT` | 27 | elementwise-tile-tile |
| `TFMA` | `BSTART.VEC TFMA` | 28 | elementwise-tile-tile |
| `TADDS` | `BSTART.VEC TADDS` | 32 | tile-scalar-and-immediate |
| `TSUBS` | `BSTART.VEC TSUBS` | 33 | tile-scalar-and-immediate |
| `TMULS` | `BSTART.VEC TMULS` | 34 | tile-scalar-and-immediate |
| `TANDS` | `BSTART.VEC TANDS` | 38 | tile-scalar-and-immediate |
| `TORS` | `BSTART.VEC TORS` | 39 | tile-scalar-and-immediate |
| `TXORS` | `BSTART.VEC TXORS` | 40 | tile-scalar-and-immediate |
| `TSHLS` | `BSTART.VEC TSHLS` | 41 | tile-scalar-and-immediate |
| `TSHRS` | `BSTART.VEC TSHRS` | 42 | tile-scalar-and-immediate |
| `TMAXS` | `BSTART.VEC TMAXS` | 43 | tile-scalar-and-immediate |
| `TMINS` | `BSTART.VEC TMINS` | 44 | tile-scalar-and-immediate |
| `TCMPS` | `BSTART.VEC TCMPS` | 45 | tile-scalar-and-immediate |
| `TSELS` | `BSTART.VEC TSELS` | 58 | tile-scalar-and-immediate |
| `TEXPANDS` | `BSTART.VEC TEXPANDS` | 59 | tile-scalar-and-immediate |

## SFU

| API / 操作 | 规范汇编 | 逻辑 selector | 分类 |
| --- | --- | ---: | --- |
| `TDIV` | `BSTART.SFU TDIV` | 3 | elementwise-tile-tile |
| `TREM` | `BSTART.SFU TREM` | 4 | elementwise-tile-tile |
| `TEXP` | `BSTART.SFU TEXP` | 18 | elementwise-tile-tile |
| `TLOG` | `BSTART.SFU TLOG` | 19 | elementwise-tile-tile |
| `TRECIP` | `BSTART.SFU TRECIP` | 20 | elementwise-tile-tile |
| `TSQRT` | `BSTART.SFU TSQRT` | 21 | elementwise-tile-tile |
| `TRSQRT` | `BSTART.SFU TRSQRT` | 22 | elementwise-tile-tile |
| `TDIVS` | `BSTART.SFU TDIVS` | 35 | tile-scalar-and-immediate |
| `TREMS` | `BSTART.SFU TREMS` | 36 | tile-scalar-and-immediate |
| `TROWSUM` | `BSTART.SFU TROWSUM` | 64 | reduce-and-expand |
| `TROWMAX` | `BSTART.SFU TROWMAX` | 65 | reduce-and-expand |
| `TROWMIN` | `BSTART.SFU TROWMIN` | 66 | reduce-and-expand |
| `TROWPROD` | `BSTART.SFU TROWPROD` | 67 | reduce-and-expand |
| `TROWEXPAND` | `BSTART.SFU TROWEXPAND` | 68 | reduce-and-expand |
| `TROWEXPANDADD` | `BSTART.SFU TROWEXPANDADD` | 69 | reduce-and-expand |
| `TROWEXPANDSUB` | `BSTART.SFU TROWEXPANDSUB` | 70 | reduce-and-expand |
| `TROWEXPANDMUL` | `BSTART.SFU TROWEXPANDMUL` | 71 | reduce-and-expand |
| `TROWEXPANDDIV` | `BSTART.SFU TROWEXPANDDIV` | 72 | reduce-and-expand |
| `TROWEXPANDMAX` | `BSTART.SFU TROWEXPANDMAX` | 73 | reduce-and-expand |
| `TROWEXPANDMIN` | `BSTART.SFU TROWEXPANDMIN` | 74 | reduce-and-expand |
| `TROWEXPANDEXPDIF` | `BSTART.SFU TROWEXPANDEXPDIF` | 75 | reduce-and-expand |
| `TROWARGMAX` | `BSTART.SFU TROWARGMAX` | 76 | reduce-and-expand |
| `TROWARGMIN` | `BSTART.SFU TROWARGMIN` | 77 | reduce-and-expand |
| `TCOLSUM` | `BSTART.SFU TCOLSUM` | 80 | reduce-and-expand |
| `TCOLMAX` | `BSTART.SFU TCOLMAX` | 81 | reduce-and-expand |
| `TCOLMIN` | `BSTART.SFU TCOLMIN` | 82 | reduce-and-expand |
| `TCOLPROD` | `BSTART.SFU TCOLPROD` | 83 | reduce-and-expand |
| `TCOLEXPAND` | `BSTART.SFU TCOLEXPAND` | 84 | reduce-and-expand |
| `TCOLEXPANDADD` | `BSTART.SFU TCOLEXPANDADD` | 85 | reduce-and-expand |
| `TCOLEXPANDSUB` | `BSTART.SFU TCOLEXPANDSUB` | 86 | reduce-and-expand |
| `TCOLEXPANDMUL` | `BSTART.SFU TCOLEXPANDMUL` | 87 | reduce-and-expand |
| `TCOLEXPANDDIV` | `BSTART.SFU TCOLEXPANDDIV` | 88 | reduce-and-expand |
| `TCOLEXPANDMAX` | `BSTART.SFU TCOLEXPANDMAX` | 89 | reduce-and-expand |
| `TCOLEXPANDMIN` | `BSTART.SFU TCOLEXPANDMIN` | 90 | reduce-and-expand |
| `TCOLEXPANDEXPDIF` | `BSTART.SFU TCOLEXPANDEXPDIF` | 91 | reduce-and-expand |
| `TCOLARGMAX` | `BSTART.SFU TCOLARGMAX` | 92 | reduce-and-expand |
| `TCOLARGMIN` | `BSTART.SFU TCOLARGMIN` | 93 | reduce-and-expand |
| `TCONCAT` | `BSTART.SFU TCONCAT` | 96 | layout-and-rearrangement |
| `TEXTRACT` | `BSTART.SFU TEXTRACT` | 98 | layout-and-rearrangement |
| `TINSERT` | `BSTART.SFU TINSERT` | 99 | layout-and-rearrangement |
| `TIMG2COL` | `BSTART.SFU TIMG2COL` | 100 | layout-and-rearrangement |
| `TFILLPAD` | `BSTART.SFU TFILLPAD` | 101 | layout-and-rearrangement |
| `TCI` | `BSTART.SFU TCI` | 102 | irregular-and-complex |
| `TTRI` | `BSTART.SFU TTRI` | 103 | irregular-and-complex |
| `THISTOGRAM` | `BSTART.SFU THISTOGRAM` | 104 | irregular-and-complex |
| `TQUANT` | `BSTART.SFU TQUANT` | 106 | irregular-and-complex |
| `TDEQUANT` | `BSTART.SFU TDEQUANT` | 107 | irregular-and-complex |
| `TSORT` | `BSTART.SFU TSORT` | 108 | irregular-and-complex |
| `TMRGSORT` | `BSTART.SFU TMRGSORT` | 109 | irregular-and-complex |
| `TTRANS` | `BSTART.SFU TTRANS` | 110 | layout-and-rearrangement |
| `TGATHER` | `BSTART.SFU TGATHER` | 111 | irregular-and-complex |
| `TSCATTER` | `BSTART.SFU TSCATTER` | 112 | irregular-and-complex |
| `TPARTADD` | `BSTART.SFU TPARTADD` | 113 | irregular-and-complex |
| `TPARTMUL` | `BSTART.SFU TPARTMUL` | 114 | irregular-and-complex |
| `TPARTMAX` | `BSTART.SFU TPARTMAX` | 115 | irregular-and-complex |
| `TPARTMIN` | `BSTART.SFU TPARTMIN` | 116 | irregular-and-complex |

## TLSU

| 操作 | 规范 block 起始指令 | Function |
| --- | --- | ---: |
| `TLOAD` | `BSTART.TLSU TLOAD` | 0 |
| `TSTORE` | `BSTART.TLSU TSTORE` | 1 |
| `TMOV` | `BSTART.TLSU TMOV` | 2 |
| `TPREFETCH` | `BSTART.TLSU TPREFETCH` | 3 |
| `MGATHER` | `BSTART.TLSU MGATHER` | 4 |
| `MSCATTER` | `BSTART.TLSU MSCATTER` | 5 |
| `MGATHER_MASK` | `BSTART.TLSU MGATHER.MASK` | 6 |
| `MSCATTER_MASK` | `BSTART.TLSU MSCATTER.MASK` | 7 |
| `MGATHER_CAS` | `BSTART.TLSU MGATHER.CAS` | 8 |
| `GMOV` | `BSTART.TLSU GMOV` | 13 |

## CUBE

| 操作 | 规范 block 起始指令 | Function |
| --- | --- | ---: |
| `TMATMUL` | `BSTART.CUBE TMATMUL` | 0 |
| `TMATMUL_BIAS` | `BSTART.CUBE TMATMUL.BIAS` | 1 |
| `TMATMUL_ACC` | `BSTART.CUBE TMATMUL.ACC` | 2 |
| `TMATMUL_MX` | `BSTART.CUBE TMATMULMX` | 4 |
| `TMATMUL_MX_BIAS` | `BSTART.CUBE TMATMULMX.BIAS` | 5 |
| `TMATMUL_MX_ACC` | `BSTART.CUBE TMATMULMX.ACC` | 6 |
| `TGEMV` | `BSTART.CUBE TGEMV` | 16 |
| `TGEMV_BIAS` | `BSTART.CUBE TGEMV.BIAS` | 17 |
| `TGEMV_ACC` | `BSTART.CUBE TGEMV.ACC` | 18 |
| `TGEMV_MX` | `BSTART.CUBE TGEMVMX` | 20 |
| `TGEMV_MX_BIAS` | `BSTART.CUBE TGEMVMX.BIAS` | 21 |
| `TGEMV_MX_ACC` | `BSTART.CUBE TGEMVMX.ACC` | 22 |

## 分类语义

PTO ISA 0.58.3（ADR 0057）将执行引擎与操作分类解耦。`elementwise-tile-tile`
和 `tile-scalar-and-immediate` 类别在 VEC 上执行逐元素操作，但其中的
`TEXP`、`TLOG`、`TRECIP`、`TSQRT`、`TRSQRT` 由 SFU 执行。
`reduce-and-expand`、`layout-and-rearrangement` 和 `irregular-and-complex` 类别由 SFU 执行。
所有 TLSU 和 CUBE 操作均由对应的引擎类别执行。`BSTART.VEC` / `BSTART.SFU`
拼写是唯一 `BSTART.TEPL` 编译 carrier 的规范别名（ADR 0057）。

## 早期版本中移除的操作

0.58 之前的版本还提供了一些已从当前目录移除的 Tile 操作（例如 ACC 风格的后处理辅助操作）。
本库不会生成任何已移除的操作；废弃名称的规范列表记录在 contract 的 `deleted_tile_names` 字段中。

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using TileT = Tile<Location::Vec, float, 8, 32>;

// 例如从 VEC 操作页面调用 C++ wrapper。
void add(TileT &dst, TileT &lhs, TileT &rhs) {
  TADD(dst, lhs, rhs);
}
```
