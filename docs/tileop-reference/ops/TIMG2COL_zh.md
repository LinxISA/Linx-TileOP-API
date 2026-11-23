# TIMG2COL

**0.59 块接口**

## 简介

`TIMG2COL` 从 GM feature-map 按打包参数提取窗口，目标可以是 Local CUBE 或 Shared ND。

## 数学语义

按卷积窗口位置读取 GM 元素，使用参数字中的步幅、扩张、窗口与 padding 参数生成目标 Tile 的逻辑片段。

## C++ 接口

```cpp
// 直接物化为 Local CUBE（ND/NHWC 源寻址；ND2M16/ND2M32 布局）
template <is_tile_data_v tile_shape_out, is_global_data_v gm_shape>
void TIMG2COL(tile_shape_out &dst, gm_shape &src,
              uint64_t param0, uint64_t param1, uint64_t param2);

// 协作形式：LB1 携带核心级总行数（1..128），destination 为每-PE 分片
template <is_tile_data_v tile_shape_out, is_global_data_v gm_shape>
void TIMG2COL(tile_shape_out &dst, gm_shape &src,
              uint64_t param0, uint64_t param1, uint64_t param2,
              size_t groupRows);

// SharedND 输出（issue #194）：SourceOrder=NORM（ND/NHWC 源寻址）或
// DN2ND（DN/NCHW 源寻址）；发布到 RowMajor Shared parent
template <LayoutCvtEnum SourceOrder = NORM, is_tile_data_v shp,
          is_global_data_v gm_shape>
void TIMG2COL(SharedTile<shp> &dst, gm_shape &src, TIMG2COLParams params);

// 单-issuer Shared 变体：一个 PE 位掩码的 B.IOS 直接发布完整 parent，
// 不需要 B.ASSEMBLE 世代协议
template <LayoutCvtEnum SourceOrder, unsigned PEMask, is_tile_data_v Local,
          is_global_data_v GM>
void TIMG2COL(SharedTile<Local> &dst, GM &src, TIMG2COLParams params);
```

### 支持的数据类型

按 PTO ISA 0.58.6，TIMG2COL 的 `TileDataType` 合法集合为：

```text
FP32, TF32, HF32, FP16, BF16,
HiF8, E4M3, E5M2, E8M0,
S32, S16, S8, U32, U16, U8
```

上述是 ISA 合法集合；本仓库 C++ wrapper 是否为每一种类型提供可调用重载，
仍以头文件和编译测试为准。

### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | GM feature-map 基址。 |
| `param0` | 输入尺寸、Cin 和 kernel 尺寸的打包参数字。 |
| `param1` | padding、dilation、stride 和扩展控制的打包参数字。 |
| `param2` | 输出 row/column 起始位置的打包参数字。 |

三个参数字按 PTO `Timg2col Parameters` contract 解码：

| 字段 | 范围 |
| --- | --- |
| `input_h`, `input_w`, `cin` | encoded `0..65535`; contract requires nonzero |
| `kernel_h`, `kernel_w` | encoded `0..255`; contract requires nonzero |
| `pad_top/left/bottom/right` | `0..255` |
| `dilation_h`, `dilation_w` | encoded `0..31`; contract requires nonzero |
| `conv_stride_h`, `conv_stride_w` | encoded `0..63`; contract requires nonzero |
| `row_start`, `col_start` | `0..2^32-1` |

基础参数 contract 要求 `ParamVersion[58:55]=0`、
`ExtensionClass[62:59]=0`，并且 `ParamGPR1[54]` 与 `[63]` 必须为零。
参数字中的编码字段虽然可表达零，基础 TIMG2COL shape contract 不接受零的
输入尺寸、kernel、dilation 或 stride。

## 约束与默认值

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 输出 valid region 由布局、偏移或拼接描述符计算；物理 padding 不等于有效数据。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |

### 默认值

参数字必须显式提供；各字段的位分配见 PTO 的 TIMG2COL parameter contract。

#### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；`LB1`/`LB2` 分别给出 `ValidRow`/
  `TotalCol`，三者都必须非零。`ValidCol <= TotalCol`，且 `ValidCol`、
  `TotalCol`、`col_start` 都必须按该 dtype 的 C0 元素数对齐。`row_start + ValidRow`
  不能超过输出窗口数，`col_start + ValidCol` 不能超过有效 K 范围。

### SharedND 输出与 NCHW（issue #194）

直接 Local 输出由 schema 固定为 `ND2M16`/`ND2M32` 转换布局，即 **ND（NHWC）源寻址**
（`spatial*Cin + channel`）。NCHW 源需要 **DN 寻址**（`channel*H*W + spatial`），
规范将该寻址唯一地路由到 SharedND 输出：

| SourceOrder | B.DATR 布局 | GM 寻址 | 适用源布局 |
| --- | --- | --- | --- |
| `NORM` | `NORM` | `spatial*Cin + channel` | NHWC |
| `DN2ND` | `DN2ND` | `channel*H*W + spatial` | NCHW |

- 四-PE 协作形式（`mask=1111` 的 `B.IOS`）要求调用方按 ASL 世代协议补发
  各 PE 的 `B.ASSEMBLE`（INIT/MIDDLE/LAST 阶段），当前封装不代发——需要
  分段写入时请改用单-issuer 变体或自行编码。
- 单 PE 形式使用 `TIMG2COL<SourceOrder, PEMask>(dst, src, params)`。
  `PEMask` 只能是 `1, 2, 4, 8`，由选中的一个 PE 发布完整结果，无需 `B.ASSEMBLE`。
  省略 `PEMask` 时仍使用四 PE Shared 形式。来源布局与发布方式分别选择。
- `SharedTile` 由编译器分配寄存器，必须在当前函数内或通过内联调用使用。

`B.DATR.Layout=29` 和 `31` 是 direct Local `CUBE_M32`/`CUBE_M16` layout
selector；它们不能与 Shared `NORM`/`DN2ND` 的 source-order conversion selector
混为一谈。Shared cooperative output 仍必须使用 `B.IOS`、`PE_MASK=1111`，并按
`INIT → MIDDLE* → LAST` 紧邻 binder 发出 `B.ASSEMBLE`。

## 示例

```cpp
#include <common/pto_tileop.hpp>

void example() {
  using namespace pto;
  using GM = global_tensor<float, RowMajor<8, 256>>;
  using TileT = CubeTileM32<float, 32, 256>;
  float src_data[8 * 256] = {};
  GM src_global(src_data);
  TileT dst;
  TIMG2COL(dst, src_global, 0x0008000800040008ULL, 0, 0);
}
```

NCHW 源经 SharedND 物化（DN2ND 寻址）：

```cpp
using namespace pto;
using SharedND = SharedTile<Tile<Location::Vec, __half, 64, 64,
                                     BLayout::RowMajor, 64, 64>>;
using FeatureGM = global_tensor<__half, RowMajor<64, 64>>;
void nchw_single_pe(FeatureGM &feature, FeatureGM &output) {
  SharedND shared;
  TIMG2COL<DN2ND, 1>(shared, feature,
                    TIMG2COLParams{0x0008000800040008ULL, 0, 0});
  TSTORE<1>(output, shared);
}
```

涉及不同输入尺寸、padding、stride、dilation 或起始位置时，请按
TIMG2COL parameter contract 重新打包三个参数字，而不是使用旧的二维位置参数。

## Bundle 对照

```asm
BSTART.TIMG2COL DataType
    B.DATR       ND2M32, DTYPE_NONE, Zero  // Local CUBE_M32 conversion
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1
B.DIM       rTotalCol, 0, ->LB2
B.IOR       GMBase, zero, zero, []
B.IOR       ParamGPR0, ParamGPR1, ParamGPR2, []
B.IOT       mask=1111, last, ->DstTile<TSize>
BSTOP
```

## 参考

[TIMG2COL 块 ASL](https://github.com/PTO-ISA/pto-spec/blob/42067d0f2890b8c013a868bd1247ebc6437b3179/asl/block/execution/BSTART.TIMG2COL.asl)

[返回操作索引](../README.md)
