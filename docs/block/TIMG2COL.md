# TIMG2COL

`TIMG2COL` 从 GM feature-map 按打包参数提取窗口，并直接物化为 Local CUBE。

## C++ 接口

当前 API 中可用的调用形式：

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
template <LayoutCvtEnum SourceOrder = NORM, is_tile_data_v shp,
          is_global_data_v gm_shape>
void TIMG2COL_SPART(SharedTile<shp> &dst, gm_shape &src,
                    TIMG2COLParams params, unsigned PEMask);
```

### 支持的数据类型

支持FP32、FP16、BF16、S32、S16、S8、U32、U16、U8类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | GM feature-map 基址。 |
| `param0` | 输入尺寸、Cin 和 kernel 尺寸的打包参数字。 |
| `param1` | padding、dilation、stride 和扩展控制的打包参数字。 |
| `param2` | 输出 row/column 起始位置的打包参数字。 |



## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

输入与输出的 Tile location、layout、dtype、物理 shape 和 valid region 必须满足该操作的 逐项规则；除非本页明确允许，不应假定可原地执行或允许 alias。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 输出 valid region 由布局、偏移或拼接描述符计算；物理 padding 不等于有效数据。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

参数字必须显式提供；各字段的位分配见 PTO 的 TIMG2COL parameter contract。

### 编码字段和省略值

- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 省略 `B.IOR` 时使用本操作规定的寄存器或控制默认值；显式编码为零表示实际的零值，不等同于省略该描述符。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TIMG2COL` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TIMG2COL DataType
B.DATR       ND2M32, DTYPE_NONE, Zero
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1
B.DIM       rTotalCol, 0, ->LB2
B.IOR       GMBase, zero, zero, []
B.IOR       ParamGPR0, ParamGPR1, ParamGPR2, []
B.IOT       mask=1111, last, ->DstTile<TSize>
BSTOP
```

## SharedND 输出与 NCHW（issue #194）

直接 Local 输出由 schema 固定为 `ND2M16`/`ND2M32` 布局，即 **ND（NHWC）源寻址**
（`spatial*Cin + channel`）。NCHW 源需要 **DN 寻址**（`channel*H*W + spatial`），
规范将该寻址唯一地路由到 SharedND 输出：

| SourceOrder | B.DATR 布局 | GM 寻址 | 适用源布局 |
| --- | --- | --- | --- |
| `NORM` | `NORM` | `spatial*Cin + channel` | NHWC |
| `DN2ND` | `DN2ND` | `channel*H*W + spatial` | NCHW |

- 四-PE 协作形式（`mask=1111` 的 `B.IOS`）要求调用方按 ASL 世代协议补发
  各 PE 的 `B.ASSEMBLE`（INIT/MIDDLE/LAST 阶段），当前封装不代发——需要
  分段写入时请改用单-issuer 变体或自行编码。
- 单-issuer 变体 `TIMG2COL_SPART` 接受恰含一个 PE 位的掩码，一条
  `B.IOS` 直接发布完整 parent，无 `B.ASSEMBLE`。

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using GM = global_tensor<float, RowMajor<8, 256>>;
using TileT = CubeTileM32<float, 32, 256>;
float src_data[8 * 256] = {};
GM src_global(src_data);
TileT dst;
TIMG2COL(dst, src_global, 0x0008000800040008ULL, 0, 0);
```

NCHW 源经 SharedND 物化（DN2ND 寻址）：

```cpp
using namespace pto;
using SharedND = SharedTile<Tile<Location::Vec, __half, 64, 64,
                                     BLayout::RowMajor, 64, 64>>;
using FeatureGM = global_tensor<__half, RowMajor<64, 64>>;
__half buf[64 * 64] = {};
FeatureGM feature(buf);
SharedND shared;
TIMG2COL_SPART<DN2ND>(shared, feature,
                      TIMG2COLParams{0x0008000800040008ULL, 0, 0}, 1);
```

涉及不同输入尺寸、padding、stride、dilation 或起始位置时，请按
TIMG2COL parameter contract 重新打包三个参数字，而不是使用旧的二维位置参数。