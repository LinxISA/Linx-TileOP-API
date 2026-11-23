# PTO-C TileOP 接口参考

Linx TileOP API `0.59` · PTO ISA `0.59.0`

按功能分类查找 TileOP。每个操作页提供语义、C++ 调用、参数和约束、示例以及相应的 ISA 规范。

## 文档目录

| 入口 | 内容 |
|---|---|
| [操作索引](#操作索引) | 活动 catalog 的 119 项操作。 |
| [变体索引](variants_zh.md) | 调用者需要显式使用的关联写入、二维索引和原子写回入口。 |
| [辅助接口](helpers_zh.md) | 分片、组装、谓词、运行时与块接口。 |

## 使用环境

使用包含目标 sysroot 和本手册配套 TileOP 头文件的 Linx SDK。Shared 路径的 `TMOV`、`TSTORE` 和 `TIMG2COL` 重载要求 SDK 安装了包含本手册对应变更的 `0.59` 头文件。仓库更新后，已安装的 SDK 不会自动更新。示例统一从 `<common/pto_tileop.hpp>` 引入接口；除完整 kernel 示例外，调用方负责初始化输入 Tile 或 GM 数据。

```bash
"$LINX_SDK/bin/clang++" --target=linx64v5-unknown-linux-musl \
  -mlxbc -fenable-matrix -D__linx -std=c++20 -O2 -c kernel.cpp -o kernel.o
```

SDK 通过默认路径加载目标 sysroot 和内置 TileOP。独立 Clang 需要按其安装情况显式提供目标 sysroot 与头文件路径。可用 `-H -fsyntax-only` 确认实际加载的头文件，并在同一安装目录的 `common/pto_tileop_api_revision.hpp` 中查看 `PTO_TILEOP_API_REVISION`，核对 SDK 打包的 Git 提交。

本参考以 TileOP `0.59` 和 PTO ISA `v0.59.0.0` 为基础，对照 active 规范 `42067d0f2890`。JCORE、CPU simulator、LLVM 和 AArch64/SME 的支持范围需分别确认。

## 操作索引

### 逐元素 Tile–Tile（26 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TADD](ops/TADD_zh.md) | `VEC` | 对两个 Tile 的对应元素求和。 |
| [TSUB](ops/TSUB_zh.md) | `VEC` | 从左 Tile 的每个元素减去右 Tile 对应元素。 |
| [TMUL](ops/TMUL_zh.md) | `VEC` | 对两个 Tile 的对应元素求积。 |
| [TDIV](ops/TDIV_zh.md) | `SFU` | 对两个 Tile 的对应元素做除法。 |
| [TREM](ops/TREM_zh.md) | `SFU` | 对两个 Tile 的对应元素取模，非零结果符号随除数。 |
| [TAND](ops/TAND_zh.md) | `VEC` | 对对应元素执行按位与。 |
| [TOR](ops/TOR_zh.md) | `VEC` | 对对应元素执行按位或。 |
| [TXOR](ops/TXOR_zh.md) | `VEC` | 对对应元素执行按位异或。 |
| [TSHL](ops/TSHL_zh.md) | `VEC` | 以右 Tile 的对应元素指定左移位数。 |
| [TSHR](ops/TSHR_zh.md) | `VEC` | 以右 Tile 的对应元素指定右移位数。 |
| [TMAX](ops/TMAX_zh.md) | `VEC` | 对对应元素取较大值。 |
| [TMIN](ops/TMIN_zh.md) | `VEC` | 对对应元素取较小值。 |
| [TCMP](ops/TCMP_zh.md) | `VEC` | 用 `CmpMode` 比较两个 Tile，生成谓词结果。 |
| [TABS](ops/TABS_zh.md) | `VEC` | 取逐元素绝对值。 |
| [TNOT](ops/TNOT_zh.md) | `VEC` | 对整数元素逐位取反。 |
| [TNEG](ops/TNEG_zh.md) | `VEC` | 对各元素取负。 |
| [TEXP](ops/TEXP_zh.md) | `SFU` | 计算逐元素自然指数。 |
| [TLOG](ops/TLOG_zh.md) | `SFU` | 计算逐元素自然对数。 |
| [TRECIP](ops/TRECIP_zh.md) | `SFU` | 计算逐元素倒数。 |
| [TSQRT](ops/TSQRT_zh.md) | `SFU` | 计算逐元素平方根。 |
| [TRSQRT](ops/TRSQRT_zh.md) | `SFU` | 计算逐元素平方根的倒数。 |
| [TRELU](ops/TRELU_zh.md) | `VEC` | 对各元素执行 ReLU。 |
| [TSEL](ops/TSEL_zh.md) | `VEC` | 用谓词逐元素选择 true/false 数据。 |
| [TCVT](ops/TCVT_zh.md) | `VEC` | 按指定舍入规则执行数值类型转换。 |
| [TFMA](ops/TFMA_zh.md) | `VEC` | 读取三个 Tile，执行逐元素融合乘加。 |
| [TEXPDIF](ops/TEXPDIF_zh.md) | `SFU` | 在同一个操作中计算逐元素差的指数。 |

### Tile 标量与立即数（16 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TADDS](ops/TADDS_zh.md) | `VEC` | 将标量加到各元素。 |
| [TSUBS](ops/TSUBS_zh.md) | `VEC` | 从各元素减去标量。 |
| [TMULS](ops/TMULS_zh.md) | `VEC` | 将各元素乘以标量。 |
| [TDIVS](ops/TDIVS_zh.md) | `SFU` | 将各元素除以标量。 |
| [TREMS](ops/TREMS_zh.md) | `SFU` | 以标量为除数逐元素取模。 |
| [TANDS](ops/TANDS_zh.md) | `VEC` | 各元素与标量按位与。 |
| [TORS](ops/TORS_zh.md) | `VEC` | 各元素与标量按位或。 |
| [TXORS](ops/TXORS_zh.md) | `VEC` | 各元素与标量按位异或。 |
| [TSHLS](ops/TSHLS_zh.md) | `VEC` | 按标量位数逐元素左移。 |
| [TSHRS](ops/TSHRS_zh.md) | `VEC` | 按标量位数逐元素右移。 |
| [TMAXS](ops/TMAXS_zh.md) | `VEC` | 对各元素和标量取较大值。 |
| [TMINS](ops/TMINS_zh.md) | `VEC` | 对各元素和标量取较小值。 |
| [TCMPS](ops/TCMPS_zh.md) | `VEC` | 对各元素和标量做指定比较，生成谓词结果。 |
| [TSELS](ops/TSELS_zh.md) | `VEC` | 按谓词选择 Tile 元素或标量。 |
| [TEXPANDS](ops/TEXPANDS_zh.md) | `VEC` | 将标量填到目标 Tile 的有效区域。 |
| [TLEA](ops/TLEA_zh.md) | `VEC` | 将元素索引缩放为 64 位字节位移，供 indexed TLSU 使用。 |

### 归约与广播（28 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TROWSUM](ops/TROWSUM_zh.md) | `SFU` | 逐行求和，输出有效区域为 `R × 1`。 |
| [TROWMAX](ops/TROWMAX_zh.md) | `SFU` | 逐行求最大值，输出有效区域为 `R × 1`。 |
| [TROWMIN](ops/TROWMIN_zh.md) | `SFU` | 逐行求最小值，输出有效区域为 `R × 1`。 |
| [TROWPROD](ops/TROWPROD_zh.md) | `SFU` | 逐行求积，输出有效区域为 `R × 1`。 |
| [TROWEXPAND](ops/TROWEXPAND_zh.md) | `SFU` | 将 `R × 1` 广播为 `R × C`。 |
| [TROWEXPANDADD](ops/TROWEXPANDADD_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵相加。 |
| [TROWEXPANDSUB](ops/TROWEXPANDSUB_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵做减法。 |
| [TROWEXPANDMUL](ops/TROWEXPANDMUL_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵相乘。 |
| [TROWEXPANDDIV](ops/TROWEXPANDDIV_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵做除法。 |
| [TROWEXPANDMAX](ops/TROWEXPANDMAX_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵取较大值。 |
| [TROWEXPANDMIN](ops/TROWEXPANDMIN_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵取较小值。 |
| [TROWEXPANDEXPDIF](ops/TROWEXPANDEXPDIF_zh.md) | `SFU` | 将 `R × 1` 按行广播，与矩阵计算差的指数。 |
| [TROWARGMAX](ops/TROWARGMAX_zh.md) | `SFU` | 逐行求最大值所在索引，输出有效区域为 `R × 1`。 |
| [TROWARGMIN](ops/TROWARGMIN_zh.md) | `SFU` | 逐行求最小值所在索引，输出有效区域为 `R × 1`。 |
| [TCOLSUM](ops/TCOLSUM_zh.md) | `SFU` | 逐列求和，输出有效区域为 `1 × C`。 |
| [TCOLMAX](ops/TCOLMAX_zh.md) | `SFU` | 逐列求最大值，输出有效区域为 `1 × C`。 |
| [TCOLMIN](ops/TCOLMIN_zh.md) | `SFU` | 逐列求最小值，输出有效区域为 `1 × C`。 |
| [TCOLPROD](ops/TCOLPROD_zh.md) | `SFU` | 逐列求积，输出有效区域为 `1 × C`。 |
| [TCOLEXPAND](ops/TCOLEXPAND_zh.md) | `SFU` | 将 `1 × C` 广播为 `R × C`。 |
| [TCOLEXPANDADD](ops/TCOLEXPANDADD_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵相加。 |
| [TCOLEXPANDSUB](ops/TCOLEXPANDSUB_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵做减法。 |
| [TCOLEXPANDMUL](ops/TCOLEXPANDMUL_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵相乘。 |
| [TCOLEXPANDDIV](ops/TCOLEXPANDDIV_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵做除法。 |
| [TCOLEXPANDMAX](ops/TCOLEXPANDMAX_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵取较大值。 |
| [TCOLEXPANDMIN](ops/TCOLEXPANDMIN_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵取较小值。 |
| [TCOLEXPANDEXPDIF](ops/TCOLEXPANDEXPDIF_zh.md) | `SFU` | 将 `1 × C` 按列广播，与矩阵计算差的指数。 |
| [TCOLARGMAX](ops/TCOLARGMAX_zh.md) | `SFU` | 逐列求最大值所在索引，输出有效区域为 `1 × C`。 |
| [TCOLARGMIN](ops/TCOLARGMIN_zh.md) | `SFU` | 逐列求最小值所在索引，输出有效区域为 `1 × C`。 |

### 初始化与不规则访问（4 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TCI](ops/TCI_zh.md) | `SFU` | 以给定起始值生成索引 Tile。`TCI_2D` 提供受限二维步长。 |
| [TTRI](ops/TTRI_zh.md) | `SFU` | 按操作参数生成三角形 Tile。 |
| [TGATHER](ops/TGATHER_zh.md) | `SFU` | 在 Tile 内按索引采集元素。 |
| [TSCATTER](ops/TSCATTER_zh.md) | `SFU` | 在 Tile 内按索引放置元素。 |

### GM 搬运与索引访存（27 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TLOAD](ops/TLOAD_zh.md) | `TLSU` | 从 GM 搬入 Local 或 Shared，可随重载转换布局。 |
| [TSTORE](ops/TSTORE_zh.md) | `TLSU` | 将 Tile 写回 GM，可随重载转换布局或执行 reduction。 |
| [TPREFETCH](ops/TPREFETCH_zh.md) | `TLSU` | 按受支持的区域和参数请求 GM 预取。 |
| [MGATHER](ops/MGATHER_zh.md) | `TLSU` | 从相对 GM base 的字节地址采集数据到 Tile。 |
| [MSCATTER](ops/MSCATTER_zh.md) | `TLSU` | 将数据 Tile 的元素写到指定 GM 字节地址。 |
| [MGATHER_MASK](ops/MGATHER_MASK_zh.md) | `TLSU` | 以掩码控制参与读取的索引 lane。 |
| [MSCATTER_MASK](ops/MSCATTER_MASK_zh.md) | `TLSU` | 以掩码控制参与写入的索引 lane。 |
| [MGATHER_CAS](ops/MGATHER_CAS_zh.md) | `TLSU` | 进行 indexed compare-and-swap，返回旧值。 |
| [GMOV](ops/GMOV_zh.md) | `TLSU` | 在操作规定的 PE 间拷贝相同布局的 Local Tile。 |
| [MGATHER_EXCH](ops/MGATHER_EXCH_zh.md) | `TLSU` | 进行 indexed atomic exchange，返回旧值。 |
| [MGATHER_MAX](ops/MGATHER_MAX_zh.md) | `TLSU` | 在 indexed GM 地址执行原子取较大值更新并取得旧值。 |
| [MGATHER_MIN](ops/MGATHER_MIN_zh.md) | `TLSU` | 在 indexed GM 地址执行原子取较小值更新并取得旧值。 |
| [MGATHER_ADD](ops/MGATHER_ADD_zh.md) | `TLSU` | 在 indexed GM 地址执行原子加法更新并取得旧值。 |
| [MGATHER_INC](ops/MGATHER_INC_zh.md) | `TLSU` | 在 indexed GM 地址执行原子递增更新并取得旧值。 |
| [MGATHER_DEC](ops/MGATHER_DEC_zh.md) | `TLSU` | 在 indexed GM 地址执行原子递减更新并取得旧值。 |
| [MGATHER_AND](ops/MGATHER_AND_zh.md) | `TLSU` | 在 indexed GM 地址执行原子按位与更新并取得旧值。 |
| [MGATHER_OR](ops/MGATHER_OR_zh.md) | `TLSU` | 在 indexed GM 地址执行原子按位或更新并取得旧值。 |
| [MGATHER_XOR](ops/MGATHER_XOR_zh.md) | `TLSU` | 在 indexed GM 地址执行原子按位异或更新并取得旧值。 |
| [MSCATTER_MAX](ops/MSCATTER_MAX_zh.md) | `TLSU` | 在 indexed GM 地址执行原子取较大值更新。 |
| [MSCATTER_MIN](ops/MSCATTER_MIN_zh.md) | `TLSU` | 在 indexed GM 地址执行原子取较小值更新。 |
| [MSCATTER_ADD](ops/MSCATTER_ADD_zh.md) | `TLSU` | 在 indexed GM 地址执行原子加法更新。 |
| [MSCATTER_INC](ops/MSCATTER_INC_zh.md) | `TLSU` | 在 indexed GM 地址执行原子递增更新。 |
| [MSCATTER_DEC](ops/MSCATTER_DEC_zh.md) | `TLSU` | 在 indexed GM 地址执行原子递减更新。 |
| [MSCATTER_AND](ops/MSCATTER_AND_zh.md) | `TLSU` | 在 indexed GM 地址执行原子按位与更新。 |
| [MSCATTER_OR](ops/MSCATTER_OR_zh.md) | `TLSU` | 在 indexed GM 地址执行原子按位或更新。 |
| [MSCATTER_XOR](ops/MSCATTER_XOR_zh.md) | `TLSU` | 在 indexed GM 地址执行原子按位异或更新。 |
| [MSCATTER_POPC](ops/MSCATTER_POPC_zh.md) | `TLSU` | 向每个有效索引对应的 GM U32 计数器加 1。 |

### 矩阵与矩阵向量计算（12 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TMATMUL](ops/TMATMUL_zh.md) | `CUBE` | 执行矩阵乘矩阵，发布新结果。 |
| [TMATMUL_BIAS](ops/TMATMUL_BIAS_zh.md) | `CUBE` | 计算矩阵乘矩阵并加入 bias。 |
| [TMATMUL_ACC](ops/TMATMUL_ACC_zh.md) | `CUBE` | 将矩阵乘矩阵结果累加到已有输入累加值。 |
| [TMATMUL_MX](ops/TMATMUL_MX_zh.md) | `CUBE` | 读取 MX 输入及配套 scale，执行矩阵乘矩阵。 |
| [TMATMUL_MX_BIAS](ops/TMATMUL_MX_BIAS_zh.md) | `CUBE` | 加入 bias 的 MX 矩阵乘矩阵。 |
| [TMATMUL_MX_ACC](ops/TMATMUL_MX_ACC_zh.md) | `CUBE` | 将 MX 矩阵乘矩阵结果累加到已有输入累加值。 |
| [TGEMV](ops/TGEMV_zh.md) | `CUBE` | 执行矩阵乘向量，发布新结果。 |
| [TGEMV_BIAS](ops/TGEMV_BIAS_zh.md) | `CUBE` | 计算矩阵乘向量并加入 bias。 |
| [TGEMV_ACC](ops/TGEMV_ACC_zh.md) | `CUBE` | 将矩阵乘向量结果累加到已有输入累加值。 |
| [TGEMV_MX](ops/TGEMV_MX_zh.md) | `CUBE` | 读取 MX 输入及配套 scale，执行矩阵乘向量。 |
| [TGEMV_MX_BIAS](ops/TGEMV_MX_BIAS_zh.md) | `CUBE` | 加入 bias 的 MX 矩阵乘向量。 |
| [TGEMV_MX_ACC](ops/TGEMV_MX_ACC_zh.md) | `CUBE` | 将 MX 矩阵乘向量结果累加到已有输入累加值。 |

### 布局与重排（6 项）

| 操作 | 引擎 | 用途 |
|---|---|---|
| [TMOV](ops/TMOV_zh.md) | `TLSU` | 在支持的 Tile 存储与布局之间搬运 payload。 |
| [TPERMUTE](ops/TPERMUTE_zh.md) | `SFU` | 用 U8 索引在每个 128 B CELL 内对两路数据按字节选值。 |
| [TSHUF](ops/TSHUF_zh.md) | `SFU` | 按 GPR 控制字段在 CELL 行段内重排原始 word 组。 |
| [TPACK](ops/TPACK_zh.md) | `SFU` | 从两路 CUBE 源按字段提取并拼接原始低位字节。 |
| [TUNPACK](ops/TUNPACK_zh.md) | `SFU` | 从 CUBE word 提取字节字段并零扩展到目标载体。 |
| [TGPR2T](ops/TGPR2T_zh.md) | `SFU` | 将四个 GPR predicate plane 的位模式写成数值 U8 Tile。 |

[PTO ISA 活动操作清单](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/spec/catalog/tile-operations.json) · [离线 HTML](index.html)

## 维护说明

更新 `ops/` 下的 Markdown 源文件后，在 TileOP 仓库根目录运行 `python3 docs/tileop-reference/build_html.py` 重新生成 `index.html`。生成需要 Pandoc；阅读时直接打开 HTML，章节导航和搜索无需联网。

本版使用锁定的 TileOP 头文件，对 119 项操作和 `TIMG2COL` 的主示例进行了 Linx 目标文件编译检查。编译检查不覆盖示例在模拟器或硬件上的运行结果，也不表示所有重载和类型组合都经过验证。
