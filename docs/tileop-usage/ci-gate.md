# 0.59 门禁与工具链缓存

`Linx TileOP required gate` 的 required check 名称仍为
`Validate jcore/template_asm.hpp`。该汇总 job 只有在 `Compile TileOP contracts`
和 `Validate four-PE startup ELF` 都成功时才成功；失败、取消或跳过任一依赖均不会放行。

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

## 四 PE 运行门禁

模型固定为 SuperScalarModel `72223d2d3b40`。门禁编译生产 compiler-rt archive，
使用同一 LLVM 提交中的 worker 源文件进行一致性校验，从 archive 抽取 worker
链接当前 TileOP adapter 和 `startup.c`，执行：

- `leader-first`：PE0 先到。
- `workers-first`：workers 先到且等待共享初始化。
- `missing`：参与者缺失后 Abort/relaunch。
- `fault-relaunch`：初始化失败、保留已提交效果与新 invocation。

日志、编译命令、反汇编和输入/ELF 哈希上传为 `multi-pe-startup-evidence`。
本门禁证明所固定功能模型上的 freestanding 启动路径，不代表 timing 模型、
Linux 设备 backend、完整 musl 四 PE 或硬件验收。

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
