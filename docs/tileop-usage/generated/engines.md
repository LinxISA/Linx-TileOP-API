# LinxISA / PTO ISA v0.59.0 wrapper engine compatibility view

架构定义的引擎类别只有 **VEC**, **TLSU**, **CUBE**, and **SFU**。
VEC 只包含逐元素操作；SFU 包含归约、广播、变换、排序以及其他需要更复杂硬件的操作。
TEPL 仍是唯一的编译 carrier 标识。`BSTART.VEC` 和 `BSTART.SFU` 是特定引擎的汇编别名；
inline wrapper 保留 `BSTART.TEPL`，以兼容之前的工具链源码。

下表根据本仓库固定的 LinxISA/PTO development golden 投影生成，数据记录在
[`contracts/linxisa-v0.58-engine-ops.json`](../../../contracts/linxisa-v0.58-engine-ops.json) 中。
Golden 共 119 项；其中 119 项在 `include/` 有名称引用，
0 项明确没有 wrapper。名称引用不证明 ASL/NDF 语义完整实现。

## VEC

| Golden 操作 | 规范汇编 | 逻辑 selector | 分类 | Wrapper 状态 |
| --- | --- | ---: | --- | --- |
| `TADD` | `BSTART.VEC TADD` | 0 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TSUB` | `BSTART.VEC TSUB` | 1 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TMUL` | `BSTART.VEC TMUL` | 2 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TAND` | `BSTART.VEC TAND` | 6 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TOR` | `BSTART.VEC TOR` | 7 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TXOR` | `BSTART.VEC TXOR` | 8 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TSHL` | `BSTART.VEC TSHL` | 9 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TSHR` | `BSTART.VEC TSHR` | 10 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TMAX` | `BSTART.VEC TMAX` | 11 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TMIN` | `BSTART.VEC TMIN` | 12 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TCMP` | `BSTART.VEC TCMP` | 13 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TABS` | `BSTART.VEC TABS` | 15 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TNOT` | `BSTART.VEC TNOT` | 16 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TNEG` | `BSTART.VEC TNEG` | 17 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TRELU` | `BSTART.VEC TRELU` | 23 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TSEL` | `BSTART.VEC TSEL` | 26 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TCVT` | `BSTART.VEC TCVT` | 27 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TFMA` | `BSTART.VEC TFMA` | 28 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TADDS` | `BSTART.VEC TADDS` | 32 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TSUBS` | `BSTART.VEC TSUBS` | 33 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TMULS` | `BSTART.VEC TMULS` | 34 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TANDS` | `BSTART.VEC TANDS` | 38 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TORS` | `BSTART.VEC TORS` | 39 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TXORS` | `BSTART.VEC TXORS` | 40 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TSHLS` | `BSTART.VEC TSHLS` | 41 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TSHRS` | `BSTART.VEC TSHRS` | 42 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TMAXS` | `BSTART.VEC TMAXS` | 43 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TMINS` | `BSTART.VEC TMINS` | 44 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TCMPS` | `BSTART.VEC TCMPS` | 45 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TSELS` | `BSTART.VEC TSELS` | 58 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TEXPANDS` | `BSTART.VEC TEXPANDS` | 59 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TLEA` | `BSTART.VEC TLEA` | 46 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |

## SFU

| Golden 操作 | 规范汇编 | 逻辑 selector | 分类 | Wrapper 状态 |
| --- | --- | ---: | --- | --- |
| `TDIV` | `BSTART.SFU TDIV` | 3 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TREM` | `BSTART.SFU TREM` | 4 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TEXP` | `BSTART.SFU TEXP` | 18 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TLOG` | `BSTART.SFU TLOG` | 19 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TRECIP` | `BSTART.SFU TRECIP` | 20 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TSQRT` | `BSTART.SFU TSQRT` | 21 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TRSQRT` | `BSTART.SFU TRSQRT` | 22 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |
| `TDIVS` | `BSTART.SFU TDIVS` | 35 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TREMS` | `BSTART.SFU TREMS` | 36 | tile-scalar-and-immediate | 源码有引用；不代表完整语义一致 |
| `TROWSUM` | `BSTART.SFU TROWSUM` | 64 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWMAX` | `BSTART.SFU TROWMAX` | 65 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWMIN` | `BSTART.SFU TROWMIN` | 66 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWPROD` | `BSTART.SFU TROWPROD` | 67 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPAND` | `BSTART.SFU TROWEXPAND` | 68 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDADD` | `BSTART.SFU TROWEXPANDADD` | 69 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDSUB` | `BSTART.SFU TROWEXPANDSUB` | 70 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDMUL` | `BSTART.SFU TROWEXPANDMUL` | 71 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDDIV` | `BSTART.SFU TROWEXPANDDIV` | 72 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDMAX` | `BSTART.SFU TROWEXPANDMAX` | 73 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDMIN` | `BSTART.SFU TROWEXPANDMIN` | 74 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWEXPANDEXPDIF` | `BSTART.SFU TROWEXPANDEXPDIF` | 75 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWARGMAX` | `BSTART.SFU TROWARGMAX` | 76 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TROWARGMIN` | `BSTART.SFU TROWARGMIN` | 77 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLSUM` | `BSTART.SFU TCOLSUM` | 80 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLMAX` | `BSTART.SFU TCOLMAX` | 81 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLMIN` | `BSTART.SFU TCOLMIN` | 82 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLPROD` | `BSTART.SFU TCOLPROD` | 83 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPAND` | `BSTART.SFU TCOLEXPAND` | 84 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDADD` | `BSTART.SFU TCOLEXPANDADD` | 85 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDSUB` | `BSTART.SFU TCOLEXPANDSUB` | 86 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDMUL` | `BSTART.SFU TCOLEXPANDMUL` | 87 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDDIV` | `BSTART.SFU TCOLEXPANDDIV` | 88 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDMAX` | `BSTART.SFU TCOLEXPANDMAX` | 89 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDMIN` | `BSTART.SFU TCOLEXPANDMIN` | 90 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLEXPANDEXPDIF` | `BSTART.SFU TCOLEXPANDEXPDIF` | 91 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLARGMAX` | `BSTART.SFU TCOLARGMAX` | 92 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCOLARGMIN` | `BSTART.SFU TCOLARGMIN` | 93 | reduce-and-expand | 源码有引用；不代表完整语义一致 |
| `TCI` | `BSTART.SFU TCI` | 102 | irregular-and-complex | 源码有引用；不代表完整语义一致 |
| `TTRI` | `BSTART.SFU TTRI` | 103 | irregular-and-complex | 源码有引用；不代表完整语义一致 |
| `TGATHER` | `BSTART.SFU TGATHER` | 111 | irregular-and-complex | 源码有引用；不代表完整语义一致 |
| `TSCATTER` | `BSTART.SFU TSCATTER` | 112 | irregular-and-complex | 源码有引用；不代表完整语义一致 |
| `TPERMUTE` | `BSTART.SFU TPERMUTE` | 117 | layout-and-rearrangement | 源码有引用；不代表完整语义一致 |
| `TSHUF` | `BSTART.SFU TSHUF` | 118 | layout-and-rearrangement | 源码有引用；不代表完整语义一致 |
| `TPACK` | `BSTART.SFU TPACK` | 119 | layout-and-rearrangement | 源码有引用；不代表完整语义一致 |
| `TUNPACK` | `BSTART.SFU TUNPACK` | 120 | layout-and-rearrangement | 源码有引用；不代表完整语义一致 |
| `TGPR2T` | `BSTART.SFU TGPR2T` | 126 | layout-and-rearrangement | 源码有引用；不代表完整语义一致 |
| `TEXPDIF` | `BSTART.SFU TEXPDIF` | 29 | elementwise-tile-tile | 源码有引用；不代表完整语义一致 |

## TLSU

| Golden 操作 | 规范 block 起始指令 | Function | Wrapper 状态 |
| --- | --- | ---: | --- |
| `TLOAD` | `BSTART.TLOAD` | 0 | 源码有引用；不代表完整语义一致 |
| `TSTORE` | `BSTART.TSTORE` | 1 | 源码有引用；不代表完整语义一致 |
| `TMOV` | `BSTART.TMOV` | 2 | 源码有引用；不代表完整语义一致 |
| `TPREFETCH` | `BSTART.TPREFETCH` | 3 | 源码有引用；不代表完整语义一致 |
| `MGATHER` | `BSTART.MGATHER` | 4 | 源码有引用；不代表完整语义一致 |
| `MSCATTER` | `BSTART.MSCATTER` | 5 | 源码有引用；不代表完整语义一致 |
| `MGATHER_MASK` | `BSTART.MGATHER.MASK` | 6 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_MASK` | `BSTART.MSCATTER.MASK` | 7 | 源码有引用；不代表完整语义一致 |
| `MGATHER_CAS` | `BSTART.MGATHER.CAS` | 8 | 源码有引用；不代表完整语义一致 |
| `GMOV` | `BSTART.GMOV` | 13 | 源码有引用；不代表完整语义一致 |
| `MGATHER_EXCH` | `BSTART.MGATHER.EXCH` | 9 | 源码有引用；不代表完整语义一致 |
| `MGATHER_MAX` | `BSTART.MGATHER.MAX` | 10 | 源码有引用；不代表完整语义一致 |
| `MGATHER_MIN` | `BSTART.MGATHER.MIN` | 11 | 源码有引用；不代表完整语义一致 |
| `MGATHER_ADD` | `BSTART.MGATHER.ADD` | 12 | 源码有引用；不代表完整语义一致 |
| `MGATHER_INC` | `BSTART.MGATHER.INC` | 14 | 源码有引用；不代表完整语义一致 |
| `MGATHER_DEC` | `BSTART.MGATHER.DEC` | 15 | 源码有引用；不代表完整语义一致 |
| `MGATHER_AND` | `BSTART.MGATHER.AND` | 16 | 源码有引用；不代表完整语义一致 |
| `MGATHER_OR` | `BSTART.MGATHER.OR` | 17 | 源码有引用；不代表完整语义一致 |
| `MGATHER_XOR` | `BSTART.MGATHER.XOR` | 18 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_MAX` | `BSTART.MSCATTER.MAX` | 19 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_MIN` | `BSTART.MSCATTER.MIN` | 20 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_ADD` | `BSTART.MSCATTER.ADD` | 21 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_INC` | `BSTART.MSCATTER.INC` | 22 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_DEC` | `BSTART.MSCATTER.DEC` | 23 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_AND` | `BSTART.MSCATTER.AND` | 24 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_OR` | `BSTART.MSCATTER.OR` | 25 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_XOR` | `BSTART.MSCATTER.XOR` | 26 | 源码有引用；不代表完整语义一致 |
| `MSCATTER_POPC` | `BSTART.MSCATTER.POPC` | 27 | 源码有引用；不代表完整语义一致 |

## CUBE

| Golden 操作 | 规范 block 起始指令 | Function | Wrapper 状态 |
| --- | --- | ---: | --- |
| `TMATMUL` | `BSTART.TMATMUL` | 0 | 源码有引用；不代表完整语义一致 |
| `TMATMUL_BIAS` | `BSTART.TMATMUL.BIAS` | 1 | 源码有引用；不代表完整语义一致 |
| `TMATMUL_ACC` | `BSTART.TMATMUL.ACC` | 2 | 源码有引用；不代表完整语义一致 |
| `TMATMUL_MX` | `BSTART.TMATMULMX` | 4 | 源码有引用；不代表完整语义一致 |
| `TMATMUL_MX_BIAS` | `BSTART.TMATMULMX.BIAS` | 5 | 源码有引用；不代表完整语义一致 |
| `TMATMUL_MX_ACC` | `BSTART.TMATMULMX.ACC` | 6 | 源码有引用；不代表完整语义一致 |
| `TGEMV` | `BSTART.TGEMV` | 16 | 源码有引用；不代表完整语义一致 |
| `TGEMV_BIAS` | `BSTART.TGEMV.BIAS` | 17 | 源码有引用；不代表完整语义一致 |
| `TGEMV_ACC` | `BSTART.TGEMV.ACC` | 18 | 源码有引用；不代表完整语义一致 |
| `TGEMV_MX` | `BSTART.TGEMVMX` | 20 | 源码有引用；不代表完整语义一致 |
| `TGEMV_MX_BIAS` | `BSTART.TGEMVMX.BIAS` | 21 | 源码有引用；不代表完整语义一致 |
| `TGEMV_MX_ACC` | `BSTART.TGEMVMX.ACC` | 22 | 源码有引用；不代表完整语义一致 |

## 分类语义

PTO ISA 0.59.0 将执行引擎与操作分类解耦。本表是 wrapper compatibility view；
其底层投影记录当前 development golden selector 数据。`elementwise-tile-tile`
和 `tile-scalar-and-immediate` 类别在 VEC 上执行逐元素操作，但其中的
`TEXP`、`TLOG`、`TRECIP`、`TSQRT`、`TRSQRT` 由 SFU 执行。
`reduce-and-expand`、`layout-and-rearrangement` 和 `irregular-and-complex` 类别由 SFU 执行。
所有 TLSU 和 CUBE 操作均由对应的引擎类别执行。`BSTART.VEC` / `BSTART.SFU`
拼写是唯一 `BSTART.TEPL` 编译 carrier 的规范别名（ADR 0057）。

## 早期版本中移除的操作

历史版本还提供了一些已从 PTO 0.59.0 active catalog 移除的 Tile 操作（例如 ACC 风格的后处理辅助操作）。
本库不会把已退役的操作生成到当前目录；退役名称的规范列表记录在 contract 的 `deleted_tile_names` 字段中。

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
