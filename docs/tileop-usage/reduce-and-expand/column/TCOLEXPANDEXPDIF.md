# TCOLEXPANDEXPDIF

`TCOLEXPANDEXPDIF` 计算每个完整形状元素与同列广播值之差的对应类型指数。

## C++ 接口

当前 API 中可用的调用形式：

```cpp
template <
    is_tile_data_v tile_shape_out,
    is_tile_data_v tile_shape_in0,
    is_tile_data_v tile_shape_in1>
void TCOLEXPANDEXPDIF(tile_shape_out &dst, tile_shape_in0 &src0, tile_shape_in1 &src1);
```

### 支持的数据类型

支持FP32、FP16、BF16类型。



### 参数说明

| 参数 | 说明 |
| --- | --- |
| `dst` | 输出 Tile；成功调用后写入操作结果。 |
| `src0` | 第一个输入 Tile。 |
| `src1` | 第二个输入 Tile。 |



## 使用要求

- Tile 类型必须满足接口模板约束；
- 数据类型、形状、有效区域、布局、容量和存储位置必须满足该操作要求；
- 输入 Tile 必须已初始化，输出 Tile 必须具有足够容量；
- 参数顺序必须与接口声明一致，不要添加接口未声明的操作数。

## 约束

该操作沿列方向归约或广播；输入、输出及广播 Tile 的有效区域和 shape 必须满足 该方向的对应关系，且索引输出的整数类型必须符合本页约束。

    操作数角色、数据类型组合、容量、PE mask 和 alias 必须符合上方约束；只能使用所选重载声明的操作数形式。

### 有效区域与 padding

| 项目 | 规则 |
| --- | --- |
| 有效元素 | 归约轴会收缩，广播轴会扩展；输出 valid region 由该操作的轴规则决定。 |
| 物理容量 / SizeCode | 只决定容量，不重新定义逻辑 shape。 |
| 输出 padding | 除非本操作明确规定填充值或传播规则，否则视为不可依赖。 |



## 默认值

 此页面列出的 C++ 形参没有默认实参；不要把省略某个操作数与传入零值视为等价。

### 编码字段和省略值

- 省略 `B.DATR` 时，padding 值使用 `Null`。
- `LB0` 给出 `ValidCol`，必须存在且非零；省略 `LB1` 时 `ValidRow=1`，省略 `LB2` 时物理列数等于 `ValidCol`。显式给出的维度不能为零。
- 显式 padding 编码 `00`、`01`、`10`、`11` 分别选择 `Zero`、`Max`、`Min`、`Null`。

`fixp::Options` 内部字段的默认值和合法组合见 [Options 指南](../../options.md)。

## 异常和边界行为

    类型不匹配、非法形状或布局、未初始化的输入、输出容量不足、非法 PE mask、错误的 Tile 位置或不合法的属性组合，会在编译期或执行前检查阶段被拒绝。`PE_MASK=0000` 时操作不产生状态或内存影响；非法调用不会发布部分输出或部分副作用。padding、alias、NaN/无穷值及 fault 行为以本页已经列出的约束和边界说明为准，未明确声明的状态不可依赖。

## 结果说明

    成功调用后，`TCOLEXPANDEXPDIF` 更新输出 Tile 的有效区域；输入 Tile 通常保持不变，输出 padding 和未明确声明的副作用不可依赖。若操作的约束或参数说明另有规定，以对应说明为准。

## Bundle 组成

开发者通常直接调用 C++ 接口，无需手工编写 bundle。下面保留对应汇编结构供核对：

```asm
BSTART.SFU TCOLEXPANDEXPDIF, DataType
B.DATR      DataType, PadValue (optional)
B.DIM       rValidCol, 0, ->LB0
B.DIM       rValidRow, 0, ->LB1  ; (optional)
B.DIM       rCol, 0, ->LB2  ; (optional)
B.IOT       SrcTile, BroadcastTile, mask=PE_MASK, last, ->DstTile<TSize>
BSTOP
```

## 使用示例

```cpp
#include <jcore/template_asm.hpp>

using namespace pto;
using MatrixGM = global_tensor<float, RowMajor<32, 32>>;
using VectorGM = global_tensor<float, RowMajor<1, 32>>;
using MatrixTile = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor, 32, 32>;
using ColTile = Tile<Location::Vec, float, 1, 32, BLayout::RowMajor, 1, 32>;
float matrix_data[32 * 32] = {}, col_data[32] = {}, dst_data[32 * 32] = {};
MatrixGM matrix_global(matrix_data), dst_global(dst_data); VectorGM col_global(col_data);
MatrixTile src, dst; ColTile col;
TLOAD(src, matrix_global); TLOAD(col, col_global);
TCOLEXPANDEXPDIF(dst, src, col);
TSTORE(dst_global, dst);
```

涉及标量、索引、scale 或 bias 的操作，请按上方实际重载替换示例参数。