# 0.59 门禁与工具链缓存

`Linx TileOP required gate` 的 required check 名称仍为
`Validate jcore/template_asm.hpp`。该汇总 job 只有在 `Compile TileOP contracts`
成功时才成功；编译 job 失败、取消或跳过均不会放行。门禁保留目标代码编译、
接口合同、同步协议及 runtime adapter 检查，不要求模型运行通过。

## 配套依赖

`.github/workflows/pr-gate.yml` 固定 LLVM `336b59c0bf76`、musl `798f0fb81915`、
Linux `12342453fa8c` 和 jemalloc `4495309cd11c`。更新版本时应一起核对编译参数、
runtime ABI 和测试结果，不应将旧 LLVM 与持续更新的 musl 分支头混用。

编译 fixture 通过 `LINX_TEST_CXXFLAGS` 显式选择配套 musl 的 IntraCore 调用 ABI。
当前 fixture 不验收 C++ exception runtime，因此使用 `-fno-exceptions`；断言保持开启。
同时使用 `-fno-builtin`，避免优化器将带调用约定的 `printf` 改写为未标注的 `puts`。
该选项只传给使用配套运行库的目标 fixture，不改变编译器默认的外部调用拒绝规则。

## 缓存

v3 表示仓库自定义的缓存方案版本。完整 SDK 缓存键包括宿主 OS/CPU/libxml2、
LLVM/musl/Linux/jemalloc 的完整 SHA、toolchain-build SHA、构建 profile、
完整 SDK 的基础 TileOP SHA、构建脚本和 composite action 内容摘要。只恢复完全相同的 SDK，不跨依赖版本
恢复已安装 libc 或构建 stamp。

安装后的 SDK 单独缓存，不包含整个源码、LLVM build tree 或候选 TileOP overlay。
另有受宿主环境隔离的 ccache；其内部内容校验允许新版本复用未变化的编译对象。
源码和构建目录不缓存，避免旧 stamp 或绝对路径导致误用。
候选 TileOP 头文件在保存通用 SDK **之后**安装，并校验 revision 与关键文件。

冷构建只生成门禁使用的编译器工具、musl 静态库/CRT、compiler-rt builtins 和
libc++ 目标头文件。门禁现有 C++ 编译检查不需要链接 libc++；完整 C++ runtime、
pthread、完整 musl 四 PE 服务都不因此获得验收。
`macro-coverage.yml` 使用独立的 `full` profile 和原配套 LLVM/musl pin，仍保留
完整 SDK 构建，避免把仅供编译检查的 SDK 用于 benchmark 链接。

工具链构建出错立即保留首个错误，不再自动删除并重建同一组不兼容源码。
首次生成新 SDK 仍需编译 LLVM，命中缓存后直接核对 revision 和编译参数。

## 四 PE 模型运行独立验证

required gate 不下载、构建或运行 `SuperScalarModel`，也不检查模型仓访问权限，
因此不需要 `SSM_READ_TOKEN`。`AUTOMERGE_TOKEN` 仅供原有自动合并 job 使用。

四 PE ELF 运行工具仍保留在 `test/multi_pe/`，需要时按
[测试说明](../../test/multi_pe/README.md) 独立运行。它覆盖 `leader-first`、
`workers-first`、`missing` 和 `fault-relaunch` 场景，结果不影响 required gate。
门禁通过表示编译及合同检查成功；模型运行、Linux 设备 backend、完整 musl 四 PE
和硬件验收仍分别记录结果。

## 编译器与寄存器载体

切换到配套 LLVM 后，GM 访问会在优化前获得 Scalar scope。Shared handle、
Subview/Assemble 引用和 Local M32 拼装载体必须在此之前通过内联和 SROA 提升为
SSA 值，避免被当作 GM 元数据读写或跨普通函数调用溢出。TileOP 为这些轻量载体、
GM 描述符构造函数和测试中跨 OPEN 区间的加载 lambda 标记 `always_inline`。
LLVM 的配套修复仅折叠聚合返回 ABI 产生的地址/聚合转换，再执行 SROA；
不提前执行 DSE 等内存优化。回归同时验证 handle 消除栈存储和两次外部 GM 写入
均保留 scope。OPEN Tile 与 Shared identity 的合法性检查仍然生效。

目标回归拆成 15 个独立 suite，最多并行 2 个；每个 suite 限时 900 秒，
超时终止整个进程组并使门禁失败。每项日志和退出码保存于 `tileop-compile-evidence`。
