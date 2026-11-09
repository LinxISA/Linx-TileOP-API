# Indexed TLSU 范围参数（PR #424）

本扩展对应 [PTO-ISA/pto-spec#424](https://github.com/PTO-ISA/pto-spec/pull/424)
的 `1981bceffa820e6dc0e56d61b2bb2cfd580e0759`，该 PR 尚未合入 main；本分支 API 基线为 0.59.0，main 与 #424 的 catalog/ASL 来源分别固定在
`contracts/pto-isa-0.59.0-sources.json`，范围接口不作为 main 合同的一部分宣称。
全部 23 个 `MGATHER` / `MSCATTER` 及 MASK、CAS、atomic、reduction、POPC 变体
可显式传递 `pto::IndexedByteRange{min, max}`。原有调用形式仍可使用。

`min`、`max` 是相对 GM Base 的 **int64_t 字节位移**，覆盖实际访问的起点；
包含的地址包络为数学区间 `[Base + min, Base + max + AccessBytes)`。
Base 是无符号地址，位移可为负数。范围不是元素数量，也不是行 stride；
它不会缩放索引、逐 lane 检查索引、改变寻址结果或放宽内存顺序。
软件负责保证覆盖和地址不回绕。范围未知时调用原重载，保持保守依赖处理。
编译器目前不自动推导范围。

```cpp
using namespace pto;
IndexedByteRange bytes{-64, 1020};
MGATHER(result, source_gm, byte_offsets, bytes);
MSCATTER(destination_gm, values, byte_offsets, bytes);
MGATHER_MASK(result, source_gm, byte_offsets, mask, bytes);
MSCATTER_MASK(destination_gm, values, byte_offsets, mask, bytes);

// Atomic/reduction 的范围放在 validCol 后、可选 validRow 前。
MGATHER_ADD(old_values, base, byte_offsets, values, validCol, bytes, validRow);
MGATHER_CAS(old_values, base, byte_offsets, expected, replacement,
            validCol, bytes, validRow);
MSCATTER_POPC(base, byte_offsets, validCol, bytes, validRow);
```

`predicate::scatter` 和 `predicate::atomic_add` 也支持最后追加范围参数；
其已有 dtype、静态有效区域和 CUBE_M32 限制继续适用。

无标志 `B.IOR [Base, zero, zero], []` 表示没有范围。
至少一个边界**选择器**非 zero 才表示范围存在，因此 `{0, 0}` 需要把零值
放到普通寄存器。配套 LLVM 新增 `R` asm 约束确保 Min 不使用 zero 选择器。
`__linx_indexed_tlsu_range == 1` 表示编译器具备此支持；
`PTO_TILEOP_API_HAS_INDEXED_TLSU_RANGE == 1` 表示头文件提供范围重载。
需要同时更新 LLVM 和 TileOP 头文件。

与 GPR 执行掩码组合时，范围记录和掩码记录必须立即相邻：

```asm
B.IOR [a0, a1, a2], []
B.IOR [a3, zero, zero], ExecMaskPresent
```

第二条仅包含掩码。仅允许原本合法的 Local CUBE_M16/M32 布局和 GPR 掩码类型；
合法 8 位类型用两个掩码字，其余合法宽度用一个。
PredicateTile 不额外添加掩码 B.IOR。所有标量选择器限于 0..23。

GPR 掩码的有效区域还受以下覆盖上限约束（物理 `Col` 可以更大）：

| 布局 | 类型宽度 | 最大有效行数 | 最大有效列数 |
| --- | --- | --- | --- |
| CUBE_M32 | 16/32/64 位 | 32 | 2 |
| CUBE_M32 | 合法 8 位类型 | 32 | 4 |
| CUBE_M16 | 32 位 | 16 | 2 |
| CUBE_M16 | 16/64 位 | 16 | 4 |
| CUBE_M16 | 合法 8 位类型 | 16 | 8 |

汇编器和宏反汇编拒绝已知越界或为零的有效维度；运行时维度由目标端检查。
范围与 PredicateTile 的组合支持 `PredInv` / `ZeroMask`，并使用额外的
PredicateTile IOT；原子操作也有对应宏形式。默认反汇编会保留这些属性。

`min > max` 在当前参与 PE 产生副作用之前 fault，不回滚其他 PE 的副作用。
PE mask 为零时不读取边界。API 不在主机侧增加统一 preflight 或 trap；
这些执行语义由支持该 ISA 扩展的目标实现负责。

验证覆盖汇编/反汇编往返、显式零范围、64 位负/大边界、builtin 类型检查、
IR lowering 和 23 个 C++ wrapper 的对象生成；不将编译测试视作硬件执行验证。

当前编译验证使用 `-O2` 生成 C++ wrapper 对象，并检查 `-O0` IR。
现有 Local Tile wrapper 在 `-O0` 对象生成时存在栈帧处理崩溃，旧无范围
MGATHER 接口也可复现；本扩展不修复该既有问题。零值 `R` 约束已单独在
`-O0` / `-O2` 验证。

头文件可在不具备范围扩展的旧编译器上解析；实例化 `IndexedByteRange` 重载
需要 `__linx_indexed_tlsu_range == 1`，否则产生明确的编译期诊断。
这只检查范围扩展能力；既有 multi-PE transport 还需要独立的编译器能力，见
[支持状态](support-status.md#编译器组合的已知限制)。
