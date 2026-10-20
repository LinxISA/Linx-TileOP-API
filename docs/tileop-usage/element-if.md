# Element-if 开发者使用手册（冻结 TopK 组合）

Element-if 允许在 Tile 的逻辑元素上编写受限的 C/C++ 表达式、`if/else`、边界判断、
indexed gather 和支持的 atomic 操作，再由配套 Linx Clang/LLVM 降低为 Tile 指令与
execution mask。它不是任意 C++ 循环的自动并行器；不能证明满足当前 Tile、内存和
控制流合同的写法必须 fail closed。

本页对应 Model #925 的历史冻结组合，不是当前各仓库分支的最新能力：

| 组件 | 固定版本 |
| --- | --- |
| SuperNPUBench | `codex/pto059-element-if-integration` @ `9076bd082544e273cdb93f39675df42510d8c071` |
| LLVM | `codex/pto-element-region-compiler` @ `482b4207e88adf312fd1b1a8a567b83f5a4e207b` |
| Linx-TileOP-API | `b223de6a80c1d9b7c2f1ac3a1ff4517b1951d650` |
| SuperScalarModel | `codex/pto-element-generic-timing` @ `8e8d8ac7b1becc8937e77989c2762139f946b8af`，gfrun/gfsim 共用 |

以上组合曾验证 13 个 element/atomic/TopK 用例，包括 Original777 TopK 的 9 个 memory
segment（777 个输入、K=37）；该结果是历史记录，本页没有重新声称本轮 gfrun/gfsim
通过。依据为 [SuperScalarModel PR #925](https://github.com/LinxISA/SuperScalarModel/pull/925)。

API 固定版本仍保留在 `codex/elementwise-public-api`。本页是该分支的文档跟进，
不是代码升级；复现原结果仍应 checkout 上表 commit，而非包含文档更新的分支 HEAD。
SuperNPUBench #202 后来已从 main 回退，不能用 benchmark main 代替冻结分支代码。

## 1. API 能力

冻结 API 只提供 `ElementTile<T, Elements>` 的 U32、S32、F32 32/128-element
profile：

- `TPARTVIEW<32>(parent, valid_elements)` 借用 128-element parent，并提供 part 与 valid size；
- `TPARTELEMENT(part)` 只接受 32-element part，携带 `dtype`、shape 和 layout metadata；
- `TLOAD(parent, pointer, valid)` 与 `TSTORE(output, part, parts, index)`
  要求 pointer dtype 与 Tile 完全匹配；
- `TSTORE(output, part, parts, index)` 写回完整 part，`valid_size` 仅供 region 内 predicate；
- 普通 vector、同宽 raw Tile 或 `reinterpret_cast` 不能冒充 typed element view。

F16/U16 logical64、native i64 logical32、NORM view、PE-domain 注解和更宽 shape 不属于
这个冻结 API+compiler+model 组合的公开能力。后续分支可能增加这些实现，但不能把
它们回填到本冻结 TopK 结果中。

F32 view 在 API 类型系统中存在，不表示冻结 LLVM `482b4207` 已提供与后续
FP32 generic lowering 相同的执行覆盖；冻结 compiler 的 physical generic profile
主要为 S32/U32 logical32。不要以 API 类型测试代替 F32 target/runtime 验证。

## 2. 推荐 element region

```cpp
#include <common/pto_tileop.hpp>
using namespace pto;

void add_if_selected(uint32_t *output, const uint32_t *left,
                     const uint32_t *right) {
  ElementTile<uint32_t, 32> lhs_tile, rhs_tile, out_tile;
  global_tensor<uint32_t, RowMajor<32, 1>> left_memory(left);
  global_tensor<uint32_t, RowMajor<32, 1>> right_memory(right);
  global_tensor<uint32_t, RowMajor<32, 1>> output_memory(output);
  TLOAD(lhs_tile, left_memory);
  TLOAD(rhs_tile, right_memory);
  auto &out = TPARTELEMENT(out_tile);
  auto &lhs = TPARTELEMENT(lhs_tile);
  auto &rhs = TPARTELEMENT(rhs_tile);
#pragma pto element for
  for (unsigned element = 0; element < 32; ++element) {
    if (lhs[element] > 0u)
      out[element] = lhs[element] + rhs[element];
    else
      out[element] = lhs[element] - rhs[element];
  }
  TSTORE(output_memory, out_tile);
}
```

循环应从零开始、以一递增，并使用可证明的固定 iteration domain。不要在 region
中修改 induction、退出 marked loop、使用异常/goto、递归或未经支持的 helper call。
`if`/`if-else` 的 active lanes 使用 execution mask；不同 element 的执行顺序不保证，
同一 element 内保持源代码顺序。

输入必须来自真实 Tile producer，例如上例 `TLOAD`；`TPARTELEMENT` 不能把普通
vector 参数变成合法 Tile。输出完整发布给后续 Tile consumer，不能直接把裸
Tile-reference 参数当作已验证的跨函数 Tile ABI。

## 3. Gather 与 atomic

冻结组合支持 U32、32-element、System-scope、Monotonic/relaxed atomic add，典型形式为：

```cpp
void histogram(uint32_t *output, const uint32_t *input_keys,
               uint32_t *histogram, unsigned valid) {
  ElementTile<uint32_t, 32> old_values_tile, keys_tile;
  global_tensor<uint32_t, RowMajor<32, 1>> keys_memory(input_keys);
  global_tensor<uint32_t, RowMajor<32, 1>> output_memory(output);
  TLOAD(keys_tile, keys_memory);
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
  TSTORE(output_memory, old_values_tile);
}
```

不同 element 可以命中同一 histogram bin；不要依赖其顺序。普通写入必须是可证明的
unit-stride/injective stream；read/write 混合需要 whole-object NoAlias。volatile、
不支持的 dtype、较强 ordering、非 System scope、错位 atomic、未知调用和复杂 pointer
arithmetic 应被拒绝，而不是用假 `restrict` 或标量 fallback 绕过。

上例按完整 32-element block 读写，输入/输出都必须有足够容量，active key 必须
落在 histogram 范围内。tail 不会缩短整个 Tile 的 TLOAD/TSTORE；不能把它当作
任意短 buffer 的边界保护。

## 4. 编译和验证

generic masked lowering 不是默认路径。需要使用配套 compiler 的：

```text
-O2 -mllvm -linxv5-enable-generic-element=true \
    -mllvm -enable-all-vector-as-tilereg=true
```

带 marked region 的 `-O0`/`optnone` 不支持 masked native lowering。API 类型测试不能
代替 target object 验证；应检查最终 object 的 masked Tile/atomic family，并用固定的
SuperNPUBench 与 SuperScalarModel 版本执行 gfrun/gfsim。不要直接使用当前 `main` 或
分支最新 HEAD 替换上述冻结版本。

编译器必须由主 `linx-toolchain-build` checkout 按固定 LLVM commit 构建，头文件
也必须来自固定 API checkout。记录实际 compiler `--version`、headers revision 和
model revision；不要拿未更新的安装树或另一份旧 compiler binary 充当该组合。

是否支持某个操作，以这个分支的诊断和回归为准；后续 #131 的缺少 opt-in/O0
诊断文本不保证已移植到历史 compiler。这里不承诺旧版本自动使用后来的诊断。

## 5. 与后续分支的边界

LLVM `codex/pto-element-region-compiler` 在冻结 commit 之后增加了 i64/logical64、
部分 FP32/NORM、bounded cast/gather、closed local call-context alias 和更严格的
Tile producer/PHI 检查。它们记录在 LLVM 的 `PTOElementFor.rst`，但不改变本页的
历史组合。PE-domain specialization、MC round-trip preservation、fused zero-store
exception 等 PR #131 能力也不属于这个冻结版本。

完整运行证据请以 SuperScalarModel #925 PR 中的冻结日志、ELF hash 和 manifest 为准。
