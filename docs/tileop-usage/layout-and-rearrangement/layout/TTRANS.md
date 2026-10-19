# TTRANS（仅历史参考）

`TTRANS` 已从 PTO ISA 0.59.0 active Tile catalog 删除。旧 SFU/TEPL
selector 110 不得由当前 wrapper、associated wrapper 或手写别名发出。

本仓库只保留 `TTRANS(...)` 和 `TTRANS_ASS(...)` 名称用于模板实例化时给出稳定的
编译错误；它们不是可执行 wrapper，也没有 PTO ISA 0.59.0 bundle 组成。需要转置
时请使用当前 golden 定义的 TLOAD/TSTORE layout transport 或矩阵 Shared 转置控制。
