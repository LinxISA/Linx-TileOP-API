# TMOV

`TMOV` 将 Local Tile 的 payload 和 definedness 复制到目标 Tile；它不是
Local/Shared 之间的转换接口。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <is_local_tile_v Tile>
inline void TMOV(Tile &dst, const Tile &src);
```

### 支持的数据类型

支持FP64、FP32、TF32、HF32、FP16、BF16、HiF8、E4M3、E5M2、E3M2、E2M3、E8M0、E2M1X2、E1M2X2、HiF4X2、S4X2、U4X2、S64、S32、S16、S8、U64、U32、U16、U8类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src` | 输入 Tile 或源数据。 |



## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

`dst` 和 `src` 必须是同一 Tile 类型（包括 location、dtype、layout、物理
shape 和 capacity），并且 `src` 必须已初始化。普通 `TMOV` 只支持 Local-to-Local，
布局限于 `SLayout::NoneBox` 的 RowMajor、CUBE_M16 和 CUBE_M32；N8、boxed、
ColumnMajor 和 range wrapper 不能调用此同类型重载。CUBE identity copy 显式
发出对应的 `B.DATR`，并将保留的 pad union 编为 Zero（0）；它不要求填充
复制数据中的 padding。RowMajor 保持 NORM 默认；所有分支显式传递物理列数 LB2。
逻辑元素分片另有 borrowed-part 重载，支持匹配 dtype 的 logical32/logical64 part。

Shared Tile 必须使用 `TMOV_L2S_INSERT` / `TMOV_L2S_PUBLISH` 或
`TMOV_S2L_BROADCAST` / `TMOV_S2L_EXTRACT`，这些接口要求函数内或 inline SSA
生命周期，不能把 Shared handle 当作普通对象跨非内联函数传递。

### Local CUBE 到 Shared ND

`TMOV_CUBE_TO_SHARED_ND` 是 #399 增加的单向布局转换接口，将 Local
`CUBE_M16/M32` 的逻辑元素写入 RowMajor Shared ND；反向 Shared-to-CUBE
转换不在此接口范围内。

```cpp
template <int PEMask = 15, is_shared_tile_v SharedND,
          is_local_tile_v Cube>
PTO_SHARED_INLINE void TMOV_CUBE_TO_SHARED_ND(SharedND &dst,
                                               const Cube &src);

// 多 PE cooperative 写入使用 range::assemble* destination carrier。
TMOV_CUBE_TO_SHARED_ND(assemble_writer, cube_fragment);
```

未带 `B.ASSEMBLE` 的形式只允许单 bit `PEMask`，并要求源 capacity 等于完整
Shared parent capacity。多 PE 生产者必须用 `range::assemble`、
`range::assemble_middle`、`range::assemble_last` 描述明确的 128-byte range；
writer SizeCode 必须等于源 CUBE capacity，最终 `LAST` 才发布完整 parent。
每个 writer 的有效矩形会按其 `B.ASSEMBLE` 线性 offset 投影到父 RowMajor
几何，并合并为 pending valid rows/columns；`INIT`/`MIDDLE` 不替换此前已发布的
有效区，只有 `LAST` 才将完整 generation 的有效区和 payload 一起发布。因而
物理列为 4、有效列为 3 的整行 fragment 拼接后，父对象有效列仍为 3。

源、目标 backing dtype 必须相同，目标列数必须等于源 CUBE 的精确物理列数，
目标 RowMajor 几何必须完整覆盖其 SizeCode capacity，且 writer range 必须落在
parent 内。RCPE6M2 backing、CUBE_N8，以及在同一 Shared generation 中混用
NORM/M322ND/M162ND 都不合法。编码使用 Function 2 `BSTART.TMOV DTYPE_NONE`、
`B.DATR M322ND/M162ND, DTYPE_NONE, Zero`、Shared destination `B.IOS` 和 Local
source `B.IOT`。

参与同一 cooperative generation 的 PE 必须对有效区使用一致的 SPMD 元数据。
当前 C++ carrier 不对各 PE 私有的动态 valid shape 执行额外标量归约。

### Shared ND 到 Local CUBE

`TMOV_SHARED_ND_TO_CUBE` 对应 [PTO-spec PR #423](https://github.com/PTO-ISA/pto-spec/pull/423)
的 `PTO-TMOV-SHARED-ND-TO-CUBE-001`，适配基准提交为
`6160ebf740eee788d9be4e2fb86fba398445241d`。接口要求支持 TMOV Layout 21/22
的 Linx LLVM；旧编译器会在汇编阶段拒绝 `ND2M32/ND2M16`。
功能检测宏为 `PTO_TILEOP_API_HAS_SHARED_ND_TO_CUBE`；当前 API/spec release
版本号与已固定的 0.58.7 catalog 快照保持原有含义。

```cpp
// PEMask 默认 15。完整 parent 形式可由多个 PE 读取同一个完整源；
// 每个参与 PE 都分配一个独立 Local 目标，不按 PE identity 自动切片。
TMOV_SHARED_ND_TO_CUBE<PEMask>(dst_cube, shared_nd);

// 长度、立即数偏移、运行时 base 全部以 128B 为单位。
auto view = range::shared_nd_subview<LengthUnits, OffsetUnits>(shared_nd,
                                                              base_units);
TMOV_SHARED_ND_TO_CUBE<PEMask>(dst_cube, view);
// 省略 base_units 使用 zero register，可直接传递临时 view。
```

源必须是具有函数内或 inline SSA 生命周期的 compiler-managed RowMajor
`SharedTile`；range view 借用源对象，不能比源存活更久，也不能从临时 Shared
对象构造。`SharedTileSlot`、普通 `range::subview` 和 `B.ASSEMBLE` 目标不能
传给此接口。目标必须是 unboxed Local `CUBE_M32/M16`，且 backing dtype 与源
完全相同。RCPE6M2、N8 和 M16 b64 不支持；M32 b64 沿用双 CELL 映射。

完整 parent 的物理行数必须恰好为 M=32/16，容量等于目标容量。带范围的形式
必须选择恰好 M 个连续完整物理行，起点在物理行边界，范围落在 parent 内。
起始行不必为 M 的倍数。parent 的物理列数及行跨度保持不变，并等于目标
精确容量导出的物理列数。不能通过减小长度提取每行的一部分列。

接口根据源当前有效区计算 `ValidRow=min(M,max(0,parent.ValidRow-origin_row))`
及 `ValidCol=parent.ValidCol`；两者必须非零。静态目标有效区必须匹配，动态
目标在转换后更新对应元数据。可静态确定的布局、类型、容量和行对齐错误在
编译期拒绝；动态范围越界、未按行对齐、空有效区及目标有效区不匹配在提交
转换前 trap，检查在 `NDEBUG` 下仍有效。源对象的运行时有效区元数据可以被
生产者更新，接口不会把类型声明的初始有效区当作当前发布的有效区。

编码为 Function 2 `BSTART.TMOV DTYPE_NONE`，`B.DATR ND2M32/ND2M16,
DTYPE_NONE, Zero`；`B.DIM LB0/LB1` 显式提供有效列/行，省略 LB2 使用有效值
1 的精确容量推导规则。Shared `B.IOS` 与 Local destination `B.IOT` 使用同一
PE mask，`B.SUBVIEW` 紧跟 Shared binder。`PEMask=0` 不读取源 handle、有效区
或检查范围；非参与 PE 也不访问源或更新目标。

源必须完整发布；硬件按规范等待未就绪的源并处理 generation/lifetime。
转换保留每个有效逻辑元素的原始位与 backing dtype，不转置、不作数值转换，
也不访问 GM。输出有效元素定义明确；padding 写原始零但仍在语义有效区外。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 逐元素操作通常仅对输入和输出共同的有效区域定义结果；未明确规定的 padding 不应读取或依赖。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- `B.IOR` 是必需描述符；未使用的选择器和字段必须编码为零。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TMOV` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.TLSU TMOV, DataType
B.DIM       LB0
B.DIM       (LB1/LB2 for 2D)
B.IOT
BSTOP
```

## 使用示例

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using TileT = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;
TileT src;
TileT dst;
TMOV(dst, src);
```

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;
using Cube = VecTileM32<uint8_t, 32, 128>;
using ND = SharedTile<Tile<Location::Vec, uint8_t, 32, 128>>;

void roundtrip(Cube &dst, Cube &src) {
  ND shared;
  TMOV_CUBE_TO_SHARED_ND<8>(shared, src);
  // 显式 mask=1000 选择 PE0；Shared handle 保持在本函数内。
  TMOV_SHARED_ND_TO_CUBE<8>(dst, shared);
}
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。
