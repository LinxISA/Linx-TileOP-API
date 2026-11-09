# TileOP 0.59：基于 linx 的增量与规范来源

`0.59` 以 `origin/linx@ad5a2310664041be1f2fad5be1dd8e9d682f23df` 为基底。
原先从 linx 迁移的 9 个提交直接由基底继承，不再重复出现在分支增量中。
本次重整前的远程提交为 `c50b04895c2cc46166fc4f6a444cc8c534ee6b40`，
本地备份分支为 `backup/0.59-before-rebase-424`。仅更新 `0.59`，不修改 linx。

## 普通合同与 #424 可选扩展

[sources.json](../../../../contracts/pto-isa-0.59.0-sources.json) 分开固定三个来源：

| 用途 | 规范来源 |
| --- | --- |
| 119 项 active catalog、普通接口 | PTO-SPEC main `42067d0f2890b8c013a868bd1247ebc6437b3179` |
| 可选 indexed byte range API | 尚未合入 main 的 PR #424，`1981bceffa820e6dc0e56d61b2bb2cfd580e0759` |
| 不可变发布身份 | v0.59.0.0，`b0165320a4b575b18d2f4d93faf40178f661ab49` |

main 和扩展均固定 catalog SHA-256 与完整 ASL/spec tree。catalog 总数仍为 119；
范围重载不新增 TileOp。main 相对发布 catalog 更新 TSTORE 的 Canonicalize/CMode。
API/ISA 身份为 `0.59.0`，publication 为 `0.59.0.0`，encoding ABI 为
`pto-isa-0.59.0-mode-function-v1`。来源检查读取 Git objects，不切换 spec checkout。

**按开发策略保留 #424。** 普通 indexed 接口遵循 main 的 base-only B.IOR；
`IndexedByteRange` 重载遵循 PR 的 Base/Min/Max 合同，不能宣称属于 main。
全部 23 个范围重载、Predicate scatter/atomic-add、文档、capability 和测试均保留。
实例化范围接口要求支持 `__linx_indexed_tlsu_range` 的编译器；未知范围可用普通重载。

## 基底之上的实际增量

- 显式保留多 PE runtime、participant mask、hosted gfrun adapter 和对应测试。
  linx 的 `712328a` 回退了这些实现，本分支通过独立补丁恢复；atomic TSTORE 的零 PE mask 保留。
- 版本头、Makefile 安装头、README、支持表和 engine index 使用 0.59.0；capability 集合一致。
- TLEA：Local RowMajor/M32，S32/S64→S64、U32/U64→U64；使用目的物理列数，
  拒绝 backing dtype 重解释、非法 layout 和静态 shape。动态 element_bits 的合法性由目标端检查。
- TCI/TCI_2D 支持 S64/U64，M16 b64 保持拒绝；row broadcast 的选定槽受有效列数约束。
- 直接继承 linx 的 PredicateCell、#398/#399、mixed binary assembly、Local M32 TLOAD、
  CUBE compare/select 和 Shared ND→CUBE 修复，不重新 cherry-pick 这些历史提交。
- 保留 linx 的共享 prepare-toolchain action、macro coverage 工具、日常工作流和测试；
  required gate 添加 0.59 与 main/#424 检查，通过可选 LLVM_PIN 固定验证工具链。
- usage 生成器读取 main 快照，atomic/reduction dtype 从 ASL 合法集合提取；
  范围接口明确标注独立 PR 来源，普通接口 bundle 保持 base-only。

## 验证方法与结果

```sh
make check
python3 tools/sync_pto059_catalog.py /path/to/pto-spec
python3 tools/check_pto_catalog_alignment.py --spec-repo /path/to/pto-spec --spec-ref origin/main
export LINX_CXX=/path/to/llvm/bin/clang++
export LINX_SYSROOT=/path/to/linx/sysroot
python3 test/test_pto059_gap_api.py -v
python3 test/test_pto_main_contract.py -v
python3 test/test_indexed_tlsu_range.py -v
python3 test/test_predicate_adapters.py -v
```

目标编译使用 LLVM `b19e8c0e184cd7e3a1fa5ab57b368cbf978c3aa8` 和 Linx musl sysroot，
resource overlay 装入当前 checkout 头。它是定向验证版本，并非全部既有接口的兼容保证。

| 检查 | 本次结果 |
| --- | --- |
| make check | 通过；121 个 host 文档示例、119 项 catalog、64 项旧合同、版本/安装、runtime 等；未配置目标环境的测试按原规则跳过 |
| main 与 #424 Git 来源校验 | 通过；catalog SHA-256、规范树与 main ref 一致 |
| main/#424 合同回归 | 7/7 通过，含全部 23 个普通 indexed 操作对象、错误来源/hash/selector 和文档回归 |
| #424 范围 native | 3/3 通过，覆盖 23 个范围重载、零边界、未优化 IR、Predicate 与汇编往返 |
| 新增 0.59 API native | 7/7 通过，含 TLEA、TCI64、动态 shape、非法参数和范围编译器能力诊断 |
| PredicateCell native | 17/17 通过 |
| 共享 CI/cache 与 macro coverage 合同 | 4/4、9/9 通过 |
| 文档生成器 | 已检查 119 项生成，更新 23 个 indexed 页面；保留人工示例 |
| 核心实现保留 | include/ 和 test/common/ 与撤出 #424 前的 `1caf51a` 相同 |

## 外部依赖与已知完整回归限制

- #415 的 worker 启动、每 PE 私有栈、PE0 共享初始化、共同 generation-zero SYNC、
  异常退出和重启依赖 compiler-rt/platform；TileOP 头与 hosted adapter 不构成完整启动 ABI 验收。
- LLVM b19 缺少 `__LINX_PTO_PE_DOMAIN__` 且拒绝既有 TileArray `.reuse` transport。
  前次 participant-mask 为 3/6 通过，原始 `1153ea6` 基线也复现同样失败；本次保留实现与检查。
- 前次完整 frontend gate 在旧 RangeAssemble fixture 的 OPEN parent 对象生成失败，原始基线可复现。
- 前次完整 negatives 为 168 通过、12 失败：10 个 pack/unpack 动态 control 实际生成 trap 而脚本只查语法拒绝，
  一个未定义的 U8 目的负例（当前 API 合法）与一个旧诊断匹配。此次不改动这些既有行为或宣称完整回归通过。
- 未执行硬件/模拟器数值回归；来源、编译与反汇编检查不构成完整 fault ordering、rollback、memory-order 或 ISA 符合性证明。

详细 dtype/layout、PredicateCell、内存及内联边界见[支持状态](../support-status.md)。
