# Element-if 开发者使用手册

Element-if 允许开发者在 Tile 的逻辑元素上编写普通 C/C++ 表达式、`if/else`、
边界判断、gather 和受支持的 atomic 操作，再由 Linx Clang/LLVM 将循环整体降低为
Tile 指令和 execution mask。开发者不需要手工拼接 `TCMP + TADD/TSUB + TSEL`。

本页描述已经合入以下组件的接口：

- Linx LLVM `#118`：pragma、条件控制流、Tile SSA 和后端 lowering；
- Linx LLVM `#131`：fused masked element regions、严格的 Tile/element 内存合同和
  PE-domain 边界保持；
- TileOP API `#255`：`ElementTile`、typed element view、Tile/element 交替和 assembly；
- SuperNPUBench `#202`：实际 workload 的推荐写法和验证方法。

Element-if 不是任意 C++ 循环的自动并行器。编译器只接受能够证明满足当前 Tile
执行合同的循环、数据类型、控制流和内存访问；不支持的写法会在编译期 fail closed。

本页中的 `#pragma pto element for` 必须与包含 generic element-region 支持的
Linx LLVM 配套使用。当前 generic masked lowering 不是默认路径；需要显式传入
`-mllvm -linxv5-enable-generic-element=true`。没有该选项时，编译器应明确拒绝
需要 generic masked lowering 的 region，而不是退回旧的 gather/publication 识别器
或标量循环。`-O2` 是推荐配置；带 marked region 的 `-O0`/`optnone` 函数不支持
masked native lowering。

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

void add_if_positive(float *output, const float *left, const float *right) {
  ElementTile<float, 32> lhs_tile, rhs_tile, out_tile;
  global_tensor<float, RowMajor<32, 1>> left_memory(left);
  global_tensor<float, RowMajor<32, 1>> right_memory(right);
  global_tensor<float, RowMajor<32, 1>> output_memory(output);
  TLOAD(lhs_tile, left_memory);
  TLOAD(rhs_tile, right_memory);
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
  TSTORE(output_memory, out_tile);
}
```

`TPARTELEMENT` 不是普通数组 cast。它把 `ElementTile` 的逻辑 dtype、元素数量和
物理 layout 合同传给编译器。不要用裸 `tile_size` vector、`reinterpret_cast` 或
同字节数的普通 Tile 替代 typed view。

### 1.2 兼容 pragma：`#pragma linx elementwise`

这个拼写仍可在 compiler frontend 中标记普通循环，但不是旧 AST pattern
recognizer 的兼容开关，也不保证裸 `tile_size(128)` 参数可以直接生成对象。
当前 generic 路径仍要求支持的 iteration domain、typed view 和真正的 Tile
producer；普通 vector load/参数不等同于 `TLOAD`、Tile operation 或合法 Local
Tile ABI 产生的 carrier。不要把历史 scalar-style demo 当作当前可执行 profile。

TileOP 应用使用
`ElementTile + TPARTELEMENT + #pragma pto element for`，因为公共 API 会提供
稳定的类型、分片、valid region 和 assembly 合同。

下文以 Tile-reference 参数展示的循环片段，需要在实际 kernel 中接入合法的
Local Tile ABI，或内联到拥有 `TLOAD`/Tile producer 的调用点；仅通过 C++ 类型
检查并不等于已验证 target object。独立编译可先采用 1.1 节的完整 GM 入口。

两个 pragma 都必须**直接位于 `for` 循环之前**。以下写法会被拒绝：

```cpp
#pragma pto element                 // 错误：缺少 for
#pragma pto element for extra       // 错误：多余 token
#pragma pto element for
if (ready) { }                      // 错误：下一条语句不是 for
```

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
- 使用 `-O2` 编译；带 marked region 的 `-O0`/`optnone` 会明确拒绝；
- logical32 profile 通常使用 32 次迭代；F16/U16 logical64 profile 使用 64 次迭代。
  logical32 和 logical64 的 generic masked region 都需要上面的 generic-element
  lowering 选项；该选项不是 F16/U16 专用开关。

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
pointer mutation 或无法证明的 byte offset 会被拒绝。读取流可以共享地址，但普通
写入流必须证明跨 iteration 不重叠；单独证明 `a[i]` 与 `a[i+1]` 不别名并不足够。

当同一个 element region 先读取输入、再更新独立 histogram 或其他独立目的对象时，
编译器可以在 closed、local、非递归 helper 的所有直接调用上下文中证明 whole-object
separation，包括 local forwarding helper 以及 argument/global-root 组合。这个证明
只附着到已证明的访问对，不会给函数或指针隐式添加 `restrict`。exported、
address-taken、indirect、unknown、recursive 或 mixed-alias 的调用上下文仍然
fail closed。

### 5.2 Fused input + histogram

普通 GM 输入也可以在同一个 region 中直接读取并更新 histogram，无需先人为
拆成两个 region。以下示例要求 `keys[element]` 在 active element 上小于
histogram 容量，且调用者确实保证两个存储对象不重叠：

```cpp
void fused_histogram(const uint32_t *__restrict keys,
                     uint32_t *__restrict histogram, unsigned valid) {
#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (element < valid)
      __atomic_fetch_add(&histogram[keys[element]], 1u, __ATOMIC_RELAXED);
  }
}
```

`__restrict` 是调用者必须履行的真实合同，不是遇到编译错误时随意添加的修饰。
也可由编译器依据上一节的 closed-callgraph 规则证明对象独立；无法证明时仍拒绝。
即使没有使用 atomic 的返回值，也不得删除 atomic effect 或改变 active element
的执行次数。不同 element 的 atomic 可以命中同一 bin，不承诺它们之间的次序。

fused clear 的例外同样很窄：只有普通 unit-stride 写入、相同类型、相同零常量且
各自按完整存储宽度对齐时才允许可能重叠的两个写入流交换。该例外不覆盖读取、
非零值、不足的对齐或任何 atomic/ordinary 混合访问。

### 5.3 U32 histogram atomic

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

当前公开验收的 atomic profile 是 U32、32-element、System-scope、Monotonic/relaxed
`fetch_add`，降低为
带 execution mask 的 `MGATHER.ADD`。不要据此假设任意 dtype、memory order、atomic
operation 或 volatile pointer 都已支持；以 compiler 诊断和配套测试为准。

atomic/atomic 访问可以包含重复索引；atomic 与普通 memory effect 混合时仍需要
whole-object separation。volatile、非自然对齐、非 System scope、较强 ordering、
其他 atomic operation 和不支持的 dtype 必须被诊断，而不是通过放宽 alias 合同接受。

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
- fused direct load + atomic update（仅限已证明的 whole-object separation）；
- 相同类型、相同零值且按完整存储宽度对齐的 fused clear；
- 多层 `TASSEMBLY` storage hierarchy。

以下写法不应假设可用：

- 任意 trip count、非 canonical induction 或循环内修改 induction；
- 退出 marked loop 的 `break`、跨 region 的 `return`/`goto`、异常和递归；
- 未经允许的函数调用、虚函数、动态分配和复杂对象生命周期；
- 无法证明 ownership/alias 的 Tile 或 pointer；
- 不受支持的 volatile、atomic dtype/order/operation；
- 普通 indexed 写入之间无法证明的跨 iteration overlap；
- 在 `-O0` 下依赖 typed Tile spill/reload；
- 把普通 vector、同宽 Tile 或 raw assembly carrier 冒充 typed element view；
- 忽略 compiler/API/model 版本配套关系。

当前 generic CFG 支持外层 marked loop 的 `continue`。前端允许的嵌套控制流
不等于后端已支持任意嵌套循环；应分别检查 frontend boundary 与 target lowering。

## 9. 常见诊断

### `requires a proven ... iteration domain`

循环次数或 induction 形式无法证明。使用 `0` 起始、`++element` 和固定 profile
容量；确认需要 logical64 时启用了 generic-element lowering。

### `unpromoted or unproved memory access in arithmetic region`

循环内存在 compiler 无法证明的 load/store、复杂 pointer arithmetic 或别名。
把循环不变量移到 region 外，使用 `TPARTVIEW`/`logical_index` 表达公共逻辑映射，
并简化 pointer ownership。

### `predication: memory streams may overlap across elements`

检查跨 iteration 的完整访问流，而不仅是同一个 element 内的两个地址。普通读写
或 mixed atomic/ordinary 访问需要已证明的对象独立；不能随意添加 `__restrict`
掩盖真实重叠。fused clear 仅适用 5.2 节列出的零写入例外。

### 不支持的 region 内调用

loop body 中存在未被内联或不属于支持集合的调用。不要在 element region 内调用
普通 helper；必要时将纯表达式内联，或使用已经明确支持的 intrinsic/API。

### `unsupported logical view contract`

compiler 与 TileOP headers 不匹配，或 view dtype/shape/layout 不受支持。确认使用
包含 LLVM #131 和 TileOP #255 的配套工具链，并检查安装 resource directory 中的
`tileop-api` headers 是否来自同一 revision。

### `masked native lowering requires -linxv5-enable-generic-element=true`

logical32 和 logical64 的 generic region 都需要显式启用该选项。不要通过恢复
旧 recognizer、去掉 pragma 或接受标量 fallback 来绕过诊断。

### `masked native lowering is unsupported at -O0`

改用 `-O2`，并检查 helper 是否仍带 `optnone`。后续 typed Tile spill/reload 也
可能单独诊断；不应通过把 typed view 改成 raw pointer 或删除 metadata 来绕过。

## 10. 推荐验证清单

提交 element-if kernel 前至少检查：

1. host 侧 golden 覆盖普通值、边界值、tail 和输入/输出重叠；
2. target object 可生成，并检查存在预期的 masked Tile/atomic family；
3. runtime/model 比较完整输出内存和 guard，而不只比较返回码；
4. negative canary 确认不支持的 alias、call、volatile 或 atomic 会被拒绝；
5. 记录 compiler、TileOP API 和 model 的精确 commit；
6. logical32/logical64 workload 按需使用 generic-element lowering，但不污染无关
   fixture；同时验证没有把普通 vector 或 raw carrier 当作 typed element view。

## 11. PE-domain 和 Element-if

Element execution mask 与 function PE-domain 是两个不同层次的合同：前者描述
region 内哪些逻辑元素有效，后者描述哪些 PE 可以执行 Local Tile 操作。不要把
element predicate 当成 PE mask，也不要在应用中手工拼接 `.pto_pe_domain` assembly
来替代函数边界。

LLVM #131 提供 `__LINX_PTO_PE_DOMAIN__` feature macro 和
`__attribute__((annotate("pto.pe_domain", mask), noinline))` 函数注解。
**当前本仓头文件尚未提供 `PTO_PE_DOMAIN(mask)` 公共 wrapper**；只有 compiler
feature macro 并不代表可以直接调用该 wrapper，不要把未来接口当成已发布 API。
架构 bit order 中 `8`（`1000`）表示 PE0；合法 mask 为
`1, 2, 4, 8, 12, 14, 15`。调用者负责只让参与的 PE 进入函数；该注解不会
predicate 普通 C++ scalar statement 或 scalar memory access。

PE-domain 跨函数调用需要 closed direct call graph。默认 domain 的 helper 可以与
specialized domain helper 并存；unknown external、indirect、recursive nondefault
和 conflicting domain call 会被拒绝。Local Tile carrier 不得跨 domain boundary，
而 scalar/GM interface 可以跨越。native Tile 指令上的默认 `1111` mask 继承
function domain，显式非默认 mask 必须是该 domain 的子集；producer、assembly
writer、consumer 和 compiler-generated copy 都适用。generic profile 中的保守
`.reuse` 保留是编译器的生命周期决策，不要求应用恢复旧的手工 last-use 写法。
使用 PE-domain 的 element-if workload 时，应
同时验证 assembly round-trip 中的 `.pto_pe_domain`、Local Tile producer/consumer
和 element region 的 execution mask。

## 12. 版本与验证依据

本次刷新对应 LLVM #131，合入 `dev-llvm15_56` 的 commit 为
`084bbc3119365d45b8538a6705356a9173e521eb`。编译器合同详见该版本
`llvm/docs/PTOElementFor.rst`；对应回归包括
`element-ordinary-call-context-alias.ll`、`element-predication-commuting-stores.ll`、
`pto-element-generic-codegen.cpp` 和 `pe-domain-*` 测试。

API 具备某种 storage/view 类型不等于每个编译器 revision 都能执行该 dtype/shape
的 region。特别是 logical64/层级 storage 应分别做类型接口、target object 和
runtime 验证，不要把 host 类型测试或其他 revision 的模型结果当成本版本完整支持。

完整 workload 写法可参考 SuperNPUBench #202；API 类型和 assembly 细节另见
[TileOP 使用指南首页](README.md)与
[B.SUBVIEW / B.ASSEMBLE 使用指南](b-subview-b-assemble.md)。
