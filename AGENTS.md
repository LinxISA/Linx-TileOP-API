# Agent 工作约定

本文件记录对 AI agent(及人类协作者)的工作约定。修改 TileOP 接口时必须遵守。

## 接口更新必须同步刷新 docs

**规则:任何 `include/jcore/template_asm.hpp`(或其它公共头)的接口变更,
必须在同一个 PR 内同步更新 `docs/tileop-usage/` 对应文档。**

具体要求:

1. **新增重载/接口**:在对应操作的 usage 页
   (`docs/tileop-usage/<category>/<op>/<OP>.md`)的 "C++ 接口" 签名块中
   追加新签名(带完整 `template <...>` 声明行),并在"参数说明"表补充新参数行、
   在"重载选择"一节说明新重载的适用场景。

2. **修改约束/语义**:同步更新该操作页的"约束"、"使用要求"、
   "有效区域与 padding"等小节,以及 `docs/tileop-usage/options.md` 中
   相关的全局说明。

3. **迁移相关变更**:若影响既有调用方式(重载矩阵、参数顺序、语义变化),
   同步更新 `docs/tileop-usage/migration/pto-0583-migration.md`。

4. **跨接口族的系统性变更**(如 groupM、CCTRL、B.DIM 形态):
   除逐接口页面外,还须更新汇总入口
   (`options.md` 的对应章节、`migration/` 迁移条目)。

5. **验证**:提交前运行 `python3 -m unittest test_v058_engine_contract`
   (含文档 freshness 检查),必须 40/40 通过。

参考先例:
- #87 / #93:groupM 重载家族的逐接口签名 + options.md 入口清单 + migration 条目
- #84:CCTRL 相关约束说明

## 其它约定

- Shared 值(TileRegister)不得跨 C++ ABI:测例中 Shared 操作数须由
  `TMOV_L2S_INSERT` 内联产生,不能作为非 inline 函数参数传递。
- 修改 `template_asm.hpp` 后,同步编译器安装树
  (`<toolchain>/lib/clang/15.0.4/include/tileop-api/jcore/`)再编译验证。
- PR 描述中区分:源码编译通过 / 汇编形态验证 / 预存失败(与基线对照)。
