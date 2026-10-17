# TINSERT（仅历史参考）

`TINSERT` 已从 PTO ISA 0.58.7 active Tile catalog 删除。旧 SFU/TEPL
selector 99 不得由当前 wrapper 或手写别名发出。

本仓库只保留 `TINSERT(...)` 名称用于模板实例化时给出稳定的编译错误；它不是
可执行 wrapper，也没有 PTO ISA 0.58.7 bundle 组成。需要区域更新时请使用当前
golden 定义的 region/assemble API，并重新验证发布和生命周期约束。
