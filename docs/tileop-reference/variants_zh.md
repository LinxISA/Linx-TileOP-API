# TileOP 变体索引

优先使用操作页给出的统一入口。Shared 搬运、分段写回和单 PE 卷积窗口加载分别见 [TMOV](ops/TMOV_zh.md)、[TSTORE](ops/TSTORE_zh.md) 和 [TIMG2COL](ops/TIMG2COL_zh.md) 的同名重载。Shared 寄存器编号由编译器分配。

## 关联写入 `_ASS`
| C++ 入口 | 操作页 | 用途 |
|---|---|---|
| `TABS_ASS` | [TABS](ops/TABS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TADDS_ASS` | [TADDS](ops/TADDS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TADD_ASS` | [TADD](ops/TADD_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TANDS_ASS` | [TANDS](ops/TANDS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TAND_ASS` | [TAND](ops/TAND_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCMPS_ASS` | [TCMPS](ops/TCMPS_zh.md) | 将 Tile 与标量的比较结果写入已关联的组装目标。 |
| `TCMP_ASS` | [TCMP](ops/TCMP_zh.md) | 将两个 Tile 的比较结果写入已关联的组装目标。 |
| `TCOLEXPANDADD_ASS` | [TCOLEXPANDADD](ops/TCOLEXPANDADD_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPANDDIV_ASS` | [TCOLEXPANDDIV](ops/TCOLEXPANDDIV_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPANDEXPDIF_ASS` | [TCOLEXPANDEXPDIF](ops/TCOLEXPANDEXPDIF_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPANDMAX_ASS` | [TCOLEXPANDMAX](ops/TCOLEXPANDMAX_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPANDMIN_ASS` | [TCOLEXPANDMIN](ops/TCOLEXPANDMIN_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPANDMUL_ASS` | [TCOLEXPANDMUL](ops/TCOLEXPANDMUL_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPANDSUB_ASS` | [TCOLEXPANDSUB](ops/TCOLEXPANDSUB_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLEXPAND_ASS` | [TCOLEXPAND](ops/TCOLEXPAND_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLMAX_ASS` | [TCOLMAX](ops/TCOLMAX_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLMIN_ASS` | [TCOLMIN](ops/TCOLMIN_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLPROD_ASS` | [TCOLPROD](ops/TCOLPROD_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCOLSUM_ASS` | [TCOLSUM](ops/TCOLSUM_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TCVT_ASS` | [TCVT](ops/TCVT_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TDIVS_ASS` | [TDIVS](ops/TDIVS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TDIV_ASS` | [TDIV](ops/TDIV_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TEXP_ASS` | [TEXP](ops/TEXP_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TFMA_ASS` | [TFMA](ops/TFMA_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TLOAD_ASS` | [TLOAD](ops/TLOAD_zh.md) | 将 GM 数据载入已关联的目标。 |
| `TLOG_ASS` | [TLOG](ops/TLOG_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TMAXS_ASS` | [TMAXS](ops/TMAXS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TMAX_ASS` | [TMAX](ops/TMAX_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TMINS_ASS` | [TMINS](ops/TMINS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TMIN_ASS` | [TMIN](ops/TMIN_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TMULS_ASS` | [TMULS](ops/TMULS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TMUL_ASS` | [TMUL](ops/TMUL_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TNEG_ASS` | [TNEG](ops/TNEG_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TNOT_ASS` | [TNOT](ops/TNOT_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TORS_ASS` | [TORS](ops/TORS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TOR_ASS` | [TOR](ops/TOR_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TPACK_ASS` | [TPACK](ops/TPACK_zh.md) | 将打包结果写入已关联的 CUBE 组装目标。 |
| `TRECIP_ASS` | [TRECIP](ops/TRECIP_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TRELU_ASS` | [TRELU](ops/TRELU_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TREMS_ASS` | [TREMS](ops/TREMS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TREM_ASS` | [TREM](ops/TREM_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDADD_ASS` | [TROWEXPANDADD](ops/TROWEXPANDADD_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDDIV_ASS` | [TROWEXPANDDIV](ops/TROWEXPANDDIV_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDEXPDIF_ASS` | [TROWEXPANDEXPDIF](ops/TROWEXPANDEXPDIF_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDMAX_ASS` | [TROWEXPANDMAX](ops/TROWEXPANDMAX_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDMIN_ASS` | [TROWEXPANDMIN](ops/TROWEXPANDMIN_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDMUL_ASS` | [TROWEXPANDMUL](ops/TROWEXPANDMUL_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPANDSUB_ASS` | [TROWEXPANDSUB](ops/TROWEXPANDSUB_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWEXPAND_ASS` | [TROWEXPAND](ops/TROWEXPAND_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWMAX_ASS` | [TROWMAX](ops/TROWMAX_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWMIN_ASS` | [TROWMIN](ops/TROWMIN_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWPROD_ASS` | [TROWPROD](ops/TROWPROD_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TROWSUM_ASS` | [TROWSUM](ops/TROWSUM_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TRSQRT_ASS` | [TRSQRT](ops/TRSQRT_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSHLS_ASS` | [TSHLS](ops/TSHLS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSHL_ASS` | [TSHL](ops/TSHL_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSHRS_ASS` | [TSHRS](ops/TSHRS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSHR_ASS` | [TSHR](ops/TSHR_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSQRT_ASS` | [TSQRT](ops/TSQRT_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSUBS_ASS` | [TSUBS](ops/TSUBS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TSUB_ASS` | [TSUB](ops/TSUB_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TXORS_ASS` | [TXORS](ops/TXORS_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |
| `TXOR_ASS` | [TXOR](ops/TXOR_zh.md) | 关联写入重载，检查目标 carrier 和源布局。 |

## 二维索引
| C++ 入口 | 操作页 | 用途 |
|---|---|---|
| `TCI_2D` | [TCI](ops/TCI_zh.md) | 编译期指定行、列步长。 |

## 规则地址原子写回
| C++ 入口 | 操作页 | 用途 |
|---|---|---|
| `TSTORE_ATOMIC_ADD` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_AND` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_DEC` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_INC` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_MAX` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_MIN` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_OR` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |
| `TSTORE_ATOMIC_XOR` | [TSTORE](ops/TSTORE_zh.md) | 规则地址原子更新，数据类型与副作用见 `TSTORE`。 |

[返回操作索引](README.md)
