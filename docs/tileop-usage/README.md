# TileOP C++ API 使用指南

本目录是 LinxISA TileOP C++ wrapper 的开发者入口。推荐先运行一个最小 kernel，
再按操作类别查阅 API 页面；页面中的签名以本仓库 `include/jcore/template_asm.hpp`
为准；各操作页分别列出该操作的约束、默认值、边界行为和 bundle 参考。

## 版本与兼容性

本目录包含两个有意并存的版本视角：

- 一般操作页和 range modifier 页面按 **v0.58.4.1** 的规范内容编写；
  [TPERMUTE](layout-and-rearrangement/layout/TPERMUTE.md)、
  [TSHUF](layout-and-rearrangement/layout/TSHUF.md)、
  [TPACK](layout-and-rearrangement/layout/TPACK.md)、
  [TUNPACK](layout-and-rearrangement/layout/TUNPACK.md) 和
  [TGPR2T](layout-and-rearrangement/layout/TGPR2T.md) 按 **v0.58.5** 编写。使用这些 API
  时应配套支持对应规则的 Linx 编译器。
- [0.58.3 迁移说明](migration/pto-0583-migration.md)与
  [engine catalog](generated/engines.md)记录的是 **0.58.3** 的历史迁移/引擎投影，
  不应被当作 0.58.4.1 新增 descriptor 的编译器兼容性声明。

不要混用不同版本的 headers、compiler 和规范内容。出现汇编编码、SizeCode 或
bundle 字段不匹配时，先确认这三者的版本是否一致。

## 快速开始

下面的例子展示普通 Local VEC Tile 的最小数据流：从 Global Memory 载入，执行
`TADD`，再写回。类型、shape 和布局必须满足每个操作页的约束。

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

using GM = global_tensor<float, RowMajor<32, 32>>;
using TileT = Tile<Location::Vec, float, 32, 32, BLayout::RowMajor>;

void add_kernel(float *out, float *lhs, float *rhs) {
  GM out_gm(out), lhs_gm(lhs), rhs_gm(rhs);
  TileT a, b, result;
  TLOAD(a, lhs_gm);
  TLOAD(b, rhs_gm);
  TADD(result, a, b);
  TSTORE(out_gm, result);
}
```

使用与 SDK 配套的 Linx clang++ 编译；target、include 路径和 feature flags 应以
当前工具链发布说明为准。range modifier 或生成汇编排查可使用：

```bash
clang++ --target=linx64v5-unknown-linux-musl -mlxbc -fenable-matrix \
  -O2 -std=c++20 -Iinclude -D__linx -S kernel.cpp -o kernel.s
```

## 阅读路径

- 先了解 [Tile 约束、location、shape 和 bundle](concepts/README.md)。
- 需要从 GM 传输数据时，阅读 [TLSU load/store/move](tlsu/load-store-move/TLOAD.md)。
- 常规逐元素算子从 [TADD](elementwise-tile-tile/arithmetic/TADD.md) 开始；按目录选择
  算术、逻辑、转换、归约、布局和不规则操作。
- PTO ISA v0.58.5 的 CUBE layout 重排操作从
  [TPERMUTE](layout-and-rearrangement/layout/TPERMUTE.md) 开始；pack/unpack、shuffle
  和 GPR predicate plane 转换页面位于同一目录。
- 矩阵/向量计算使用 [CUBE TMATMUL](cube/matrix-matrix/TMATMUL.md) 或相应 GEMV 页面。
- 需要启用矩阵后处理属性时，先阅读 [`fixp::Options` 指南](options.md)。
- 需要绑定 Tile range 或分区/组装时，阅读
  [B.SUBVIEW / B.ASSEMBLE developer guide](range-modifiers-developer-guide.md)。
- 按执行引擎或 selector 查找操作时，使用 [engine catalog](generated/engines.md)。

## 每个操作页如何使用

每页依次给出 C++ 重载、支持数据类型、参数角色、特定约束、默认值、边界行为、
bundle 参考和使用示例。实现 kernel 时优先检查“参数说明”和“约束”；bundle 内容
主要用于核对生成汇编，不应替代 C++ API。

特别注意：GM row stride 在 TLSU contract 中以**字节**计；`valid region` 不会改变
Tile 的物理容量；Shared Tile、CUBE layout、PE mask 和 range lifecycle 都有额外约束。

## 排障

- 编译期 Tile 类型不匹配：核对 location、dtype、layout、shape、valid region 和操作数顺序。
- TLSU 地址/数据异常：核对 stride 单位是字节而非元素，并检查 GM layout。
- CUBE 计算失败：核对 A/B/D 的 CUBE layout、累加器类型和 `TLOAD/TSTORE` 的转换路径。
- range modifier 失败：检查 source/destination 角色、`RegSrc`、offset，以及
  `INIT → MIDDLE* → LAST` 的 assembly 生命周期。

仍无法定位时，生成 `.s` 并检查 `BSTART`、binder 与相关 modifier 的顺序；详细命令见
[range modifier 指南](range-modifiers-developer-guide.md#生成代码检查)。
