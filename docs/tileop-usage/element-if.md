# Element-if 开发者使用手册

Element-if 允许开发者在 Tile 的逻辑元素上编写普通 C/C++ 表达式、`if/else`、
边界判断、gather 和受支持的 atomic 操作，再由 Linx Clang/LLVM 将循环整体降低为
Tile 指令和 execution mask。开发者不需要手工拼接 `TCMP + TADD/TSUB + TSEL`。

本页描述已经合入以下组件的接口：

- Linx LLVM `#118`：pragma、条件控制流、Tile SSA 和后端 lowering；
- TileOP API `#255`：`ElementTile`、typed element view、Tile/element 交替和 assembly；
- SuperNPUBench `#202`：实际 workload 的推荐写法和验证方法。

Element-if 不是任意 C++ 循环的自动并行器。编译器只接受能够证明满足当前 Tile
执行合同的循环、数据类型、控制流和内存访问；不支持的写法会在编译期 fail closed。

## 1. 两种入口

### 1.1 TileOP API 入口：`#pragma pto element for`

这是 TileOP kernel 推荐使用的入口。典型流程是：

1. 用 `ElementTile<T, N>` 表示逻辑元素 Tile；
2. 必要时用 `TPARTVIEW` 把 parent 分成可处理的 part；
3. 用 `TPARTELEMENT(tile)` 取得带 dtype、shape 和 layout metadata 的 typed view；
4. 在紧随其后的 `for` 循环前写 `#pragma pto element for`；
5. 循环后继续使用 TileOP API，例如 `TMOV`、`TASSEMBLY`、`TSTORE`。

```cpp
#include <common/pto_tileop.hpp>

using namespace pto;

void add_if_positive(ElementTile<float, 32> &out_tile,
                     ElementTile<float, 32> &lhs_tile,
                     ElementTile<float, 32> &rhs_tile) {
  auto &out = TPARTELEMENT(out_tile);
  auto &lhs = TPARTELEMENT(lhs_tile);
  auto &rhs = TPARTELEMENT(rhs_tile);

#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (lhs[element] > 0.0f)
      out[element] = lhs[element] + rhs[element];
    else
      out[element] = lhs[element] - rhs[element];
  }
}
```

`TPARTELEMENT` 不是普通数组 cast。它把 `ElementTile` 的逻辑 dtype、元素数量和
物理 layout 合同传给编译器。不要用裸 `tile_size` vector、`reinterpret_cast` 或
同字节数的普通 Tile 替代 typed view。

### 1.2 Scalar-style 入口：`#pragma linx elementwise`

不直接使用 TileOP wrapper 的编译器测试或底层 kernel，也可以对一个
`tile_size(N)` 类型编写 scalar-style 循环：

```cpp
using tile = float tile_size(128);

void scalar_style(tile &out, const tile &lhs, const tile &rhs) {
#pragma linx elementwise
  for (unsigned element = 0; element < 128; ++element) {
    if (lhs[element] > 0.0f)
      out[element] = lhs[element] + rhs[element];
    else
      out[element] = lhs[element] - rhs[element];
  }
}
```

这个入口主要属于 compiler frontend。TileOP 应用优先使用
`ElementTile + TPARTELEMENT + #pragma pto element for`，因为公共 API 会提供
稳定的类型、分片、valid region 和 assembly 合同。

两个 pragma 都必须**直接位于 `for` 循环之前**。以下写法会被拒绝：

```cpp
#pragma pto element                 // 错误：缺少 for
#pragma pto element for extra       // 错误：多余 token
#pragma pto element for
if (ready) { }                      // 错误：下一条语句不是 for
```

### 1.3 与显式 PredicateCell API 的区别

Element-if 是编译器驱动的路径：开发者在 pragma 循环内写普通条件表达式，
由编译器构造、组合 predicate 并选择受支持的 masked 指令。通常不需要手工
创建或传递 predicate carrier。

开发分支另有独立的、尚未发布的
[PredicateCell adapters](predicate-adapters.md)，供显式 TileOP 数据流使用：

```cpp
auto active = predicate::compare_scalar<CmpMode::GE>(values, threshold);
predicate::scatter(destination, byte_offsets, values, active);
predicate::atomic_add(destination, byte_offsets, values, active);
```

这条路径不要求 `#pragma pto element for`、`#pragma linx elementwise` 或
`TPARTELEMENT`，也不是供 pragma loop body 调用的入口。目前没有将
`Predicate<Basis>` 导入/导出 element-if 隐式 mask 的桥接接口，不能假设
把这些调用放进循环就能编译。

两条路径使用相同架构中的 predicate/ExecutionMask 概念，但公开支持边界
不同。显式 adapters 首版只覆盖 Local VEC CUBE_M32、静态有效区域和
FP32/S32/U32，尚无 predicate 逻辑组合；本页 element-if 已支持的组合条件
不能据此推广到 `Predicate<Basis>`。显式 `atomic_add` 使用 MSCATTER_ADD
ExecutionMask，本页 histogram 示例的已验收 profile 则使用 MGATHER.ADD，
不能互相替代其 dtype、memory-order、返回值或验证结论。

写普通逐元素条件时优先使用本页的 element-if；已有显式 Tile comparison、
需要把结果直接交给 scatter/atomic consumer 时，再选择实验性 adapters，
并满足其配套 compiler builtin 和独立类型约束。

## 2. 基本语法规则

推荐使用编译器容易证明的 canonical loop：

```cpp
#pragma pto element for
for (unsigned element = 0; element < 32; ++element) {
  // element body
}
```

开发者应遵循以下规则：

- induction variable 从 `0` 开始，步长为 `1`；
- 循环上界必须能在编译期或优化阶段证明；
- loop body 内使用当前 induction variable 访问 typed element view；
- 不要修改 induction variable，不要从循环中 `goto`、抛异常或执行不可证明的调用；
- 优先把循环外不变的值、pointer 和 part index 在 pragma 前计算好；
- 使用 `-O2` 编译。typed Tile 在 `-O0` 下可能需要不受支持的 spill/reload；
- logical32 profile 通常使用 32 次迭代；F16/U16 logical64 profile 使用 64 次迭代，
  并需要 generic-element lowering。

## 3. 条件控制流

### 3.1 单分支写入

```cpp
void clear_selected(ElementTile<uint32_t, 32> &values_tile,
                    uint32_t selected) {
  auto &values = TPARTELEMENT(values_tile);

#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (values[element] == selected)
      values[element] = 0;
  }
}
```

未进入分支的 lane 保留原值。编译器会为条件构造 execution mask，并在受 mask
保护的 Tile 操作上维持 merge 语义。

### 3.2 `if/else`

```cpp
void absolute_value(ElementTile<int32_t, 32> &out_tile,
                    ElementTile<int32_t, 32> &input_tile) {
  auto &out = TPARTELEMENT(out_tile);
  auto &input = TPARTELEMENT(input_tile);

#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (input[element] < 0)
      out[element] = -input[element];
    else
      out[element] = input[element];
  }
}
```

编译器通常把两个分支降低为互补 mask，再用 Tile select 或等价的 masked Tile
数据流合并结果。开发者不应依赖具体选择了 `TSEL` 还是其他等价指令。

### 3.3 组合条件和 tail

```cpp
void clamp_tail(ElementTile<uint32_t, 32> &out_tile,
                ElementTile<uint32_t, 32> &input_tile,
                unsigned valid, uint32_t limit) {
  auto &out = TPARTELEMENT(out_tile);
  auto &input = TPARTELEMENT(input_tile);

#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (element < valid && input[element] > limit)
      out[element] = limit;
    else if (element < valid)
      out[element] = input[element];
    else
      out[element] = 0;
  }
}
```

`element < valid` 是常见的 runtime tail predicate。组合条件应只依赖可证明的
loop induction、typed view 元素、循环外 scalar 和支持的比较/布尔运算。

## 4. Tile 与 element 交替

Element-if 的核心用法不是把整个 kernel 写成一个大循环，而是在 Tile 数据流之间
插入小型 element region：

```text
TLOAD / Tile operation
  -> TPARTVIEW / TMOV
  -> TPARTELEMENT
  -> #pragma pto element for
  -> TMOV / TASSEMBLY
  -> reduction / Tile operation / TSTORE
```

下面的例子处理一个 128-element U32 parent。每个 part 独立进入 element region，
最后重新组装为 parent：

```cpp
void transform_128(uint32_t *output, const uint32_t *input,
                   unsigned valid) {
  using Parent = ElementTile<uint32_t, 128>;
  using Leaf = ElementTile<uint32_t, 32>;

  Parent source;
  TLOAD(source, input, valid);
  auto source_parts = TPARTVIEW<32>(source, valid);
  TileArray<Leaf, 1, 4> destinations;

  for (unsigned part = 0; part < source_parts.size(); ++part) {
    Leaf leaf;
    TMOV(leaf, source_parts.part(part));
    auto &elements = TPARTELEMENT(leaf);
    const unsigned part_valid = source_parts.valid_size(part);

#pragma pto element for
    for (unsigned element = 0; element < 32; ++element) {
      if (element < part_valid)
        elements[element] = elements[element] * 3u + 1u;
    }

    TMOV(destinations[0][part], leaf);
  }

  Parent result = TASSEMBLY<Parent>(std::move(destinations));
  TSTORE(output, result, valid);
}
```

重要生命周期规则：

- `TPARTVIEW` 借用 parent，parent 必须保持存活；
- `TPARTELEMENT` 借用 Tile carrier，不得返回或保存到 carrier 生命周期之外；
- `TileArray` 不可复制，所有 writer 完成后用 `std::move` 交给 `TASSEMBLY`；
- assembly 的 slot 必须完整覆盖，不能把未关闭的中间 assembly 当普通 Tile 使用；
- element region 前后的 Tile 必须保持明确 ownership，避免别名破坏 snapshot 语义。

## 5. Gather 和 atomic 示例

### 5.1 Indexed gather

```cpp
void gather(ElementTile<uint32_t, 32> &out_tile,
            ElementTile<uint32_t, 32> &index_tile,
            const uint32_t *__restrict source,
            unsigned valid) {
  auto &out = TPARTELEMENT(out_tile);
  auto &indices = TPARTELEMENT(index_tile);

#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (element < valid)
      out[element] = source[indices[element]];
    else
      out[element] = 0;
  }
}
```

当前 compiler 会为受支持的 indexed access 构造 Tile 地址并降低为 masked gather。
pointer 应尽量使用明确的 element type 和 `__restrict` ownership；复杂别名、循环内
pointer mutation 或无法证明的 byte offset 会被拒绝。

### 5.2 U32 histogram atomic

```cpp
void histogram(ElementTile<uint32_t, 32> &old_values_tile,
               ElementTile<uint32_t, 32> &keys_tile, uint32_t *histogram,
               unsigned valid) {
  auto &old_values = TPARTELEMENT(old_values_tile);
  auto &keys = TPARTELEMENT(keys_tile);

#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (element < valid)
      old_values[element] =
          __atomic_fetch_add(&histogram[keys[element]], 1u, __ATOMIC_RELAXED);
    else
      old_values[element] = 0;
  }
}
```

当前公开验收的 atomic profile 是 U32、32-element、relaxed `fetch_add`，降低为
带 execution mask 的 `MGATHER.ADD`。不要据此假设任意 dtype、memory order、atomic
operation 或 volatile pointer 都已支持；以 compiler 诊断和配套测试为准。

## 6. F16/U16 logical64

F16/U16 的 dense element part 是 `ElementTile<T, 64>`，物理布局为 32x2。它的
逻辑索引映射由 typed view 和 `TPARTVIEW<64>` 管理，不应由应用自行把它当成普通
线性 vector 重新解释。

```cpp
void relu_f16(ElementTile<__half, 64> &tile) {
  auto &values = TPARTELEMENT(tile);

#pragma pto element for
  for (unsigned element = 0; element < 64; ++element) {
    if (values[element] < static_cast<__half>(0))
      values[element] = static_cast<__half>(0);
  }
}
```

logical64 pragma lowering 使用 generic-element pipeline。使用本仓 object harness 时，
`Element*` fixture 会自动加：

```text
-mllvm -linxv5-enable-generic-element=true
```

独立编译同类 kernel 时也应加该选项。不要把这个开关无条件加到所有旧 TileOP
fixture；它只应作用于需要 generic element lowering 的 workload。

## 7. 编译方式

示例命令：

```bash
clang++ --target=linx64v5-unknown-linux-musl \
  -mlxbc -fenable-matrix -O2 -std=c++20 \
  -mllvm -enable-all-vector-as-tilereg=true \
  -mllvm -linxv5-enable-generic-element=true \
  --sysroot="$LINX_SYSROOT" \
  -I/path/to/Linx-TileOP-API/include \
  -D__linx -DENABLE_TENSOR_INSTR \
  -c kernel.cpp -o kernel.o
```

检查 lowering：

```bash
llvm-objdump -d --no-show-raw-insn \
  --disassembler-options=no-tile-macros kernel.o
```

根据源码，常见输出包括 `TCI`、`TCMP/TCMPS`、masked arithmetic、`TSEL`、
`TLEA`、`MGATHER` 或 `MGATHER.ADD`。具体指令序列属于 compiler implementation，
测试功能时应优先验证最终内存结果和 guard，而不是只匹配单条汇编。

## 8. 当前支持边界

当前已经重点覆盖：

- typed U32/S32/F32 logical32 element view；
- F16/U16 logical64 view 和对应存储/assembly；
- 普通整数和浮点表达式、局部 scalar、比较和受限 cast；
- `if`、`if/else`、组合 predicate 和 runtime tail；
- Tile load/view/element/assembly/store 交替；
- 受支持的 indexed gather；
- U32 relaxed atomic add；
- 多层 `TASSEMBLY` storage hierarchy。

以下写法不应假设可用：

- 任意 trip count、非 canonical induction 或循环内修改 induction；
- `break`、`continue`、`goto`、异常和递归；
- 未经允许的函数调用、虚函数、动态分配和复杂对象生命周期；
- 无法证明 ownership/alias 的 Tile 或 pointer；
- 不受支持的 volatile、atomic dtype/order/operation；
- 在 `-O0` 下依赖 typed Tile spill/reload；
- 把普通 vector、同宽 Tile 或 raw assembly carrier 冒充 typed element view；
- 忽略 compiler/API/model 版本配套关系。

## 9. 常见诊断

### `requires a proven ... iteration domain`

循环次数或 induction 形式无法证明。使用 `0` 起始、`++element` 和固定 profile
容量；确认需要 logical64 时启用了 generic-element lowering。

### `unpromoted or unproved memory access in arithmetic region`

循环内存在 compiler 无法证明的 load/store、复杂 pointer arithmetic 或别名。
把循环不变量移到 region 外，使用 `TPARTVIEW`/`logical_index` 表达公共逻辑映射，
并简化 pointer ownership。

### `calls are not in the ... arithmetic region`

loop body 中存在未被内联或不属于支持集合的调用。不要在 element region 内调用
普通 helper；必要时将纯表达式内联，或使用已经明确支持的 intrinsic/API。

### `unsupported logical view contract`

compiler 与 TileOP headers 不匹配，或 view dtype/shape/layout 不受支持。确认使用
包含 LLVM #118 和 TileOP #255 的配套工具链，并检查安装 resource directory 中的
`tileop-api` headers 是否来自同一 revision。

### `typed Tile spill/reload is unsupported at -O0`

改用 `-O2`。这是 lowering 前 Tile SSA promotion 的要求，不应通过把 typed view
改成 raw pointer 或删除 metadata 来绕过。

## 10. 推荐验证清单

提交 element-if kernel 前至少检查：

1. host 侧 golden 覆盖普通值、边界值、tail 和输入/输出重叠；
2. target object 可生成，并检查存在预期的 masked Tile/atomic family；
3. runtime/model 比较完整输出内存和 guard，而不只比较返回码；
4. negative canary 确认不支持的 alias、call、volatile 或 atomic 会被拒绝；
5. 记录 compiler、TileOP API 和 model 的精确 commit；
6. logical64 workload 使用 generic-element lowering，但不污染无关 fixture。

完整 workload 写法可参考 SuperNPUBench #202；API 类型和 assembly 细节另见
[TileOP 使用指南首页](README.md)与
[B.SUBVIEW / B.ASSEMBLE 使用指南](b-subview-b-assemble.md)。
