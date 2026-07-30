# DavinciOO v5 Intrinsic 对齐状态

> 核对日期：2026-07-30
> 参考仓库：`hengliao1972/DavinciOO`
> 参考分支：`codex/update-intrinsic-docs`
> 参考提交：`3b4fe5e6f7f95d008fc05f6ecb5d1a2acef9fce1`
> 参考目录：`isa/intrinsic`

## 总体结论

当前实现**不能视为与 DavinciOO `isa/intrinsic` 全部对齐**。

现状可以概括为：

- v5 SharedTReg、CUBE FIXP、GMOV、PEID/SSR 相关的主要 opcode 和 header
  encoding 已经补入 LLVM MC 层。
- `TMATMUL_FIXP` 和 `TMATMUL_ACC_FIXP` 的基础 Local form 已具有
  Clang builtin、LLVM intrinsic、SelectionDAG、pseudo 展开和 TileOP API。
- 完整的 FIXP 参数模式、Shared CUBE form、Shared SSA/version 管理、scope/event
  语义、部分 TLSU Shared form 和 TGEMV 指令簇仍未实现。

因此应区分以下三个层次：

1. **Encoding 对齐**：汇编器能够识别、编码和反汇编相应 header/opcode。
2. **编译链对齐**：Clang builtin 可以稳定降低到对应机器指令。
3. **公开语义对齐**：TileOP API、类型系统、事件、scope 和合法性检查与参考规范一致。

当前第一层大体完成，第二层部分完成，第三层仍有明显缺口。

## 已对齐项目

### CUBE Function 编码

LLVM MC 层已经包含以下 DavinciOO v5 CUBE Function：

| Function | 指令 |
| ---: | --- |
| 0 | `TMATMUL` |
| 1 | `TMATMUL.BIAS` |
| 2 | `TMATMUL.ACC` |
| 4 | `TMATMULMX` |
| 5 | `TMATMULMX.BIAS` |
| 6 | `TMATMULMX.ACC` |
| 9 | `TMATMUL.FIXP` |
| 10 | `TMATMUL.BIAS.FIXP` |
| 11 | `TMATMUL.ACC.FIXP` |
| 12 | `TMATMULMX.FIXP` |
| 13 | `TMATMULMX.BIAS.FIXP` |
| 14 | `TMATMULMX.ACC.FIXP` |

上述名称可以通过 `BSTART.CUBE` 汇编、编码并反汇编。

### B.FPATR

已经实现七字段 `B.FPATR`：

```asm
B.FPATR PreQuantMode, ReluMode, GroupNCode,
         RowMaxEn, GroupMaxEn, RowMaxInit, MaxAbsEn
```

基础 FIXP form 当前固定生成：

```asm
B.FPATR 0, 0, 0, 0, 0, 0, 0
```

即不启用 quant、ReLU、RowMax、GroupMax 或 MaxAbs。

### 基础 TMATMUL FIXP 编译链

已经实现：

```cpp
TMATMUL_FIXP(d, a, b);
TMATMUL_ACC_FIXP(d, acc, a, b);
```

基础 Local form 的典型展开为：

```asm
BSTART.CUBE TMATMUL.FIXP, AType
B.DATR      BType, byte0, Null
B.FPATR     0, 0, 0, 0, 0, 0, 0
B.IOT       A, B, mask=1111
B.IOT       mask=1111, TSize=size, last, ->D
```

`TMATMUL_ACC_FIXP` 使用相同的 Local operand stream，但通过专用机器寄存器依赖
读取 implicit ACC。ACC 不占用 `B.IOT` source slot，也不会作为普通可分配 Tile
传入 inline asm。

### TSize

当前 v5 有效编码为：

| TSize | 逻辑大小 |
| ---: | ---: |
| 0 | implicit/无显式大小 |
| 1 | 512 B |
| 2 | 1 KB |
| 3 | 2 KB |
| 4 | 4 KB |
| 5 | 8 KB |
| 6 | 16 KB |
| 7 | 32 KB |

公开 Tile 输出和 GMOV/Shared TMOV API 对显式大小要求 512 B–32 KB。

### GMOV

已经支持 TLSU Function 13 基础编码：

```asm
BSTART.TLSU GMOV, DataType
B.IOT       SrcTile, mask=PEMask, TSize=size, last, ->DstTile
B.IOR       peer_tid, 0, 0
```

当前 API 会检查 source/destination 的 dtype、形状、layout 和逻辑大小一致，且
`peer_tid` 保持运行时 GPR 输入。常量 `peer_tid > 3` 会在后端报错。

### PEID/SSR

已经提供：

```cpp
uint32_t get_thread_id();
uint32_t get_thread_idx(); // 兼容名称
```

两者降低到只读 PEID SSR `0x0802` 的 `SSRGET`，返回值范围语义为 `0..3`。

### Shared TMOV 基础编码

已经包含 TLSU Function 8–11：

- `TMOV.L2S.INSERT`
- `TMOV.L2S.PUBLISH`
- `TMOV.S2L.BROADCAST`
- `TMOV.S2L.EXTRACT`

并能生成 `C.B.IOS + B.IOT` 基础指令序列。

## 尚未对齐项目

### 完整 FIXP 指令簇

以下 opcode 已存在于 MC encoding，但还没有完整的 builtin、LLVM intrinsic、
后端 lowering 和公开 TileOP API：

- `TMATMUL_BIAS_FIXP`
- `TMATMUL_MX_FIXP`
- `TMATMUL_MX_BIAS_FIXP`
- `TMATMUL_MX_ACC_FIXP`

### FIXP 高级参数模式

当前只实现 `B.FPATR` 全零的基础模式，尚未支持：

- `PreQuantMode` 的完整模式组合。
- PReLU/LReLU 参数。
- Tile 或 GPR quant 参数。
- `RowMaxIn`、`RowMaxOut`。
- `GroupMaxOut`。
- `RowMaxInit` 和 `MaxAbsEn` 约束。
- D、RowMaxOut、GroupMaxOut 的紧凑多 destination 编码。
- mode 对应 source/destination 数量的编译期校验。
- FIXP output dtype、rounding、saturation 与 descriptor ABI 的完整推导。

### Shared Right CUBE form

参考规范要求所有具有 Right role 的 TMATMUL/TGEMV variant 支持 Shared Right。
当前尚未实现：

- non-MX 的单个 Shared Right binder。
- MX 的连续两个 binder：Shared Right 和 Shared ScaleRight。
- Shared form 中从 Local `B.IOT` stream 移除被 binder 替代的 operand。
- Shared CUBE 强制 `PE_MASK=1111`。
- 禁止 Shared Left、Bias、ACC 和 output 的完整 verifier。
- 禁止 MX 的 Local/Shared Right 与 ScaleRight 混用。

### Shared ID 和版本语义

当前低层 Shared TMOV API使用显式模板参数：

```cpp
TMOV_L2S_INSERT<SharedId>(src);
TMOV_S2L_EXTRACT<SharedId>(dst);
```

这只暴露了 encoding，不符合参考规范中的最终公开语义。参考要求：

- Shared ID 和版本由编译器分配和绑定。
- C++ 调用方不能直接指定 `S#n`。
- 每个 binder 是一次性的，不能跨 entry 或 block 保持 sticky 状态。
- Shared value 具有 SSA/version 和静态 `defined_mask`。
- Broadcast 只能读取 fully-defined version。
- partial Shared value 只能用于允许 partial 的操作。

上述版本、生命周期和 readiness 检查目前尚未实现。

### Shared TLOAD/TSTORE 与 TSTORE.SPART

MC 层已经识别 TLSU Function 12 `TSTORE.SPART`，但缺少完整的公开接口和编译器
合法性检查。尚需覆盖：

- GM→Shared full load。
- Shared→GM full store。
- `TSTORE<pe_scope>` partition store。
- exactly-one issuer 或 Core4 collective 的静态控制流证明。
- Shared pointer、partition 地址不重叠和 defined-mask 检查。

### TGEMV 指令簇

参考规范定义了以下 CUBE Function，当前尚未实现完整编码和编译链：

| Function | 指令 |
| ---: | --- |
| 16 | `TGEMV` |
| 17 | `TGEMV.BIAS` |
| 18 | `TGEMV.ACC` |
| 20 | `TGEMVMX` |
| 21 | `TGEMVMX.BIAS` |
| 22 | `TGEMVMX.ACC` |

### Scope、事件和收敛性

参考接口使用 `RecordEvent`、`WaitEvents`、`pe_scope` 和 `core_scope` 表达执行与
依赖语义。当前实现主要是同步 `void` wrapper 或低层 builtin，尚未完整实现：

- `RecordEvent` 返回和 wait-event 输入。
- Shared producer/consumer event dependency。
- GMOV 和 Shared CUBE 的 Core4 静态收敛性验证。
- `SYNCALL<core_scope>()` 与 collective 的顺序验证。
- 被 `PE_MASK` 关闭的 PE 的 placeholder/producer-age 语义。

### GMOV 公开语义差异

GMOV 的基础机器编码已经一致，但仍有以下差异：

- 当前 TileOP API 返回 `void`，参考接口返回 `RecordEvent`。
- 当前 API 允许调用方显式设置模板 `PEMask`；参考公开签名不以 scope overload
  改变 Core4 collective 语义。
- 尚未静态证明四个 PE 以相同动态顺序到达 collective。
- `peer_tid` 的运行时越界 precise trap 由硬件负责，编译器目前只检查常量越界。

## 验证情况

已执行以下验证：

- 构建 `llvm-mc`、`llvm-objdump`、`clang` 和 `llc`。
- MC 测试覆盖 CUBE Function 9–14、`B.FPATR`、Shared TLSU 和 GMOV encoding。
- Clang IR 测试覆盖 `blk_matmul_fixp` 和 `blk_matmul_acc_fixp`。
- LLVM CodeGen 测试覆盖基础 `TMATMUL.FIXP` 和 `TMATMUL.ACC.FIXP`。
- TileOP API 实例化后进行对象反汇编，确认两个基础 wrapper 生成正确 opcode、
  `B.DATR`、`B.FPATR`、A/B `B.IOT` 和有效输出 `TSize`。
- `TMATMUL_ACC_FIXP` 不再把 ACC 作为普通 inline-asm Tile operand，因此不会触发
  non-allocatable ACC register class 的寄存器合并崩溃。

## 后续建议顺序

建议按以下顺序继续实现：

1. 补齐四个缺失的 `TMATMUL*.FIXP` builtin 和基础 Local form。
2. 抽象完整 `B.FPATR` descriptor，并实现高级 FIXP source/destination schema。
3. 引入编译器管理的 `SharedTile` SSA/version 和一次性 binder。
4. 为 TMATMUL/TGEMV 增加 Shared Right overload。
5. 实现 Shared TLOAD/TSTORE 和 `TSTORE.SPART`。
6. 实现 TGEMV Function 16–22。
7. 补齐 `RecordEvent`、scope 和 Core4 收敛性 verifier。

在以上项目完成前，不应对外宣称“与 DavinciOO `isa/intrinsic` 全量对齐”；建议使用
“主要 encoding 已对齐，基础 FIXP/GMOV/PEID 可用，完整 Shared 与高级语义仍在实现”
作为当前状态描述。
